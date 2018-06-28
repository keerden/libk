# ==========================================
#   Unity Project - A Test Framework for C
#   Copyright (c) 2007 Mike Karlesky, Mark VanderVoord, Greg Williams
#   [Released under MIT License. Please refer to license.txt for details]
# ==========================================



SRC_DIR = src
TEST_DIR = tests

LIBNAME = libk


all: clean lib test


lib:
	@$(MAKE) all -C $(SRC_DIR)

test:
	@$(MAKE) test -C $(TEST_DIR)

clean:
	@$(MAKE) clean -C $(SRC_DIR)
	@$(MAKE) clean -C $(TEST_DIR)

clean-all:
	@$(MAKE) clean-all -C $(SRC_DIR)
	@$(MAKE) clean-all -C $(TEST_DIR)

