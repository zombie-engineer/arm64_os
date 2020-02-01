
fontmaker:
	gcc -g main.c -o $@ -lpng

test: fontmaker
	./fontmaker myfont.pbm myfont.raw && less myfont.h

.PHONY: fontmaker

