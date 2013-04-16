CC     = g++

LIBSRC = librvm.c log.c
RVMLIB = librvm.a
TESTSRC = testRVM.c
TEST = testRVM

CFLAGS = -Wall -g

all: $(RVMLIB) $(TEST)

$(RVMLIB): $(LIBSRC:.c=.o)
	ar rcs $(RVMLIB) $^

$(TEST): $(TESTSRC:.c=.o) $(RVMLIB)
	$(CC) -o $@ $(TESTSRC) $(RVMLIB)

clean:
	rm $(LIBSRC:.c=.o) $(RVMLIB) $(TEST)
