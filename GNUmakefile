CFLAGS = -Wall -Wc++-compat
TEST_CFLAGS = $(CFLAGS) -g -DALLOC_TESTING

all: yocton_print yocton_test

check: yocton_test
	./yocton_test tests/*

docs: html/index.html

html/index.html: yocton.h yoctonw.h
	doxygen

yocton_print : yocton.o yocton_print.o
	$(CC) $(LDFLAGS) $^ -o $@

yocton_test : yocton.test.o yocton_test.test.o alloc-testing.test.o
	$(CC) $(TEST_CFLAGS) $(LDFLAGS) $^ -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.test.o : %.c
	$(CC) -c $(TEST_CFLAGS) $< -o $@

clean:
	rm -f yocton_print yocton_test \
	   yocton.o yocton_print.o \
	   yocton.test.o yocton_test.test.o alloc-testing.test.o

