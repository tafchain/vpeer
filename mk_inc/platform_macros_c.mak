ifeq ($(plat),android)
include $(WORK_ROOT)/mk_inc/platform_android_linux_c.mak
else ifeq ($(plat),arm)
include $(WORK_ROOT)/mk_inc/platform_arm_linux_c.mak
else ifeq ($(plat),linux)
include $(WORK_ROOT)/mk_inc/platform_linux_c.mak
else ifeq ($(plat),freebsd)
include $(WORK_ROOT)/mk_inc/platform_freebsd_c.mak
else ifeq ($(plat),ios)
include $(WORK_ROOT)/mk_inc/platform_freebsd_c.mak
endif
