# ==========================================
#   Unity Project - A Test Framework for C
#   Copyright (c) 2007 Mike Karlesky, Mark VanderVoord, Greg Williams
#   [Released under MIT License. Please refer to license.txt for details]
# ==========================================



CLEANUP = rm -f
MKDIR = mkdir -p
TARGET_EXTENSION=.test


C_COMPILER=gcc
UNITY_ROOT= tests/unity
SRC_DIR = src
TEST_DIR = tests

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

TARGET_BASE1=TestMallocUtil
TARGET1 = $(TARGET_BASE1)$(TARGET_EXTENSION)
SRC_FILES1=$(UNITY_ROOT)/src/unity.c $(SRC_DIR)/kmalloc_util.c $(TEST_DIR)/TestMallocUtil.c  $(TEST_DIR)/runners/TestMallocUtil_Runner.c

TARGET_BASE2=TestMalloc
TARGET2 = $(TARGET_BASE2)$(TARGET_EXTENSION)
SRC_FILES2=$(UNITY_ROOT)/src/unity.c $(SRC_DIR)/kmalloc_util.c $(SRC_DIR)/kmalloc.c $(TEST_DIR)/TestMalloc.c  $(TEST_DIR)/runners/TestMalloc_Runner.c


INC_DIRS=-I$(SRC_DIR) -I$(UNITY_ROOT)/src
SYMBOLS=

all: clean default

default: $(SRC_FILES1)  $(SRC_FILES2) 
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $(SRC_FILES1) -o $(TARGET1)
	./$(TARGET1)

	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SYMBOLS) $(SRC_FILES2) -o $(TARGET2)
	./$(TARGET2)

$(TEST_DIR)/runners/TestMallocUtil_Runner.c: $(TEST_DIR)/TestMallocUtil.c
	ruby $(UNITY_ROOT)/auto/generate_test_runner.rb $(TEST_DIR)/TestMallocUtil.c  $(TEST_DIR)/runners/TestMallocUtil_Runner.c

$(TEST_DIR)/runners/TestMalloc_Runner.c: $(TEST_DIR)/TestMalloc.c
	ruby $(UNITY_ROOT)/auto/generate_test_runner.rb $(TEST_DIR)/TestMalloc.c  $(TEST_DIR)/runners/TestMalloc_Runner.c

clean:
	$(CLEANUP) $(TARGET1) $(TARGET2)

