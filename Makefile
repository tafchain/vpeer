SHELL = /bin/sh

.PHONY: all
all:

	cd $(WORK_ROOT)/src && $(MAKE) 
	
clean:
#	cd $(WORK_ROOT)/src/3rd && $(MAKE) clean
	cd $(WORK_ROOT)/lib && rm -rf ./libdsc.so
	cd $(WORK_ROOT)/lib && rm -rf ./libaxis2c.so
	cd $(WORK_ROOT)/lib && rm -rf ./libACE.so
	cd $(WORK_ROOT)/src && $(MAKE) clean
	