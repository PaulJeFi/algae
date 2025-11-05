# Compiler and base flags
CXX := g++
CXXFLAGS := -O3 -std=c++17 -ffast-math

# Source files
SOURCES := main.cpp board.cpp sliding_movegen.cpp tt.cpp search.cpp evaluation.cpp ordering.cpp uci.cpp timeman.cpp

# BMI2 detection
UNAME := $(shell uname -s)

ifeq ($(UNAME),Linux)
    HAS_BMI2 := $(shell grep -m1 -o bmi2 /proc/cpuinfo)
else ifeq ($(UNAME),Darwin)
    HAS_BMI2 := $(shell sysctl -n machdep.cpu.features | grep -o BMI2)
else
    # Windows or unknown - assume no BMI2 for safety
    HAS_BMI2 :=
endif

# Targets
TARGET_BMI2 := algae-bmi2
TARGET_COMPAT := algae

# Default builds optimized version if BMI2 available
all:
ifneq ($(HAS_BMI2),)
	@echo "=== Building BMI2 optimized version ==="
	$(MAKE) bmi2
else
	@echo "=== Building compatibility version (BMI2 not detected) ==="
	$(MAKE) compat
endif

# BMI2 optimized version
bmi2:
	$(CXX) $(CXXFLAGS) -mbmi2 -DUSE_BMI2_FORCE -o $(TARGET_BMI2) $(SOURCES)

# Compatibility version
compat:
	$(CXX) $(CXXFLAGS) -o $(TARGET_COMPAT) $(SOURCES)

# Build both versions
both:
	$(CXX) $(CXXFLAGS) -mbmi2 -DUSE_BMI2_FORCE -o $(TARGET_BMI2) $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET_COMPAT) $(SOURCES)

clean:
	rm -f $(TARGET_BMI2) $(TARGET_COMPAT)

.PHONY: all bmi2 compat both clean