// Basic example program that reads a .yocton file and prints the contents.

#include <stdio.h>

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
	struct yocton_object *obj;
	obj = yocton_open(argv[1]);
	print_object(obj, 0);
	yocton_close(obj);
}

