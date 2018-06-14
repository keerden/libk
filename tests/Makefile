# ==========================================
#   Unity Project - A Test Framework for C
#   Copyright (c) 2007 Mike Karlesky, Mark VanderVoord, Greg Williams
#   [Released under MIT License. Please refer to license.txt for details]
# ==========================================





C_COMPILER=gcc

SRC_DIR = ../src
UNITY_DIR= unity

LIBNAME = k

TARGET_EXTENSION=test

# automatically scan for source files

TESTFILES := $(shell find . -type f  -name "Test*.c"  ! -path "./$(UNITY_DIR)/*")
CFILES := $(shell find . -type f  -name "*.c" ! -name "Test*"  ! -path "./$(UNITY_DIR)/*")
HFILES := $(shell find . -type f  -name "*.h" ! -path "./$(UNITY_DIR)/*")

TEST_TARGETS := $(patsubst %.c,%.$(TARGET_EXTENSION),$(TESTFILES))

CFLAGS=-m32
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wpointer-arith
CFLAGS += -Wcast-align
CFLAGS += -Wwrite-strings
CFLAGS += -Wswitch-default
CFLAGS += -Wunreachable-code
CFLAGS += -Winit-self
CFLAGS += -Wmissing-field-initializers
CFLAGS += -Wno-unknown-pragmas
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wundef
CFLAGS += -Wold-style-definition
INC_DIRS= -I$(SRC_DIR) -I$(UNITY_DIR)/src  -L $(SRC_DIR)
CFLAGS +=  $(INC_DIRS) 

all: clean default test

default: $(TEST_TARGETS)

clean:

clean-all:
	rm $(TEST_TARGETS)

test: default
	@ X=0;							\
	 for test in $(TEST_TARGETS); 	\
	 do 							\
		if $$test; \
			then X=1; \
		fi;							\
	done; 							\
	exit $$X						


%.$(TARGET_EXTENSION): %.c %.runner.c $(CFILES) $(HFILES) 
	 $(C_COMPILER) $(CFLAGS) $(UNITY_DIR)/src/unity.c $(CFILES) $< $(word 2,$^)  -lk  -o $@

%.runner.c: %.c $(HFILES) 
	ruby $(UNITY_DIR)/auto/generate_test_runner.rb $< $@


.PHONY: all default clean clean-all test
