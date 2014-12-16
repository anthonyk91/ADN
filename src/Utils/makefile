include ../makefile-defs

SUFFIX := build/
SRC_DIR := ./
DEP_MAKE := make -k all
CLEAN_MAKE := make -k cleanall

VERSION = DEBUG

DEBUG_CFLAGS = -O0 -g3 -Wall -D_DEBUG
RELEASE_CFLAGS = -O3
PROFILE_CFLAGS = -O3 -g -pg

ifeq "$(VERSION)" "DEBUG"
  CFLAGS = $(DEBUG_CFLAGS)
  LDFLAGS := -g
  TOKEN := Debug
  DEFINES = -D_DEBUG
else
  ifeq "$(VERSION)" "PROFILE"
    CFLAGS = $(PROFILE_CFLAGS)
    LDFLAGS := -g -pg
    TOKEN := Profile
  else
    CFLAGS = $(RELEASE_CFLAGS)
    TOKEN := Release
  endif
endif

#ifndef BUILD_ROOT
#  BUILD_ROOT := ../build/
#endif
BUILD_ROOT := ../../build/

BLD_DIR := $(BUILD_ROOT)$(TOKEN)/
PRODUCT := $(BLD_DIR)/libUtils.a

INCLUDES := -I$(BASE_INCLUDE)
LIBDIRS := 

SRCS :=  macros.cpp \
Associating.cpp\
ParameterParser.cpp \
logger.cpp \
Logging.cpp \
Displaying.cpp\
StatHandler.cpp\
RNGHandler.cpp


OBJS := ${addprefix $(BLD_DIR), $(SRCS:.cpp=.o) }

DEPS := ${addprefix $(BLD_DIR), $(SRCS:.cpp=.d) }
ifneq ($(strip $(DEPS)),)
-include $(DEPS)
endif

all: $(PRODUCT)

$(PRODUCT): mkdirs $(OBJS)
	@echo 'Building target: $@'
	ar -r $(PRODUCT) $(LIBDIRS) ${addsuffix /$(TOKEN), $(DEPLIBDIRS)} $(OBJS)
	@echo 'Target finished'
	
$(BLD_DIR)%.o: $(SRC_DIR)%.cpp
	@echo 'Building file: $@'
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $< -o$@
	#@echo 'Updating depencies for $@'
	echo -n $(@:%.o=%.d) $(dir $@) > $(@:%.o=%.d) && \
	$(CC) -MM -MG $(BLD_FLAGS) -c $< >> $(@:%.o=%.d)
	@echo 'Finished building $@'

%.a:
	@echo 'Refreshing dependency $@'
	cd $(dir $@) && $(DEP_MAKE)

mkdirs:
	@echo Making folders:
	if [ ! -d $(BUILD_ROOT) ]; then mkdir $(BUILD_ROOT); fi
	if [ ! -d $(BLD_DIR) ]; then mkdir $(BLD_DIR); fi

cleanall: clean

clean:
	@echo 'Cleaning build products'
	-rm -rf $(OBJS) $(PRODUCT) $(DEPS)
