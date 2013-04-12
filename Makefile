CC     = g++

LIBSRC = librvm.c log.c
RVMLIB = librvm.a
TESTSRC = testRVM.c librvm.c rvm.h librvm.a
TEST = testRVM

CFLAGS = -Wall -g

all: $(RVMLIB) $(TEST)

$(RVMLIB): $(LIBSRC:.c=.o) log.h
	ar rcs $(RVMLIB) $^

$(TEST): $(TESTSRC:.c=.o)
	$(CC) -o $@  $(TESTSRC)

clean:
	rm $(LIBSRC:.c=.o) $(RVMLIB) $(TEST)
