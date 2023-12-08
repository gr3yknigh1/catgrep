PHONY := \
	default all build \
	s21_cat s21_grep \
	format fmt lint \
	clean

ALL   :=
CLEAN :=
TESTS :=

THIS_MAKE_FILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
THIS_MAKE_FILE_DIR := $(realpath $(patsubst %/,%,$(dir $(THIS_MAKE_FILE))))

SRCROOT  := $(THIS_MAKE_FILE_DIR)
SRC_DIR  := $(SRCROOT)/src
INC_DIR  := $(SRCROOT)/include
SOURCES  :=


default: all



# ============== [ COMPILATION ] ==============
#
CC     := gcc
CFLAGS := -Wall -Wextra -pedantic -std=c11 -I $(INC_DIR)

ifeq ($(BUILD_CONFIG), DEBUG)
CFLAGS += -O0 -g -D CONFIG_DEBUG
else
CFLAGS += -Werror -O2 -D CONFIG_RELEASE
endif

ifeq ($(USE_SANITIZE), ADDRESS)
CFLAGS += -fsanitize=address
endif


# ============== [ OS ] ==============
#
# NOTE(wittenbb): Reference:
# <https://stackoverflow.com/questions/714100/os-detecting-makefile>
ifeq ($(OS),Windows_NT)
	$(error We in S21 doesn't use Windows...)
else
	UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        CFLAGS += -D OS_LINUX
    endif
    ifeq ($(UNAME_S),Darwin)
        CFLAGS += -D OS_OSX
    endif
endif


# ================= [ SSTD ] =================
#
SSTD_DIR  := $(INC_DIR)/sstd

SSTD_SRCS :=            \
	$(SSTD_DIR)/bits.h  \
	$(SSTD_DIR)/color.h \
	$(SSTD_DIR)/etc.h   \
	$(SSTD_DIR)/sstd.h  \
	$(SSTD_DIR)/types.h

SSTD_OBJS :=

SOURCES += $(SSTD_SRCS)

# ===== [ CAT ] =====
#
CAT_DIR  := $(SRC_DIR)/cat
CAT_BIN  := $(CAT_DIR)/s21_cat
CAT_SRCS := \
	$(CAT_DIR)/cat.c

CAT_OBJS := $(patsubst $(CAT_DIR)/%.c, $(CAT_DIR)/%.o, $(CAT_SRCS))

$(CAT_BIN): $(CAT_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(CAT_DIR)/%.o: $(CAT_DIR)/%.c
	$(CC) $(CFLAGS) -c $^ -o $@

s21_cat: $(CAT_BIN)

s21_cat_test: $(CAT_BIN)
	@$(CAT_DIR)/tests/tests.py

PHONY   += s21_cat
ALL     += s21_cat
CLEAN   += $(CAT_OBJS) $(CAT_BIN)
TESTS   += s21_cat_test

SOURCES += $(CAT_SRCS) $(CAT_DIR)/utils/gen_file.c

# ===== [ GREP ] =====
#
GREP_DIR := $(SRC_DIR)/grep
GREP_BIN := $(GREP_DIR)/s21_grep

GREP_SRCS := \
	$(GREP_DIR)/grep.c \
	$(GREP_DIR)/patterns.c

GREP_OBJS := $(patsubst $(GREP_DIR)/%.c, $(GREP_DIR)/%.o, $(GREP_SRCS))

$(GREP_BIN): $(GREP_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(GREP_DIR)/%.o: $(GREP_DIR)/%.c
	$(CC) $(CFLAGS) -c $^ -o $@

s21_grep: $(GREP_BIN)

ALL     += s21_grep
CLEAN   += $(GREP_OBJS) $(GREP_BIN)
PHONY   += s21_grep
SOURCES += $(GREP_SRCS)

# ============= [ MAIN ] =============

all: $(ALL)

build: $(ALL)

test: $(TESTS)

clean:
	$(RM) $(CLEAN)

include ./make/run.mk

lint: run-clang-format run-cppcheck run-clang-tidy

format fmt:
	clang-format -i $(SOURCES)

.PHONY: $(PHONY)
