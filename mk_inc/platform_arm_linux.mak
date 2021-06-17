CC_FLAG += -I$(WORK_ROOT)/src -D_REENTRANT -w -D_GNU_SOURCE -D_ARM_LINUX_PLATFORM_ -pthread -pipe

ifeq ($(release),1)
  CC_FLAG += -w -O3 
  ifeq ($(native),1)
    CC_FLAG +=  -march=armv7-a -mtune=cortex-a9 -mfpu=vfpv3-d16 -flto
  endif
else
  #CC_FLAG += -ggdb -fstack-protector -fno-inline -fno-default-inline  -fno-strict-aliasing -fsanitize=address -fsanitize=leak
  CC_FLAG += -ggdb -fstack-protector -fno-inline -fno-default-inline
endif

ifeq ($(release),1)
  CC_LINK += -O3 -s
  ifeq ($(native),1)
    CC_LINK += -march=armv7-a -mtune=cortex-a9 -mfpu=vfpv3-d16 -flto
  endif
else
  CC_LINK += -ggdb -fstack-protector
endif

CC_LINK += -L$(WORK_ROOT)/lib -static-libgcc -static-libstdc++ -laxis2c -pthread

include $(WORK_ROOT)/mk_inc/platform_common.mak

include $(WORK_ROOT)/mk_inc/platform_common.mak

#define compiler
##################################################
CXX           = arm-linux-gnueabihf-g++
CCC           = CXX
CC            = arm-linux-gnueabihf-gcc
PIC           = -fPIC
AR            = arm-linux-gnueabihf-ar
ARFLAGS       = rsuv
SOLINK.cc = $(CXX) $(PIC) -shared -o
##################################################

#compile switch
##################################################