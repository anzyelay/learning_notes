# Makefile for Smart Light State Machine Project

# 编译器
CC=gcc

# 编译选项
CFLAGS=-std=c99 -Wall -Wextra -O2

# 目标文件名
TARGET=smart_light

# 源文件目录
SRCDIR=src

# 源文件列表
SOURCES=$(SRCDIR)/main.c $(SRCDIR)/state_off.c $(SRCDIR)/state_on.c $(SRCDIR)/state_machine.c

# 对象文件列表
OBJECTS=$(SOURCES:.c=.o)

# 默认目标：构建程序
all: $(TARGET)

# 链接目标程序
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

# 编译源文件为对象文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理生成的文件
clean:
	rm -f $(OBJECTS) $(TARGET)

# 重新构建
rebuild: clean all

.PHONY: all clean rebuild
