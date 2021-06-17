SHELL = /bin/sh

.PHONY: all
all:
ifneq ($(DSC_TEST),1)
	for subdir in `ls -d */`; do \
		if [ -f $$subdir\Makefile ]; then \
			$(MAKE) -C $$subdir; \
		fi \
	done
endif

clean:
	for subdir in `ls -d */`; do \
		if [ -f $$subdir\Makefile ]; then \
			$(MAKE) clean -C $$subdir; \
		fi \
	done
