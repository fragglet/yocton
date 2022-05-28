
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "yocton.h"

#define ERROR_BUF_SIZE 100

#define ERROR_ALLOC "memory allocation failure"
#define ERROR_EOF   "unexpected EOF"

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
	// callback gets invoked to read more data from input.
	yocton_read callback;
	void *callback_handle;
	// buf is the input buffer containing last data read by callback.
	// buf[buf_offset:buf_len] is still to read.
	char *buf;
	size_t buf_len, buf_size;
	unsigned int buf_offset;
	// string contains the last string token read.
	char *string;
	size_t string_len, string_size;
	// error_buf is non-empty if an error occurs during parsing.
	char *error_buf;
	int lineno;
	struct yocton_object *root;
};

static void input_error(struct yocton_instream *s, char *fmt, ...)
{
	va_list args;

	// We only store the first error.
	if (strlen(s->error_buf) > 0) {
		return;
	}
	va_start(args, fmt);
	vsnprintf(s->error_buf, ERROR_BUF_SIZE, fmt, args);
}

// Assign the result of an allocation, storing an error if result == NULL.
static int assign_alloc(void *ptr, struct yocton_instream *s, void *result)
{
	if (result == NULL) {
		input_error(s, ERROR_ALLOC);
		return 0;
	}
	* ((void **) ptr) = result;
	return 1;
}

static int peek_next_char(struct yocton_instream *s, char *c)
{
	if (s->buf_offset >= s->buf_len) {
		s->buf_len = s->callback(s->buf, s->buf_size,
		                         s->callback_handle);
		if (s->buf_len == 0) {
			return 0;
		}
		s->buf_offset = 0;
	}
	*c = s->buf[s->buf_offset];
	return 1;
}

// Read next char from input. Failure stores an error.
static int read_next_char(struct yocton_instream *s, char *c)
{
	if (!peek_next_char(s, c)) {
		input_error(s, ERROR_EOF);
		return 0;
	}
	++s->buf_offset;
	if (*c == '\n') {
		++s->lineno;
	}
	return 1;
}

static int is_bare_string_char(int c)
{
       return isalnum(c) || strchr("_-+.", c) != NULL;
}

static int append_string_char(struct yocton_instream *s, char c)
{
	if (s->string_len + 1 >= s->string_size) {
		s->string_size = s->string_size == 0 ? 64 : s->string_size * 2;
		CHECK_OR_GOTO_FAIL(
		    assign_alloc(&s->string, s,
		        realloc(s->string, s->string_size)));
	}
	s->string[s->string_len] = c;
	s->string[s->string_len + 1] = '\0';
	++s->string_len;
	return 1;
fail:
	return 0;
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
			if (!unescape_string_char(&c)) {
				input_error(s, "unknown string escape: \\%c", c);
				return TOKEN_ERROR;
			}
		}
		CHECK_OR_GOTO_FAIL(append_string_char(s, c));
	}
fail:
	return TOKEN_ERROR;
}

static enum token_type read_bare_string(struct yocton_instream *s, char first)
{
	char c;
	if (!is_bare_string_char(first)) {
		input_error(s, "unknown token: not valid bare-string character");
		return TOKEN_ERROR;
	}
	s->string_len = 0;
	CHECK_OR_GOTO_FAIL(append_string_char(s, first));
	// Reaching EOF in the middle of the string is explicitly okay here:
	while (peek_next_char(s, &c) && is_bare_string_char(c)) {
		CHECK_OR_GOTO_FAIL(read_next_char(s, &c));
		CHECK_OR_GOTO_FAIL(append_string_char(s, c));
	}
	return TOKEN_STRING;
fail:
	return TOKEN_ERROR;
}

static enum token_type read_next_token(struct yocton_instream *s)
{
	char c;
	if (strlen(s->error_buf) > 0) {
		return TOKEN_ERROR;
	}
	// Skip past any spaces. Reaching EOF is not always an error.
	do {
		if (!peek_next_char(s, &c)) {
			return TOKEN_EOF;
		}
		CHECK_OR_GOTO_FAIL(read_next_char(s, &c));
	} while (isspace(c));

	switch (c) {
		case ':':  return TOKEN_COLON;
		case '{':  return TOKEN_OPEN_BRACE;
		case '}':  return TOKEN_CLOSE_BRACE;
		case '\"': return read_string(s);
		default:   return read_bare_string(s, c);
	}
fail:
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
	struct yocton_object *parent, *child;
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
	free(instream->error_buf);
	free(instream->string);
	free(instream);
}

static size_t fread_wrapper(void *buf, size_t buf_size, void *handle)
{
	return fread(buf, 1, buf_size, (FILE *) handle);
}

struct yocton_object *yocton_read_from(FILE *fstream)
{
	return yocton_read_with(fread_wrapper, fstream);
}

struct yocton_object *yocton_read_with(yocton_read callback, void *handle)
{
	struct yocton_instream *instream = NULL;
	struct yocton_object *obj = NULL;

	instream = calloc(1, sizeof(struct yocton_instream));
	CHECK_OR_GOTO_FAIL(instream != NULL);
	obj = calloc(1, sizeof(struct yocton_object));
	CHECK_OR_GOTO_FAIL(obj != NULL);

	obj->instream = instream;
	obj->field = NULL;
	obj->done = 0;

	instream->root = obj;
	instream->callback = callback;
	instream->callback_handle = handle;
	instream->lineno = 1;
	instream->buf_len = 0;
	instream->buf_offset = 0;
	instream->buf_size = 256;
	CHECK_OR_GOTO_FAIL(
	    assign_alloc(&instream->error_buf, instream,
	        calloc(ERROR_BUF_SIZE, 1)));
	CHECK_OR_GOTO_FAIL(
	    assign_alloc(&instream->buf, instream,
	        calloc(instream->buf_size, sizeof(char))));
	instream->string_size = 0;
	instream->string = NULL;

	return obj;

fail:
	free_instream(instream);
	free_obj(obj);
	return NULL;
}

void yocton_free(struct yocton_object *obj)
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

	CHECK_OR_GOTO_FAIL(
	    assign_alloc(&f, obj->instream,
	        calloc(1, sizeof(struct yocton_field))));
	obj->field = f;
	f->parent = obj;
	CHECK_OR_GOTO_FAIL(
	    assign_alloc(&f->name, obj->instream,
	        strdup(obj->instream->string)));

	switch (read_next_token(obj->instream)) {
		case TOKEN_COLON:
			// This is the string:string case.
			f->type = YOCTON_FIELD_STRING;
			if (read_next_token(obj->instream) != TOKEN_STRING) {
				input_error(obj->instream, "string expected "
				            "to follow ':'");
				goto fail;
			}
			CHECK_OR_GOTO_FAIL(
			    assign_alloc(&f->value, obj->instream,
			        strdup(obj->instream->string)));
			return f;
		case TOKEN_OPEN_BRACE:
			f->type = YOCTON_FIELD_OBJECT;
			CHECK_OR_GOTO_FAIL(
			    assign_alloc(&f->child, obj->instream,
			        calloc(1, sizeof(struct yocton_object))));
			f->child->instream = obj->instream;
			f->child->done = 0;
			return f;
		default:
			input_error(obj->instream, "':' or '{' expected to "
			            "follow field name");
			goto fail;
	}
fail:
	free_field(f);
	return NULL;
}

struct yocton_field *yocton_next_field(struct yocton_object *obj)
{
	if (obj == NULL || obj->done || strlen(obj->instream->error_buf) > 0) {
		return NULL;
	}

	skip_forward(obj);
	free_field(obj->field);
	obj->field = NULL;

	switch (read_next_token(obj->instream)) {
		case TOKEN_STRING:
			return next_field(obj);
		case TOKEN_CLOSE_BRACE:
			if (obj == obj->instream->root) {
				input_error(obj->instream, "closing brace "
				            "'}' not expected at top level");
				return NULL;
			}
			obj->done = 1;
			return NULL;
		case TOKEN_EOF:
			// EOF is only valid at the top level.
			if (obj != obj->instream->root) {
				input_error(obj->instream, ERROR_EOF);
				return NULL;
			}
			obj->done = 1;
			return NULL;
		default:
			input_error(obj->instream, "expected start of "
			            "next field");
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
	if (f->type != YOCTON_FIELD_STRING) {
		input_error(f->parent->instream, "field '%s' has object, "
		            "not value type", f->name);
		return "";
	}
	return f->value;
}

struct yocton_object *yocton_field_inner(struct yocton_field *f)
{
	if (f->type != YOCTON_FIELD_OBJECT) {
		input_error(f->parent->instream, "field '%s' has value, "
		            "not object type", f->name);
		return NULL;
	}
	return f->child;
}

