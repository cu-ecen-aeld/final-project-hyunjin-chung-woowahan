SYSTEM_MONITOR_VERSION = 33eaf2c6405ae2891f4d1dcddc7738a821cc8d78
SYSTEM_MONITOR_SITE = git@github.com:cu-ecen-aeld/final-project-hyunjin-chung-woowahan.git
SYSTEM_MONITOR_SITE_METHOD = git
SYSTEM_MONITOR_GIT_SUBMODULES = YES

#define SYSTEM_MONITOR_BUILD_CMDS
#	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)/server all
#endef

define SYSTEM_MONITOR_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755 $(@D)/system_monitor/system_data_collector $(TARGET_DIR)/usr/bin
  $(INSTALL) -m 0755 $(@D)/system_monitor/system_data_collector-start-stop $(TARGET_DIR)/etc/init.d/S99system_data_collector
endef

$(eval $(generic-package))
