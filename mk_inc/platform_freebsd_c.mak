OUTPUT_FLAG = -o $@

CC_FLAG = -I$(WORK_ROOT)/src -Wpointer-arith -m64 -D_REENTRANT -D_GNU_SOURCE -w -pipe
ifeq ($(release),1)
CC_FLAG += -O3
else
CC_FLAG += -g -fstack-protector -fno-inline -fno-default-inline
endif

CC_LINK = -m64

ifeq ($(release),1)
CC_LINK += -O3 -mtune=native -march=native -flto
else
CC_LINK += -g -fstack-protector
endif

CXX           = clang
CCC           = clang
CC            = clang
PIC           = -fpic
AR            = ar
ARFLAGS       = rsuv
SOLINK.cc = $(CXX) $(PIC) -shared -o
##################################################

#compile switch
##################################################