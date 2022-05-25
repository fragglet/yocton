
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "yocton.h"

#define CHECK_OR_GOTO_FAIL(condition) \
	if (!(condition)) { goto fail; }


enum token_type {
	TOKEN_STRING,
	TOKEN_COLON,
	TOKEN_OPEN_BRACE,
	TOKEN_CLOSE_BRACE,
	TOKEN_EOF,
};

struct yocton_instream {
	FILE *file;
	char *buf;
	struct yocton_object *root;
};

static enum token_type read_next_token(struct yocton_instream *f)
{
	// TODO
}

struct yocton_object {
	struct yocton_instream *instream;
	struct yocton_field *field;
	int done;
};

struct yocton_field {
	enum yocton_field_type type;
	char *name, *value;
	struct yocton_object *child;
};

static void free_obj(struct yocton_object *obj);

static void free_field(struct yocton_field *field)
{
	if (field == NULL) {
		return;
	}
	free_obj(field->child);
	field->child = NULL;

	free(field->name);
	free(field->value);
	free(field);
}

static void free_obj(struct yocton_object *obj)
{
	if (obj == NULL) {
		return;
	}
	free_field(obj->field);
	obj->field = NULL;
	free(obj);
}

struct yocton_object *yocton_open(const char *filename)
{
	FILE *file = NULL;
	struct yocton_instream *instream = NULL;
	struct yocton_object *obj = NULL;

	file = fopen(filename, "r");
	CHECK_OR_GOTO_FAIL(file != NULL);

	instream = calloc(1, sizeof(struct yocton_instream));
	CHECK_OR_GOTO_FAIL(instream != NULL);
	obj = calloc(1, sizeof(struct yocton_object));
	CHECK_OR_GOTO_FAIL(obj != NULL);

	instream->root = obj;
	instream->file = file;
	instream->buf = NULL;  // TODO
	obj->instream = instream;
	obj->field = NULL;
	obj->done = 0;

	return obj;

fail:
	free(instream);
	free(obj);
	if (file != NULL) {
		fclose(file);
	}
	return NULL;
}

void yocton_close(struct yocton_object *obj)
{
	if (obj != obj->instream->root) {
		return;
	}

	free(obj->instream->buf);
	fclose(obj->instream->file);
	free_obj(obj);
}

// If we're partway through reading a child object, skip through any
// of its fields so we can read the next of ours.
static void skip_forward(struct yocton_object *obj)
{
	struct yocton_object *child;
	if (obj->field == NULL || obj->field->child == NULL) {
		return;
	}
	child = obj->field->child;
	// Read out all subfields until we get a NULL response and have
	// finished skipping over them.
	while (yocton_next_field(child) != NULL);
	free_obj(child);
	obj->field->child = NULL;
}

static struct yocton_field *next_field(struct yocton_object *obj)
{
	struct yocton_field *f = NULL;
	enum token_type tt;

	f = calloc(1, sizeof(struct yocton_field));
	CHECK_OR_GOTO_FAIL(f != NULL);
	obj->field = f;
	f->name = strdup(obj->instream->buf);
	CHECK_OR_GOTO_FAIL(f->name != NULL);

	switch (read_next_token(obj->instream)) {
		case TOKEN_COLON:
			// This is the string:string case.
			f->type = YOCTON_FIELD_STRING;
			CHECK_OR_GOTO_FAIL(
				read_next_token(obj->instream) == TOKEN_STRING);
			f->value = strdup(obj->instream->buf);
			CHECK_OR_GOTO_FAIL(f->value != NULL);
			return f;
		case TOKEN_OPEN_BRACE:
			f->type = YOCTON_FIELD_OBJECT;
			f->child = calloc(1, sizeof(struct yocton_object));
			CHECK_OR_GOTO_FAIL(f->child != NULL);
			f->child->instream = obj->instream;
			f->child->done = 0;
			return f;
		default:
			// TODO: Save error
			assert(0);
			return NULL;
	}
fail:
	free_field(f);
	// TODO: Save error
	return NULL;
}

struct yocton_field *yocton_next_field(struct yocton_object *obj)
{
	if (obj->done) {
		return NULL;
	}

	skip_forward(obj);
	free_field(obj->field);
	obj->field = NULL;

	switch (read_next_token(obj->instream)) {
		case TOKEN_STRING:
			return next_field(obj);
		case TOKEN_CLOSE_BRACE:
			obj->done = 1;
			return NULL;
		case TOKEN_EOF:
			// EOF is only valid at the top level.
			if (obj == obj->instream->root) {
				obj->done = 1;
				return NULL;
			}
			/* fallthrough */
		default:
			assert(0);
			return NULL;
	}
}

enum yocton_field_type yocton_field_type(struct yocton_field *f)
{
	return f->type;
}

const char *yocton_field_name(struct yocton_field *f)
{
	return f->name;
}

const char *yocton_field_value(struct yocton_field *f)
{
	// TODO: Error if type != YOCTON_FIELD_STRING
	return f->value;
}

struct yocton_object *yocton_field_inner(struct yocton_field *f)
{
	// TODO: Error if type != YOCTON_FIELD_OBJECT
	return f->child;
}


