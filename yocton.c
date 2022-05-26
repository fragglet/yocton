
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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
	TOKEN_ERROR,
};

struct yocton_instream {
	FILE *file;
	char *buf, *string;
	size_t buf_len, buf_size;
	size_t string_len, string_size;
	unsigned int buf_offset;
	struct yocton_object *root;
};

static int peek_next_char(struct yocton_instream *s, char *c)
{
	if (s->buf_offset >= s->buf_len) {
		s->buf_len = fread(s->buf, sizeof(char), s->buf_size, s->file);
		if (s->buf_len == 0) {
			return 0;
		}
		s->buf_offset = 0;
	}
	*c = s->buf[s->buf_offset];
	return 1;
}

static int read_next_char(struct yocton_instream *s, char *c)
{
	if (!peek_next_char(s, c)) {
		return 0;
	}
	++s->buf_offset;
	return 1;
}

static int is_bare_string_char(int c)
{
       return isalnum(c) || strchr("_-+.", c) != NULL;
}

static int append_string_char(struct yocton_instream *s, char c)
{
	char *new_stringbuf;
	if (s->string_len + 1 >= s->string_size) {
		s->string_size = s->string_size == 0 ? 64 : s->string_size * 2;
		new_stringbuf = realloc(s->string, s->string_size);
		if (new_stringbuf == NULL) {
			return 0;
		}
		s->string = new_stringbuf;
	}
	s->string[s->string_len] = c;
	s->string[s->string_len + 1] = '\0';
	++s->string_len;
	return 1;
}

static int unescape_string_char(char *c) {
	switch (*c) {
		case 'a':  *c = '\a'; return 1;
		case 'b':  *c = '\b'; return 1;
		case 'n':  *c = '\n'; return 1;
		case 'r':  *c = '\r'; return 1;
		case 't':  *c = '\b'; return 1;
		case '\\': *c = '\\'; return 1;
		case '\'': *c = '\''; return 1;
		case '"':  *c = '\"'; return 1;
		default: return 0;
		// TODO: \x, \u etc.
	}
}

// Read quote-delimited "C style" string.
static enum token_type read_string(struct yocton_instream *s)
{
	char c;
	s->string_len = 0;
	for (;;) {
		CHECK_OR_GOTO_FAIL(read_next_char(s, &c));
		if (c == '"') {
			return TOKEN_STRING;
		} else if (c == '\\') {
			CHECK_OR_GOTO_FAIL(read_next_char(s, &c));
			CHECK_OR_GOTO_FAIL(unescape_string_char(&c));
		}
		CHECK_OR_GOTO_FAIL(append_string_char(s, c));
	}
fail:
	// TODO: More detailed error reporting
	return TOKEN_ERROR;
}

static enum token_type read_next_token(struct yocton_instream *s)
{
	char c;
	do {
		if (!read_next_char(s, &c)) {
			return TOKEN_EOF;
		}
	} while (isspace(c));
	s->string_len = 0;
	CHECK_OR_GOTO_FAIL(append_string_char(s, c));
	switch (c) {
		case ':':  return TOKEN_COLON;
		case '{':  return TOKEN_OPEN_BRACE;
		case '}':  return TOKEN_CLOSE_BRACE;
		case '\"': return read_string(s);
		default:   break;
	}
	CHECK_OR_GOTO_FAIL(is_bare_string_char(c));
	for (;;) {
		if (!peek_next_char(s, &c) || !is_bare_string_char(c)) {
			break;
		}
		CHECK_OR_GOTO_FAIL(read_next_char(s, &c));
		CHECK_OR_GOTO_FAIL(append_string_char(s, c));
	}
	return TOKEN_STRING;
fail:
	// TODO: More detailed error reporting
	return TOKEN_ERROR;
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

static void free_instream(struct yocton_instream *instream)
{
	if (instream == NULL) {
		return;
	}
	free(instream->buf);
	free(instream->string);
	if (instream->file != NULL) {
		fclose(instream->file);
	}
	free(instream);
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

	obj->instream = instream;
	obj->field = NULL;
	obj->done = 0;

	instream->root = obj;
	instream->file = file;
	instream->buf_size = 256;
	instream->buf_len = 0;
	instream->buf_offset = 0;
	instream->buf = calloc(instream->buf_size, sizeof(char));
	CHECK_OR_GOTO_FAIL(instream->buf != NULL);
	instream->string_size = 0;
	instream->string = NULL;

	return obj;

fail:
	free_instream(instream);
	free_obj(obj);
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

	free_instream(obj->instream);
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

	f = calloc(1, sizeof(struct yocton_field));
	CHECK_OR_GOTO_FAIL(f != NULL);
	obj->field = f;
	f->name = strdup(obj->instream->string);
	CHECK_OR_GOTO_FAIL(f->name != NULL);

	switch (read_next_token(obj->instream)) {
		case TOKEN_COLON:
			// This is the string:string case.
			f->type = YOCTON_FIELD_STRING;
			CHECK_OR_GOTO_FAIL(
				read_next_token(obj->instream) == TOKEN_STRING);
			f->value = strdup(obj->instream->string);
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
			CHECK_OR_GOTO_FAIL(obj != obj->instream->root);
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
fail:
	assert(0);
	return NULL;
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


