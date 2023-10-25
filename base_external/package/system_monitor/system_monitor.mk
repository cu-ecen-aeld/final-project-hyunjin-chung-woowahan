SYSTEM_MONITOR_VERSION = 8d12940a3ae8b590f41e18c57a6a1fb868773bcc
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
