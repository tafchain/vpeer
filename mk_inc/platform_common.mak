OUTPUT_FLAG = -o $@

ifneq ($(delay),1)
  CC_FLAG += -DDSC_NO_DELAY
endif

ifeq ($(release),1)
  CC_FLAG += -DUSING_GARBAGE_RECLAIM -DNDEBUG
else
  CC_FLAG += -DDONNT_USING_MEMORY_POOL -DUSING_DSC_LOG_BIN
endif

# MARIADB MACROS
###################################################################
ifeq ($(mariadb),1)
  CC_FLAG +=  -DUSING_MYSQL_DB -I$(MARIADB_ROOT)/include/mariadb
  CC_LINK += -L$(MARIADB_ROOT)/lib/mariadb -lmariadb
endif
#
###################################################################

# SQLITE MACROS
###################################################################
ifeq ($(sqlite_db),1)
  CC_FLAG += -DUSING_SQLITE_DB -I$(WORK_ROOT)/src/3rd/sqlite
  CC_LINK += -lvbhsqlite
endif
#
###################################################################

# ACE MACROS
###################################################################
#
ifeq ($(ACE),1)
  CC_FLAG +=  -I$(ACE_ROOT) -DACE_HAS_EXCEPTIONS -D__ACE_INLINE__ -DACE_HAS_LIMITED_SELECT
  ifeq ($(release),1)
    CC_FLAG += -DACE_NDEBUG
  endif
  CC_LINK += -lACE
endif
#

###################################################################

# OPENSSL MACROS
###################################################################
#
ifeq ($(OPENSSL),1)
  CC_FLAG += -I$(SSL_ROOT)/include
  CC_LINK += -L$(SSL_ROOT)/lib

  ifeq ($(plat),ios)
    CC_LINK += -lcrypto -lssl
  else
    CC_LINK += -lcryptovbh -lsslvbh
  endif
endif

ifeq ($(use_smx),1)
  CC_FLAG += -DVBH_USE_SMX
endif

ifeq ($(use_sign),1)
  CC_FLAG += -DVBH_USE_SIGNATURE
endif

# 将插件链接到可执行程序，方便valgrind探测插件中的问题
ifeq ($(dsc_test),1)
  CC_FLAG += -DDSC_TEST
endif

#
###################################################################

