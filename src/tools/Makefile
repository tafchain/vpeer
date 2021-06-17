SHELL = /bin/sh

.PHONY: all
all:
	cd $(WORK_ROOT)/src/vbh_server/vbh_server_comm && $(MAKE)
	cd $(WORK_ROOT)/src/vbh_server/cc && $(MAKE)
	cd $(WORK_ROOT)/src/vbh_server/committer && $(MAKE)
	cd $(WORK_ROOT)/src/vbh_server/endorser && $(MAKE)
#	cd $(WORK_ROOT)/src/vbh_server/order && $(MAKE)
	
clean:
	for subdir in `ls -d */`; do \
		if [ -f $$subdir\Makefile ]; then \
			$(MAKE) clean -C $$subdir; \
		fi \
	done
	