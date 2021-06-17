SHELL = /bin/sh

all:
	cd $(WORK_ROOT)/src/vbh_server/endorser/plugin && $(MAKE)
	cd $(WORK_ROOT)/src/vbh_server/endorser/end_app && $(MAKE)

clean:
	for subdir in `ls -d */`; do \
		if [ -f $$subdir\Makefile ]; then \
			$(MAKE) clean -C $$subdir; \
		fi \
	done
