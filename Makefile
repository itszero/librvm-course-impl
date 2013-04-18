CC     = gcc

LIBSRC = librvm.c log.c sha256.c
RVMLIB = librvm.a
TESTSRC = testRVM.c
TEST = testRVM

CFLAGS = -Wall -g -std=c99 -DDEBUG

all: $(RVMLIB) $(TEST)

$(RVMLIB): $(LIBSRC:.c=.o)
	ar rcs $(RVMLIB) $^

$(TEST): $(TESTSRC:.c=.o) $(RVMLIB)
	$(CC) -o $@ $(TESTSRC) $(RVMLIB)

clean:
	rm $(LIBSRC:.c=.o) $(RVMLIB) $(TEST)
