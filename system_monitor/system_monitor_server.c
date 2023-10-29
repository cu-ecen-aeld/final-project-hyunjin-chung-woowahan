#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/queue.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#define BUF_SIZE (1024 * 1024)

#define FILE_PATH "/var/tmp/system_data"

typedef struct thread_info
{
	int sockfd;
	char* buf;
	bool is_success;
	bool is_complete;
}thread_info_t;

typedef struct slist_thread
{
	pthread_t thread;
	thread_info_t thread_info;
	SLIST_ENTRY(slist_thread) entries;
}slist_thread_t;

static bool disconnect = false;
pthread_mutex_t mutex;

static void signal_handler(int signal_number)
{
	if (signal_number == SIGINT || signal_number == SIGTERM)
	{
		disconnect = true;
	}
}

static bool write_file(char* buf, int data_size)
{
  FILE* file;

  if ((file = fopen(FILE_PATH, "a+")) == NULL)
  {
      printf("fail to open write file\n");
      return false;
  }

	for (int i = 0; i < data_size; i++)
	{
		if (buf[i] == '\n')
		{
			buf[i] = '\0';
		  printf("write string: %s\n", buf);

			if (fprintf(file, "%s\n", buf) < 0)
			{
				printf("fail to write file\n");
				return false;
			}

			buf = &buf[i + 1];
		}
	}

  if (fclose(file) != 0)
  {
    printf("fail to close file\n");
    return false;
  }

	return true;
}

static int init_socket(int *sockfd, char* ipaddr)
{
	struct sigaction new_action;
	struct addrinfo hints;

	memset(&new_action, 0, sizeof(struct sigaction));
	new_action.sa_handler = signal_handler;

	if (sigaction(SIGTERM, &new_action, NULL) != 0)
	{
		perror("fail to register SIGTERM signal handler");
		return -1;
	}

	if (sigaction(SIGINT, &new_action, NULL) != 0)
	{
		perror("fail to register SIGINT signal handler");
		return -1;
	}

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	printf("start aesdsocket\n");

	if ((*sockfd = socket(hints.ai_family, hints.ai_socktype, 0)) == -1)
	{
		perror("fail to create socket");
		return -1;
	}

	printf("create socket\n");


	int option = 1;
	if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) == -1)
	{
		perror("fail to set socket option");
		return -1;
	}

	printf("set socket\n");

	struct addrinfo *servinfo;
	int getaddrinfo_err;
	if ((getaddrinfo_err = getaddrinfo(NULL, "9000", &hints, &servinfo)))
	{
		printf("%d\n", getaddrinfo_err);
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getaddrinfo_err));
		return -1;
	}

	const char* p_ip_addr;
	if ((p_ip_addr = inet_ntop(AF_INET, servinfo->ai_addr, ipaddr, INET_ADDRSTRLEN)) == NULL)
	{
		freeaddrinfo(servinfo);
		perror("fail to convert ipv4 string");
		return -1;
	}

	syslog(LOG_INFO, "Accepted connection from %s\n", p_ip_addr);
	printf("ip: %s\n", p_ip_addr);

	if (bind(*sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
	{
		freeaddrinfo(servinfo);
		perror("fail to bind socket");
		return -1;
	}

	printf("bind socket\n");

	freeaddrinfo(servinfo);

	if (listen(*sockfd, 2) == -1)
	{
		perror("fail to listen socket");
		return -1;
	}

	printf("listen\n");

	int flags;
	if ((flags = fcntl(*sockfd, F_GETFL)) == -1)
	{
		perror("fail to get flags");
		return -1;
	}

	if (fcntl(*sockfd, F_SETFL, flags | O_NONBLOCK))
	{
		perror("fail to set non-blocking io socket");
		return -1;
	}

	printf("complete initialization\n");

	return 0;
}

static void write_timestamp(void)
{
	#define STRFTIME_MAX_SIZE (100)
	char timestamp[STRFTIME_MAX_SIZE];
	static int last_timestamp = 0;
	int current_timestamp = time(NULL);

	if ((current_timestamp - last_timestamp) > 10)
	{
		pthread_mutex_lock(&mutex);
		FILE* file;

		if ((file = fopen(FILE_PATH, "a+")) == NULL)
		{
			printf("fail to open write file\n");
		}

		time_t rawtime;
		struct tm* timeinfo;

		timeinfo = localtime(&rawtime);
		strftime(timestamp, STRFTIME_MAX_SIZE, "%c", timeinfo);

		last_timestamp = current_timestamp;

		if (fprintf(file, "timestamp:%s\n", timestamp) < 0)
		{
			printf("fail to write file\n");
		}

		if (fclose(file) != 0)
		{
			printf("fail to close file\n");
		}

		pthread_mutex_unlock(&mutex);
	}
}

static void* receive_send_data(void* arg)
{
	thread_info_t* p_thread_info = (thread_info_t*)arg;
//char *aesdchar_ioctl_cmd = "AESDCHAR_IOCSEEKTO:";

	while(true)
	{
		int num_bytes_received;

		if ((num_bytes_received = recv(p_thread_info->sockfd, p_thread_info->buf, BUF_SIZE, 0)) < 0)
		{
			perror("fail to receive data");
			p_thread_info->is_success = false;
			break;
		}
		else if (num_bytes_received == 0)
		{
			printf("finished\n");
			break;
		}

    pthread_mutex_lock(&mutex);

    if (write_file(p_thread_info->buf, num_bytes_received) == false)
    {
        pthread_mutex_unlock(&mutex);
        fprintf(stderr, "fail to write file");
        printf("fail to write file\n");
        p_thread_info->is_success = false;
        break;
    }

    pthread_mutex_unlock(&mutex);
	}

	p_thread_info->is_complete = true;
	pthread_exit(NULL);
}

int main(int argc, char * argv[])
{
	int result = 0;
	bool daemon_mode = false;
	int pid = 0;
	int sockfd;
	char ipaddr[INET_ADDRSTRLEN];

	if ((argc == 2) && (!strcmp(argv[1], "-d")))
		daemon_mode = true;

	if (init_socket(&sockfd, ipaddr))
		return -1;

	if (daemon_mode)
		pid = fork();

	if (pid == 0)
	{
		struct sockaddr sockaddr_connected;
		socklen_t sockaddrlen_connected = sizeof(struct sockaddr);

		openlog (NULL, 0, LOG_USER);

		SLIST_HEAD(slisthead, slist_thread) head;
		SLIST_INIT(&head);

		pthread_mutex_init(&mutex, NULL);

		while (true)
		{
			slist_thread_t* p_slist_thread;
			int sockfd_connected;

			if ((sockfd_connected = accept(sockfd, &sockaddr_connected, &sockaddrlen_connected)) == -1)
			{
				if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
				{
					perror("fail to listen socket");
					result = -1;
					disconnect = true;
					break;
				}
			}
			else
			{
				p_slist_thread = malloc(sizeof(slist_thread_t));
				thread_info_t* p_thread_info = &p_slist_thread->thread_info;
				p_thread_info->sockfd = sockfd_connected;
				p_thread_info->buf = malloc(sizeof(char) * BUF_SIZE);
				p_thread_info->is_success = true;
				p_thread_info->is_complete = false;

				pthread_create(&p_slist_thread->thread, NULL, &receive_send_data, (void*)p_thread_info);

				printf("create thread\n");

				SLIST_INSERT_HEAD(&head, p_slist_thread, entries);
			}

			SLIST_FOREACH(p_slist_thread, &head, entries)
			{
				if (p_slist_thread->thread_info.is_complete)
				{
					pthread_join(p_slist_thread->thread, NULL);

					if (!p_slist_thread->thread_info.is_success)
					{
						result = -1;
						disconnect = true;
						printf("fail thread\n");
						break;
					}

					SLIST_REMOVE(&head, p_slist_thread, slist_thread, entries);

					shutdown(p_slist_thread->thread_info.sockfd, SHUT_RDWR);
					free(p_slist_thread->thread_info.buf);
					free(p_slist_thread);

					printf("destroy thread\n");
				}
			}

			write_timestamp();

			if (disconnect)
			{
				printf("disconnected\n");

				while (!SLIST_EMPTY(&head))
				{
					p_slist_thread = SLIST_FIRST(&head);
					while (!p_slist_thread->thread_info.is_complete);
					pthread_join(p_slist_thread->thread, NULL);
					SLIST_REMOVE_HEAD(&head, entries);
					shutdown(p_slist_thread->thread_info.sockfd, SHUT_RDWR);
					free(p_slist_thread->thread_info.buf);
					free(p_slist_thread);

					printf("destroy thread\n");
				}

				break;
			}
		}

		syslog(LOG_INFO, "Closed connection from %s\n", ipaddr);
		closelog();

		shutdown(sockfd, SHUT_RDWR);
	}

	return result;
}
