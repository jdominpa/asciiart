NAME := asciiart

SRC_DIR := src
SRCS	:= \
	asciiart.c
SRCS    := $(SRCS:%=$(SRC_DIR)/%)

BUILD_DIR := build
OBJS    := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

CC		:= gcc
CFLAGS	:= -Wall -Wextra -ggdb

RM			:= rm -f
MAKEFLAGS	+= --no-print-directory
DIR_DUP     = mkdir -p $(@D)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(OBJS) -lm -o $(NAME)
	$(info CREATED $(NAME))

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(DIR_DUP)
	$(CC) $(CFLAGS) -c -o $@ $<
	$(info CREATED $@)

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re:
	$(MAKE) fclean
	$(MAKE) all

.PHONY: clean fclean re
.SILENT:
