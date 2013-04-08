LIBSRC = librvm.c
RVMLIB = librvm.a

CFLAGS = -Wall -g

$(RVMLIB): $(LIBSRC:.c=.o)
	ar rcs $(RVMLIB) $^

clean:
	rm $(LIBSRC:.c=.o) $(RVMLIB)
