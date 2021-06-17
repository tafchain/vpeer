SHELL = /bin/sh

.PHONY: all
all:
	cd $(WORK_ROOT)/src/vbh_server/endorser/plugin/end_comm && $(MAKE)
	cd $(WORK_ROOT)/src/vbh_server/endorser/plugin/end_tas && $(MAKE)
	cd $(WORK_ROOT)/src/vbh_server/endorser/plugin/end_ces && $(MAKE)
#	cd $(WORK_ROOT)/src/vbh_server/endorser/plugin/end_es && $(MAKE)
	
clean:
	for subdir in `ls -d */`; do \
		if [ -f $$subdir\Makefile ]; then \
			$(MAKE) clean -C $$subdir; \
		fi \
	done
