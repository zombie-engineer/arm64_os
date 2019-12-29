PROJECT = /home/zombie/projects/raspberry/baremetal_aarch64
INCLUDES = $(PROJECT)/include
SRC = $(PROJECT)

SRC := lib/stringlib/strtol lib/stringlib/sprintf lib/stringlib/stringlib rectangle
SRC := $(addsuffix .o, $(addprefix $(PROJECT)/, $(SRC)))
SRC += test.o

CFLAGS := -g -I$(INCLUDES) -DTEST_STRING

$(info SRC = $(SRC))
%.o: %.c
	@echo Making $@ from $<
	gcc -c $(CFLAGS) -o $@ $<

test: $(SRC)
	gcc $(SRC) -o test


#test: $(SRC) $(INCLUDES)/rectangle.h $(INCLUDES)/stringlib.h
#	ls $(INCLUDES)
#	@echo $(SRC)
#	gcc -g -DTEST_STRING -I$(INCLUDES) $(SRC)
