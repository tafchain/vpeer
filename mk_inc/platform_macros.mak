ifeq ($(plat),android)
include $(WORK_ROOT)/mk_inc/platform_android_linux.mak
else ifeq ($(plat),arm)
include $(WORK_ROOT)/mk_inc/platform_arm_linux.mak
else ifeq ($(plat),linux)
include $(WORK_ROOT)/mk_inc/platform_linux.mak
else ifeq ($(plat),freebsd)
include $(WORK_ROOT)/mk_inc/platform_freebsd.mak
else ifeq ($(plat),ios)
include $(WORK_ROOT)/mk_inc/platform_ios.mak
endif
