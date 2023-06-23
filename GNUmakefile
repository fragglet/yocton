LIB_OBJS = yocton.o yoctonw.o
TEST_OBJS = yocton.test.o yocton_test.test.o alloc-testing.test.o
GCOV_OBJS = $(subst .test.o,.gcov.o,$(TEST_OBJS))

CFLAGS = -Wall -Wc++-compat
TEST_CFLAGS = $(CFLAGS) -g -DALLOC_TESTING
GCOV_CFLAGS = $(TEST_CFLAGS) -fprofile-arcs -ftest-coverage

all: yocton_print yocton_test

check: yocton_test
	./yocton_test tests/*
	./yocton_test.py

coverage : yocton.c.gcov

%.c.gcov: %.gcov.gcda
	gcov $<

%.gcov.gcda : yocton_test_gcov
	rm -f yocton.gcov.gcda
	./yocton_test_gcov tests/*

docs: html/index.html

html/index.html: $(wildcard *.h *.md) Doxyfile
	doxygen

yocton_print : yocton_print.o $(LIB_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

yocton_test : $(TEST_OBJS)
	$(CC) $(TEST_CFLAGS) $(LDFLAGS) $^ -o $@

yocton_test_gcov : $(GCOV_OBJS)
	$(CC) $(GCOV_CFLAGS) $(LDFLAGS) $^ -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.gcov.o : %.c
	$(CC) -c $(GCOV_CFLAGS) $< -o $@

%.test.o : %.c
	$(CC) -c $(TEST_CFLAGS) $< -o $@

clean:
	rm -f yocton_print $(LIB_OBJS) \
	      yocton_test $(TEST_OBJS) \
	      yocton_test_gcov $(GCOV_OBJS) \
	          $(subst .gcov.o,.gcov.gcno,$(GCOV_OBJS)) \
	          $(subst .gcov.o,.c.gcov,$(GCOV_OBJS)) \
	          $(subst .gcov.o,.gcov.gcda,$(GCOV_OBJS))

