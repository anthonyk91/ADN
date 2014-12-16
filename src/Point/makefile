include ../makefile-defs

ADN_DIR := ../ADN
UTILS_DIR := ../Utils
SUFFIX := build/
SRC_DIR := ./
DEP_MAKE := make -k all
CLEAN_MAKE := make -k cleanall
MOD_MAKE := make -k module

VERSION = DEBUG

DEBUG_CFLAGS = -O0 -g3 -Wall -D_DEBUG
RELEASE_CFLAGS = -O3
PROFILE_CFLAGS = -O3 -pg

ifeq "$(VERSION)" "DEBUG"
  CFLAGS = $(DEBUG_CFLAGS)
  LDFLAGS := -g
  TOKEN := Debug
  DEFINES = -D_DEBUG
else
  ifeq "$(VERSION)" "PROFILE"
    CFLAGS = $(PROFILE_CFLAGS)
    LDFLAGS := -pg
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
PRODUCT := $(BLD_DIR)/adn-point
PRODUCT-STATIC := $(BLD_DIR)/adn-point-static

INCLUDES := -I$(ADN_DIR) -I$(UTILS_DIR) -I$(GP_DIR) -I$(BASE_INCLUDE) 
LIBDIRS := -L$(ADN_DIR) -L$(PARAM_DIR) -L$(BUILD_ROOT)
#DEPLIBDIRS = -L$(EVAG_DIR) -L$(UTILS_DIR) -L$(GP_DIR)
BUILDLIBDIR := -L$(BLD_DIR)

#LIBS := -lEvAgents -lgsl -lgslcblas -lm -lboost_filesystem -lParamContainer -lUtils -lboost_regex -lDotsnBoxes
COMMONLIBS := -lADN -lm -lUtils 
DYNLIBS := -lgsl -lgslcblas -lboost_filesystem -lboost_system -lboost_thread -lboost_program_options
LINKLIBS := $(ROOT_LIBS)libgsl.a $(ROOT_LIBS)libgslcblas.a $(ROOT_LIBS)libboost_filesystem.a $(ROOT_LIBS)libboost_system.a $(ROOT_LIBS)libboost_thread.a $(ROOT_LIBS)libboost_program_options.a
LIBS := $(COMMONLIBS) $(DYNLIBS)
STATICLIBS := $(COMMONLIBS) $(LINKLIBS)

SRCS := PointDriver.cpp\
PointRep.cpp\
PointWorld.cpp\
worlds/MplxWorld.cpp\
worlds/ParityplexWorld.cpp\
PtHarness.cpp\
PtFragment.cpp\
PtCompFragment.cpp\
GradCompFragment.cpp\
GradFragment.cpp\
GradHarness.cpp

OBJS := ${addprefix $(BLD_DIR), $(SRCS:.cpp=.o) }

DEPS := ${addprefix $(BLD_DIR), $(SRCS:.cpp=.d) }
ifneq ($(strip $(DEPS)),)
-include $(DEPS)
endif

all:  depends mkdirs $(OBJS)
	@echo 'Building target: $@'
	$(LD) $(LDFLAGS) $(LIBDIRS) $(DEFINES) $(BUILDLIBDIR) -o $(PRODUCT) $(OBJS) $(LIBS)

all-static:  depends $(OBJS)
	@echo 'Building target: $@'
	$(CC) $(LDFLAGS) $(LIBDIRS) $(DEFINES) $(BUILDLIBDIR) -o $(PRODUCT-STATIC) $(OBJS) $(STATICLIBS) -pthread

mkdirs:
	@echo Making folders:
	if [ ! -d $(BUILD_ROOT) ]; then mkdir $(BUILD_ROOT); fi
	if [ ! -d $(BLD_DIR) ]; then mkdir $(BLD_DIR); fi
	if [ ! -d $(BLD_DIR)worlds ]; then mkdir $(BLD_DIR)worlds; fi

condor:  depends mkdirs $(OBJS)
	@echo 'Building target: $@'
	condor_compile $(LD) $(LDFLAGS) $(LIBDIRS) $(DEFINES) $(BUILDLIBDIR) -o $(PRODUCT) $(OBJS) $(LIBS) $(CONDOR_LIBS)/libstdc++.a $(CONDOR_LIBS)/libgcc.a $(CONDOR_LIBS)/libgcc_eh.a /home/aek/scratch/lib/libpthread.a

$(BLD_DIR)%.o: $(SRC_DIR)%.cpp
	@echo 'Building file: $@'
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $< -o$@
	@echo 'Updating depencies for $@'
	$(CC) -MM -MG $(BLD_FLAGS) -c $< >> $(@:%.o=%.d)
	@echo 'Finished building $@'

depends:
	@ echo 'Building dependency ADN'
	cd $(ADN_DIR) && $(DEP_MAKE)
	
cleanall: clean
	@echo 'Recursive clean'
	cd $(ADN_DIR) && $(CLEAN_MAKE)

clean:
	@echo 'Cleaning build products'
	-rm -rf $(OBJS) $(PRODUCT) $(DEPS)

