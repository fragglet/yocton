//
// Copyright (c) 2022, Simon Howard
//
// Permission to use, copy, modify, and/or distribute this software
// for any purpose with or without fee is hereby granted, provided
// that the above copyright notice and this permission notice appear
// in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
// WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
// AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
// CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
// NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
// CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//

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

	if (argc < 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		exit(1);
	}
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

