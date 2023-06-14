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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "alloc-testing.h"
#include "yocton.h"

enum { FIRST, SECOND, THIRD };
static const char *enum_values[] = {"FIRST", "SECOND", "THIRD", NULL};

struct error_data {
	char *error_message;
	char *expected_output;
	int error_lineno;
};

#define ERROR_ALLOC "memory allocation failure"

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
	struct yocton_prop *property;

	for (;;) {
		property = yocton_next_prop(obj);
		if (property == NULL) {
			return;
		}
		YOCTON_FIELD_STRING(property, *data, error_message);
		YOCTON_FIELD_INT(property, *data, int, error_lineno);
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

	// Read expected output from //> lines.
	data->expected_output = strdup("");
	while (!feof(fstream)) {
		char buf[128];
		fgets(buf, sizeof(buf), fstream);
		if (!strncmp(buf, "//> ", 4)) {
			data->expected_output = (char *) realloc(
				data->expected_output,
				strlen(data->expected_output)
				+ strlen(buf + 4) + 1);
			strcat(data->expected_output, buf + 4);
		}
	}
	rewind(fstream);

	return success;
}

static void integer_value(struct yocton_object *obj)
{
	struct yocton_prop *property;
	struct { int size; } s = {0};
	for (;;) {
		property = yocton_next_prop(obj);
		if (property == NULL) {
			break;
		}
		YOCTON_FIELD_INT(property, s, size_t, size);
		if (!strcmp(yocton_prop_name(property), "value")) {
			yocton_prop_int(property, s.size);
		}
	}
}

static void uinteger_value(struct yocton_object *obj)
{
	struct yocton_prop *property;
	struct { unsigned int size; } s = {0};
	for (;;) {
		property = yocton_next_prop(obj);
		if (property == NULL) {
			break;
		}
		YOCTON_FIELD_UINT(property, s, size_t, size);
		if (!strcmp(yocton_prop_name(property), "value")) {
			yocton_prop_uint(property, s.size);
		}
	}
}

static void enum_value(struct yocton_object *obj)
{
	struct { unsigned int expected, value; } s = {-1, -2};
	for (;;) {
		struct yocton_prop *property = yocton_next_prop(obj);
		if (property == NULL) {
			break;
		}
		YOCTON_FIELD_UINT(property, s, unsigned int, expected);
		YOCTON_FIELD_ENUM(property, s, value, enum_values);
	}
	yocton_check(obj, "wrong enum value matched", s.expected == s.value);
}

static void add_output(struct yocton_object *obj, char **output, const char *s)
{
	char *new_output;
	new_output = (char *) realloc(
		*output, strlen(*output) + strlen(s) + 2);
	if (new_output == NULL) {
		yocton_check(obj, ERROR_ALLOC, 0);
		return;
	}
	*output = new_output;
	strcat(*output, s);
}

struct array_data_item {
	unsigned int id;
	int value;
};

struct array_data {
	unsigned int *unsigneds;
	size_t unsigneds_count;

	int *signeds;
	size_t signeds_count;

	char **strings;
	size_t strings_count;

	int *enums;
	size_t enums_count;

	struct array_data_item *items;
	size_t items_count;
};

static void parse_array_item(struct yocton_object *obj,
                             struct array_data_item *item)
{
	struct yocton_prop *p;

	while ((p = yocton_next_prop(obj)) != NULL) {
		YOCTON_FIELD_UINT(p, *item, unsigned int, id);
		YOCTON_FIELD_INT(p, *item, int, value);
	}
}

static void array_values(struct yocton_object *obj, char **output)
{
	struct array_data ad = {NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0};
	struct yocton_prop *p;
	char buf[32];
	int i;

	while ((p = yocton_next_prop(obj)) != NULL) {
		YOCTON_FIELD_UINT_ARRAY(p, ad, unsigned int,
		                       unsigneds, unsigneds_count);
		YOCTON_FIELD_INT_ARRAY(p, ad, int, signeds, signeds_count);
		YOCTON_FIELD_STRING_ARRAY(p, ad, strings, strings_count);
		YOCTON_FIELD_ENUM_ARRAY(p, ad, enums, enums_count, enum_values);
		YOCTON_FIELD_ARRAY(p, ad, items, items_count, {
			parse_array_item(yocton_prop_inner(p), &ad.items[ad.items_count]);
			++ad.items_count;
		});
	}

	for (i = 0; i < ad.unsigneds_count; ++i) {
		snprintf(buf, sizeof(buf), "%u\n", ad.unsigneds[i]);
		add_output(obj, output, buf);
	}
	free(ad.unsigneds);
	for (i = 0; i < ad.signeds_count; ++i) {
		snprintf(buf, sizeof(buf), "%i\n", ad.signeds[i]);
		add_output(obj, output, buf);
	}
	free(ad.signeds);
	for (i = 0; i < ad.strings_count; ++i) {
		add_output(obj, output, ad.strings[i]);
		add_output(obj, output, "\n");
		free(ad.strings[i]);
	}
	free(ad.strings);
	for (i = 0; i < ad.enums_count; ++i) {
		snprintf(buf, sizeof(buf), "%i\n", ad.enums[i]);
		add_output(obj, output, buf);
	}
	free(ad.enums);
	for (i = 0; i < ad.items_count; ++i) {
		snprintf(buf, sizeof(buf), "{ id %u: value %d }\n",
		         ad.items[i].id, ad.items[i].value);
		add_output(obj, output, buf);
	}
	free(ad.items);
}

static char *string_dup(struct yocton_object *obj, const char *value)
{
	char *result = strdup(value);
	yocton_check(obj, ERROR_ALLOC, result != NULL);
	return result;
}

int evaluate_is_equal(struct yocton_object *obj)
{
	struct yocton_prop *property;
	const char *name;
	int result;
	char *x, *y;

	for (;;) {
		property = yocton_next_prop(obj);
		if (property == NULL) {
			break;
		}
		name = yocton_prop_name(property);
		if (!strcmp(name, "x")) {
			x = string_dup(obj, yocton_prop_value(property));
		} else if (!strcmp(name, "y")) {
			y = string_dup(obj, yocton_prop_value(property));
		}
	}

	result = !strcmp(x, y);
	free(x);
	free(y);
	return result;
}

void evaluate_obj(struct yocton_object *obj, char **output)
{
	struct yocton_prop *property;
	enum yocton_prop_type pt;
	const char *name;

	for (;;) {
		property = yocton_next_prop(obj);
		if (property == NULL) {
			return;
		}
		name = yocton_prop_name(property);
		assert(name != NULL);
		if (!strcmp(name, "special.fail_before_any_property")) {
			yocton_check(yocton_prop_inner(property),
			             "failed before any property was read", 0);
		} else if (!strcmp(name, "special.parse_as_int")) {
			int throwaway;
			yocton_check(obj, "failed to parse as integer",
			    1 == sscanf(yocton_prop_value(property), "%d",
			                &throwaway));
		}
		pt = yocton_prop_type(property);
		if (!strcmp(name, "special.read_as_object")) {
			pt = YOCTON_PROP_OBJECT;
		} else if (!strcmp(name, "special.read_as_string")) {
			pt = YOCTON_PROP_STRING;
		}
		if (!strcmp(name, "special.is_equal")) {
			yocton_check(obj, "values not equal",
			    evaluate_is_equal(yocton_prop_inner(property)));
		} else if (!strcmp(name, "special.integer")) {
			integer_value(yocton_prop_inner(property));
		} else if (!strcmp(name, "special.uinteger")) {
			uinteger_value(yocton_prop_inner(property));
		} else if (!strcmp(name, "special.enum")) {
			enum_value(yocton_prop_inner(property));
		} else if (!strcmp(name, "special.arrays")) {
			array_values(yocton_prop_inner(property), output);
		} else if (pt == YOCTON_PROP_OBJECT) {
			evaluate_obj(yocton_prop_inner(property), output);
		} else {
			assert(yocton_prop_value(property) != NULL);
		}
		if (!strcmp(name, "special.fail_after_last_property")) {
			yocton_check(yocton_prop_inner(property),
			             "failed after last property was read", 0);
		}
		if (!strcmp(name, "output")) {
			add_output(obj, output, yocton_prop_value(property));
			add_output(obj, output, "\n");
		}
	}
}

int run_test_with_limit(char *filename, int alloc_limit)
{
	struct error_data error_data = {NULL};
	struct yocton_object *obj;
	FILE *fstream;
	const char *error_msg;
	char *output;
	int have_error, lineno, success;

	assert(alloc_test_get_allocated() == 0);
	alloc_test_set_limit(-1);

	fstream = fopen(filename, "r");
	assert(fstream != NULL);
	assert(read_error_data_from(filename, fstream, &error_data));
	output = strdup("");

	alloc_test_set_limit(alloc_limit);

	success = 1;
	obj = yocton_read_from(fstream);
	if (obj == NULL) {
		if (alloc_limit == -1) {
			fprintf(stderr, "%s: yocton_read_from failed\n",
			        filename);
			success = 0;
		}
		free(error_data.error_message);
		free(error_data.expected_output);
		free(output);
		return success;
	}

	evaluate_obj(obj, &output);
	fclose(fstream);

	have_error = yocton_have_error(obj, &lineno, &error_msg);
	if (alloc_limit != -1 && strstr(error_msg, ERROR_ALLOC) != NULL) {
		// Perfectly normal to get a memory alloc error.
	} else if (strcmp(output, error_data.expected_output) != 0) {
		fprintf(stderr, "%s: wrong output, want:\n%s\ngot:\n%s\n",
			filename, error_data.expected_output, output);
		success = 0;
	} else if (error_data.error_message == NULL) {
		if (have_error) {
			fprintf(stderr, "%s:%d: error when parsing: %s\n",
				filename, lineno, error_msg);
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
	free(error_data.expected_output);
	free(output);

	if (alloc_test_get_allocated() != 0) {
		fprintf(stderr, "%s: %d bytes still allocated after test\n",
		        filename, alloc_test_get_allocated());
		success = 0;
	}

	return success;
}

static int run_test(char *filename)
{
	int i;
	int success = 1, test_success;

	for (i = -1; i < 50; ++i) {
		test_success = run_test_with_limit(filename, i);
		success = success && test_success;
		// The first time we encounter an error, don't run the test
		// again. There's no point in spamming stderr for a single
		// file.
		if (!test_success) {
			fprintf(stderr, "%s: test failed with limit=%d\n",
			        filename, i);
			break;
		}
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

