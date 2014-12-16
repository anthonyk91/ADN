include ../makefile-defs

UTILS_DIR := ../Utils/
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
PRODUCT := $(BLD_DIR)/libADN.a

#INCLUDES := -I../ParamContainer -I/usr/local/include -I/usr/local/include/boost-1_33_1 -I/usr/local/include -I../../Utils
INCLUDES := -I$(UTILS_DIR)  -I$(BASE_INCLUDE)
LIBDIRS := -L$(ROOT_LIBS)
DEPLIBDIRS = -L$(UTILS_DIR)

SRCS := Driver.cpp \
ExternalIO.cpp \
Harness.cpp \
Params.cpp \
World.cpp \
Fragment.cpp \
CompFragment.cpp \
ADN.cpp
#InstHarness.cpp

OBJS := ${addprefix $(BLD_DIR), $(SRCS:.cpp=.o) }

DEPS := ${addprefix $(BLD_DIR), $(SRCS:.cpp=.d) }
ifneq ($(strip $(DEPS)),)
-include $(DEPS)
endif

all: $(PRODUCT)

$(PRODUCT): mkdirs depends $(OBJS)
	@echo 'Building target: $@'
	ar -r $(PRODUCT) $(OBJS)
	@echo 'done'	

$(BLD_DIR)%.o: $(SRC_DIR)%.cpp
	@echo 'Building file: $@'
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $< -o$@
	@echo -n $(@:%.o=%.d) $(dir $@) > $(@:%.o=%.d)
	$(CC) -MM -MG $(CFLAGS) $(INCLUDES) -c $< >> $(@:%.o=%.d)
	@echo 'Finished building $@'

depends:
	@echo 'Refreshing dependency Utils'
	@cd $(UTILS_DIR) && $(DEP_MAKE)

mkdirs:
	@echo Making folders:
	if [ ! -d $(BUILD_ROOT) ]; then mkdir $(BUILD_ROOT); fi
	if [ ! -d $(BLD_DIR) ]; then mkdir $(BLD_DIR); fi

cleanall: clean
	@echo 'Recursive clean'
	cd $(UTILS_DIR) && $(CLEAN_MAKE)

clean:
	@echo 'Cleaning build products'
	-rm -rf $(OBJS) $(PRODUCT) $(DEPS)
