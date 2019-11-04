PROJECT = /home/zombie/projects/raspberry/baremetal_aarch64
INCLUDES = $(PROJECT)/include
SRC = $(PROJECT)

SRC := lib/stringlib/strtol lib/stringlib/sprintf lib/stringlib/stringlib rectangle
SRC := $(addsuffix .c, $(addprefix $(PROJECT)/, $(SRC)))
SRC += test.c

test: test.c $(INCLUDES)/rectangle.h $(INCLUDES)/stringlib.h
	ls $(INCLUDES)
	gcc -g -DTEST_STRING -I$(INCLUDES) $(SRC)
