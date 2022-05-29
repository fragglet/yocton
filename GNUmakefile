
all: yocton_print yocton_test

yocton_print : yocton.o yocton_print.o
	$(CC) $(LDFLAGS) $^ -o $@

yocton_test : yocton.o yocton_test.o
	$(CC) $(LDFLAGS) $^ -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm yocton_print yocton_test \
	   yocton.o yocton_print.o yocton_test.o

