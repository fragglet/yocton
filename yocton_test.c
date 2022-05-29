#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yocton.h"

void evaluate_obj(struct yocton_object *obj)
{
	struct yocton_field *field;

	for (;;) {
		field = yocton_next_field(obj);
		if (field == NULL) {
			return;
		}
		assert(yocton_field_name(field) != NULL);
		if (yocton_field_type(field) == YOCTON_FIELD_OBJECT) {
			evaluate_obj(yocton_field_inner(field));
		} else {
			assert(yocton_field_value(field) != NULL);
		}
	}
}

int run_test(char *filename)
{
	struct yocton_object *obj;
	FILE *fstream;
	const char *error_msg;
	int have_error, lineno, success;

	fstream = fopen(filename, "r");
	assert(fstream != NULL);
	obj = yocton_read_from(fstream);
	evaluate_obj(obj);

	success = 1;
	have_error = yocton_have_error(obj, &lineno, &error_msg);
	if (strstr(filename, "/error-") != NULL) {
		if (!have_error) {
			fprintf(stderr, "expected error when evaluating "
			        "'%s'; got none\n", filename);
			success = 0;
		}
	} else if (have_error) {
		fprintf(stderr, "error when parsing %s: %s\n",
		        filename, error_msg);
		success = 0;
	}
	yocton_free(obj);

	return success;
}

int main(int argc, char *argv[])
{
	int i;
	int success = 1;

	for (i = 1; i < argc; i++) {
		success = success && run_test(argv[i]);
	}

	exit(!success);
	return 0;
}

