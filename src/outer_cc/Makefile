SHELL = /bin/sh

.PHONY: all
all:
	cd $(WORK_ROOT)/src/outer_cc/vbh_func_test_cc && $(MAKE)
	
clean:
	for subdir in `ls -d */`; do \
		if [ -f $$subdir\Makefile ]; then \
			$(MAKE) clean -C $$subdir; \
		fi \
	done
	