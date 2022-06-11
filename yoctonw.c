
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "yoctonw.h"

#define CHECK_OR_GOTO_FAIL(condition) \
	if (!(condition)) { goto fail; }

struct yoctonw_outstream {
	yoctonw_write callback;
	void *callback_handle;
	char *buf;
	size_t buf_len, buf_size;
	struct yoctonw_object *root;
};

struct yoctonw_object {
	struct yoctonw_outstream *outstream;
	struct yoctonw_object *inner;
	int nesting_level;
};

static int is_bare_string(const char *s)
{
	int i;

	for (i = 0; s[i] != '\0'; i++) {
		if (!isalnum(s[i]) && strchr("_-+.", s[i]) == NULL) {
			return 0;
		}
	}

	return 1;
}

static void free_outstream(struct yoctonw_outstream *outstream)
{
	if (outstream == NULL) {
		return;
	}
	free(outstream->buf);
	free(outstream);
}

struct yoctonw_object *yoctonw_write_with(yoctonw_write callback, void *handle)
{
	struct yoctonw_object *obj = NULL;
	struct yoctonw_outstream *outstream = NULL;

	outstream = calloc(1, sizeof(struct yoctonw_outstream));
	CHECK_OR_GOTO_FAIL(outstream != NULL);
	outstream->callback = callback;
	outstream->callback_handle = handle;
	outstream->buf_size = 256;
	outstream->buf_len = 0;
	outstream->buf = calloc(outstream->buf_size, sizeof(char));
	CHECK_OR_GOTO_FAIL(outstream->buf != NULL);

	obj = calloc(1, sizeof(struct yoctonw_object));
	CHECK_OR_GOTO_FAIL(obj != NULL);
	obj->outstream = outstream;
	outstream->root = obj;

	return obj;
fail:
	free(outstream);
	free(obj);
	return NULL;
}

static int fwrite_wrapper(void *buf, size_t nbytes, void *handle)
{
	return fwrite(buf, 1, nbytes, (FILE *) handle) == nbytes;
}

struct yoctonw_object *yoctonw_write_to(FILE *fstream)
{
	return yoctonw_write_with(fwrite_wrapper, fstream);
}

void yoctonw_free(struct yoctonw_object *obj)
{
	if (obj->outstream->root != obj) {
		return;
	}
	free_outstream(obj->outstream);
	free(obj);
}

void yoctonw_write_value(struct yoctonw_object *obj, const char *name,
                         const char *value)
{
}

struct yoctonw_object *yoctonw_write_inner(struct yoctonw_object *obj,
                                           const char *name)
{
	struct yoctonw_object *inner = NULL;
	inner = calloc(1, sizeof(struct yoctonw_object));
	CHECK_OR_GOTO_FAIL(inner != NULL);
	inner->outstream = obj->outstream;
	inner->nesting_level = obj->nesting_level + 1;
	obj->inner = inner;
	// TODO: write "...name {" to output
	return inner;
fail:
	// TODO: log error
	free(inner);
	return NULL;
}


