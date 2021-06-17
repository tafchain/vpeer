###################################################################
# PLATFORM MACROS
###################################################################
#
include $(WORK_ROOT)/mk_inc/platform_macros.mak
#
###################################################################

# Define Objects and PROGRAMS and release ....
#################################################

CC_ARG = $(USER_CC_FLAG) $(CC_FLAG) 
LINK_ARG = $(CC_LINK) $(USER_CC_LINK)
#################################################

OBJECTS = $(SOURCE:.cpp=.o)

#################################################
display_env:
	@echo "[ Compiler: \"$(CCC)\"    Args: \"$(CC_ARG)\" ]"
	@echo "[ Linker: \"$(CCC)\"    Args: \"$(LINK_ARG)\" ]"

clean:
	@echo "[Cleanning...]"
	@rm -rf $(PROGRAM_NAME)
	@rm -rf $(STATIC_LIB_NAME)
	@rm -rf $(SHARED_LIB_NAME)
	@rm -rf $(OBJECTS)

%.o: %.cpp
	$(COMPILE.cc) $(CC_ARG) $< $(OUTPUT_FLAG)

$(PROGRAM_NAME): $(OBJECTS)
	@rm -rf $@
	$(LINK.cc) $(OUTPUT_FLAG) $(OBJECTS) $(CC_ARG) $(LINK_ARG) 

$(STATIC_LIB_NAME): $(OBJECTS)
	@rm -rf $(STATIC_LIB_NAME)
	$(AR) rv $(STATIC_LIB_NAME) $(OBJECTS)
	chmod a+rx $(STATIC_LIB_NAME)

$(SHARED_LIB_NAME): $(OBJECTS)
	@rm -rf $(SHARED_LIB_NAME)
	$(SOLINK.cc) $(SHARED_LIB_NAME) $(OBJECTS) $(LINK_ARG)
