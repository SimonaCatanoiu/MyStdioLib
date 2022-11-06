CC := gcc
CFLAGS := -fpic -Wall -g
CFLAGS2 := -g -shared
objs := libso_stdio.o utils.o ErrorCheck.o

.PHONY:build clean

build: libso_stdio.so
	
libso_stdio.so: $(objs) 
	$(CC) $(CFLAGS2) $(objs) -o $@

libso_stdio.o: ​libso_stdio.c so_stdio.h ErrorCheck.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c $< -o $@

ErrorCheck.o: ErrorCheck.c ErrorCheck.h so_stdio.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
	rm -f libso_stdio.so