#########################
## Library Directories ##
########################

# This FMM library
FMMTL_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
KERNEL_DIR = $(FMMTL_DIR)kernel

# External libraries
BOOST_DIR = /usr/include

# Optional libraries
# Only needed if nvcc exists and using CUDA
THRUST_DIR = /usr/local/cuda-5.5/include

####################
## Makefile Setup ##
####################

# Get the shell name to determine the OS
UNAME := $(shell uname)

# Define the C++ compiler to use
ifeq ($(UNAME), Linux)
CXX := g++-4.7 -std=gnu++0x
endif
ifeq ($(UNAME), Darwin)
CXX := g++-mp-4.7 -std=gnu++0x
endif
# Check for CUDA compiler
NVCC := $(shell which nvcc)

# Dependency directory and flags
DEPSDIR := $(shell mkdir -p .deps; echo .deps)
# MD: Dependency as side-effect of compilation
# MF: File for output
# MP: Include phony targets
DEPSFILE = $(DEPSDIR)/$(notdir $*.d)
DEPSFLAGS = -MD -MF $(DEPSFILE) -MP

# Define any directories containing header files
#   To include directories use -Ipath/to/files
INCLUDES += -I. -I$(FMMTL_DIR) -I$(KERNEL_DIR)
INCLUDES += -I$(BOOST_DIR)

# Define any compile flags
CXXFLAGS  = $(ADDED_FLAGS) -fopenmp -funroll-loops -O3 -W -Wall -Wextra

# Define nvcc compile flags
NVCCFLAGS := $(ADDED_FLAGS) -arch=sm_30 -w --compiler-options "$(CXXFLAGS)"

# Define any directories containing libraries
#   To include directories use -Lpath/to/files
LDFLAGS +=

# Define any libraries to link into executable
#   To link in libraries (libXXX.so or libXXX.a) use -lXXX
LDLIBS  +=

######################
## Makefile Options ##
######################

ifeq ($(NDEBUG),1)
CXXFLAGS += -DFMMTL_NDEBUG
endif
ifeq ($(DEBUG),1)
CXXFLAGS += -g -fno-inline -DFMMTL_DEBUG
endif
ifeq ($(PROFILE),1)
CXXFLAGS += -g -pg
endif
ifeq ($(LOG),1)
CXXFLAGS += -DFMMTL_LOGGING
endif
ifeq ($(NO_CUDA),1)
NVCC :=
endif
# Set up for CUDA if available
ifeq ($(NVCC),)
# NVCC isn't available
CXXFLAGS += -DFMMTL_NO_CUDA
else
# NVCC is available
INCLUDES += -I$(THRUST_DIR)
LDFLAGS += -L/usr/local/cuda-5.0/lib64
LDLIBS  += -lcudart
endif

####################
## Makefile Rules ##
####################

# Suffix replacement rules
#   $^: the name of the prereqs of the rule
#   $<: the name of the first prereq of the rule
#   $@: the name of the target of the rule
.SUFFIXES:                    # Delete the default suffixes
.SUFFIXES: .hpp .cpp .kern .o # Define our suffix list

# 'make' - default rule
all: $(EXEC)

# Default rule for creating an exec of $(EXEC) from a .o file
$(EXEC): % : %.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# Default rule for creating a .o file from a .cpp file
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(DEPSFLAGS) -c -o $@ $<

# Default rule for creating a .o file from a .cu file
%.o: %.cu
	$(NVCC) $(NVCCFLAGS) $(INCLUDES) -c -o $@ $<
	$(NVCC) $(INCLUDES) -M -o $(DEPSFILE) $<

# Default rule for creating a .o file from a .kern file
%.o: %.kern
ifeq ($(NVCC),)  # NVCC isn't available
	ln -s $< $(<:%.kern=%.cpp)
	@$(MAKE) --no-print-directory $@ ADDED_FLAGS='-DFMMTL_KERNEL'
else             # NVCC is availble
	ln -s $< $(<:%.kern=%.cu)
	@$(MAKE) --no-print-directory $@ ADDED_FLAGS='-DFMMTL_KERNEL'
endif

# 'make clean' - deletes all .o and temp files, exec, and dependency file
clean:
	-$(RM) *.o $(OBJS) $(KERNEL_DIR)/*.o
	-$(RM) $(EXEC)
	find $(KERNEL_DIR) -maxdepth 1 -type l -delete
	$(RM) -r $(DEPSDIR)

# Define rules that do not actually generate the corresponding file
.PHONY: clean all

# Include the dependency files
-include $(wildcard $(DEPSDIR)/*.d)