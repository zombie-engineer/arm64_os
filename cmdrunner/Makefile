DIR=cmdrunner

OBJS_CMDRUNNER := \
	cmdrunner\
	cmdrunner_process\
	f5161ah\
	memdump\
	ls\
	help\
	gpio\
	mbox\
	pwm\
	clock\
	shiftreg\
	spi\
	max7219\
	sleep\
	ps

ifeq (CONFIG_NOKIA5110,y)
	OBJS_CMDRUNNER += nokia5110
endif

OBJS_CMDRUNNER := $(addsuffix .o, $(addprefix $(DIR)/, $(OBJS_CMDRUNNER)))

$(info OBJS_CMDRUNNER = $(OBJS_CMDRUNNER))
