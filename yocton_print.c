// Basic example program that reads a .yocton file and prints the contents.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "yocton.h"

void print_indent(int indent)
{
	int i;
	for (i = 0; i < indent; ++i) {
		putchar(' ');
	}
}

void print_object(struct yocton_object *obj, int indent)
{
	struct yocton_field *field;
	for (;;) {
		field = yocton_next_field(obj);
		if (field == NULL) {
			break;
		}
		print_indent(indent);
		printf("%s", yocton_field_name(field));
		if (yocton_field_type(field) == YOCTON_FIELD_OBJECT) {
			printf(":\n");
			print_object(yocton_field_inner(field), indent + 4);
		} else {
			printf(" = \"%s\"\n", yocton_field_value(field));
		}
	}
}

int main(int argc, char *argv[])
{
	FILE *fstream;
	struct yocton_object *obj;
	const char *error;
	int error_lineno;

	fstream = fopen(argv[1], "r");
	if (fstream == NULL) {
		fprintf(stderr, "Error opening %s: %s\n",
		        argv[1], strerror(errno));
		exit(1);
	}
	obj = yocton_read_from(fstream);
	print_object(obj, 0);
	if (yocton_have_error(obj, &error_lineno, &error)) {
		fprintf(stderr, "%d: %s\n", error_lineno, error);
	}
	yocton_free(obj);
	fclose(fstream);
}

