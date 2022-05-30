#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "alloc-testing.h"
#include "yocton.h"

struct error_data {
	char *error_message;
	int error_lineno;
};

size_t read_from_comment(void *buf, size_t buf_size, void *handle)
{
	FILE *fstream = (FILE *) handle;
	int linelen;
	char linebuf[128];

	for (;;) {
		if (fgets(linebuf, sizeof(linebuf), fstream) == NULL) {
			assert(feof(fstream));
			break;
		}
		if (strncmp(linebuf, "//|", 3) != 0) {
			continue;
		}
		linelen = strlen(linebuf + 3);
		assert(linelen < buf_size);
		memcpy(buf, linebuf + 3, linelen);
		return linelen;
	}
	return 0;
}

void read_error_data(struct error_data *data, struct yocton_object *obj)
{
	struct yocton_field *field;
	const char *name;

	for (;;) {
		field = yocton_next_field(obj);
		if (field == NULL) {
			return;
		}
		name = yocton_field_name(field);
		if (!strcmp(name, "error_message")) {
			data->error_message = strdup(yocton_field_value(field));
			assert(data->error_message != NULL);
		} else if (!strcmp(name, "error_lineno")) {
			data->error_lineno = atoi(yocton_field_value(field));
			assert(data->error_lineno > 0);
		}
	}
}

int read_error_data_from(char *filename, FILE *fstream, struct error_data *data)
{
	struct yocton_object *obj;
	const char *error_msg;
	int success = 1;

	obj = yocton_read_with(read_from_comment, fstream);
	read_error_data(data, obj);
	if (yocton_have_error(obj, NULL, &error_msg)) {
		fprintf(stderr, "%s: error in test data: %s\n", filename, error_msg);
		success = 0;
	}
	yocton_free(obj);
	rewind(fstream);
	return success;
}

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
	struct error_data error_data = {NULL};
	struct yocton_object *obj;
	FILE *fstream;
	const char *error_msg;
	int have_error, lineno, success;

	fstream = fopen(filename, "r");
	assert(fstream != NULL);
	assert(read_error_data_from(filename, fstream, &error_data));
	obj = yocton_read_from(fstream);
	evaluate_obj(obj);

	success = 1;
	have_error = yocton_have_error(obj, &lineno, &error_msg);
	if (error_data.error_message == NULL) {
		if (have_error) {
			fprintf(stderr, "%s: error when parsing: %s\n",
				filename, error_msg);
			success = 0;
		}
	} else if (!have_error) {
		fprintf(stderr, "%s: expected error '%s', got none\n",
		        filename, error_data.error_message);
		success = 0;
	} else {
		if (strcmp(error_msg, error_data.error_message) != 0) {
			fprintf(stderr, "%s: wrong error message, want '%s', "
			        "got '%s'\n", filename,
			        error_data.error_message, error_msg);
			success = 0;
		}
		if (lineno != error_data.error_lineno) {
			fprintf(stderr, "%s: wrong error lineno, want %d, "
			        "got %d\n", filename, error_data.error_lineno,
			        lineno);
			success = 0;
		}
	}
	yocton_free(obj);
	free(error_data.error_message);

	if (alloc_test_get_allocated() != 0) {
		fprintf(stderr, "%s: %d bytes still allocated after test\n",
		        filename, alloc_test_get_allocated());
		success = 0;
	}

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

