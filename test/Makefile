PROJECT = /home/zombie/projects/raspberry/baremetal_aarch64
INCLUDES = $(PROJECT)/include
SRC = $(PROJECT)

test: test.c $(INCLUDES)/rectangle.h
	ls $(INCLUDES)
	gcc -g -I$(INCLUDES) test.c $(PROJECT)/rectangle.c
