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

#include "yoctonw.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

struct yoctonw_writer {
	yoctonw_write callback;
	void *callback_handle;
	uint8_t *buf;
	size_t buf_len, buf_size;
	char *printf_buf;
	size_t printf_buf_size;
	int indent_level;
	int error;
};

struct yoctonw_writer *yoctonw_write_with(yoctonw_write callback, void *handle)
{
	struct yoctonw_writer *writer = NULL;

	writer = (struct yoctonw_writer *) calloc(
		1, sizeof(struct yoctonw_writer));
	if (writer == NULL) {
		return NULL;
	}
	writer->callback = callback;
	writer->callback_handle = handle;
	writer->indent_level = 0;
	writer->error = 0;
	writer->buf_size = 256;
	writer->buf_len = 0;
	writer->buf = (uint8_t *) calloc(writer->buf_size, sizeof(char));
	writer->printf_buf = NULL;
	writer->printf_buf_size = 0;
	if (writer->buf == NULL) {
		free(writer);
		return NULL;
	}

	return writer;
}

static int fwrite_wrapper(void *buf, size_t nbytes, void *handle)
{
	return fwrite(buf, 1, nbytes, (FILE *) handle) == nbytes;
}

struct yoctonw_writer *yoctonw_write_to(FILE *fstream)
{
	return yoctonw_write_with(fwrite_wrapper, fstream);
}

void yoctonw_free(struct yoctonw_writer *writer)
{
	free(writer->printf_buf);
	free(writer->buf);
	free(writer);
}

void yoctonw_flush(struct yoctonw_writer *w)
{
	int success;

	if (w->buf_len == 0) {
		return;
	}
	if (w->error) {
		w->buf_len = 0;
		return;
	}
	success = w->callback(w->buf, w->buf_len, w->callback_handle);
	if (!success) {
		w->error = 1;
	}
	w->buf_len = 0;
}

static inline void insert_char(struct yoctonw_writer *w, uint8_t c)
{
	if (w->buf_len >= w->buf_size) {
		yoctonw_flush(w);
	}
	w->buf[w->buf_len] = c;
	++w->buf_len;
}

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

static void write_string(struct yoctonw_writer *w, const char *s)
{
	int i, c;
	char hex[3];

	if (strlen(s) > 0 && is_bare_string(s)) {
		for (i = 0; s[i] != '\0'; ++i) {
			insert_char(w, s[i]);
		}
		return;
	}

	// Some characters need escaping:
	insert_char(w, '"');
	for (i = 0; s[i] != '\0'; i++) {
		c = s[i];
		switch (c) {
		case '\n': insert_char(w, '\\'); insert_char(w, 'n'); break;
		case '\t': insert_char(w, '\\'); insert_char(w, 't'); break;
		case '\\': insert_char(w, '\\'); insert_char(w, '\\'); break;
		case '\"': insert_char(w, '\\'); insert_char(w, '\"'); break;
		default:
			if (c >= 0x20) {
				insert_char(w, c);
				break;
			}
			snprintf(hex, sizeof(hex), "%02x", c);
			insert_char(w, '\\');
			insert_char(w, 'x');
			insert_char(w, hex[0]);
			insert_char(w, hex[1]);
			break;
		}
	}
	insert_char(w, '"');
}

static void write_indent(struct yoctonw_writer *w)
{
	int i;
	for (i = 0; i < w->indent_level; i++) {
		insert_char(w, '\t');
	}
}

void yoctonw_prop(struct yoctonw_writer *w, const char *name,
                   const char *value)
{
	if (w->error) {
		return;
	}
	write_indent(w);
	write_string(w, name);
	insert_char(w, ':');
	insert_char(w, ' ');
	write_string(w, value);
	insert_char(w, '\n');
	// We flush after every top-level def is completed; this means
	// output will always have been flushed before writer is freed.
	if (w->indent_level == 0) {
		yoctonw_flush(w);
	}
}

void yoctonw_subobject(struct yoctonw_writer *w, const char *name)
{
	if (w->error) {
		return;
	}
	write_indent(w);
	write_string(w, name);
	insert_char(w, ' ');
	insert_char(w, '{');
	insert_char(w, '\n');
	++w->indent_level;
}

void yoctonw_end(struct yoctonw_writer *w)
{
	if (w->indent_level == 0) {
		return;
	}
	--w->indent_level;
	write_indent(w);
	insert_char(w, '}');
	insert_char(w, '\n');
	if (w->indent_level == 0) {
		yoctonw_flush(w);
	}
}

int yoctonw_have_error(struct yoctonw_writer *w)
{
	return w->error;
}

static int increase_buffer(struct yoctonw_writer *w, size_t min_size)
{
	char *new_buf;
	size_t new_buf_size;

	if (w->printf_buf_size == 0) {
		new_buf_size = min_size;
	} else {
		new_buf_size = w->printf_buf_size;
	}
	while (new_buf_size < min_size) {
		new_buf_size *= 2;
	}
	new_buf = (char *) realloc(w->printf_buf, new_buf_size);
	if (new_buf == NULL) {
		// TODO: Better error reporting?
		w->error = 1;
		return 0;
	}

	w->printf_buf = new_buf;
	w->printf_buf_size = new_buf_size;
	return 1;
}

void yoctonw_printf(struct yoctonw_writer *w, const char *name,
                    const char *fmt, ...)
{
	va_list args;
	int attempt, sz;

	if (w->error) {
		return;
	}
	if (w->printf_buf_size == 0 && !increase_buffer(w, 128)) {
		return;
	}
	// We try to snprintf and if it fails, we try again with a larger
	// buffer.
	for (attempt = 0; attempt < 2; ++attempt) {
		va_start(args, fmt);
		sz = vsnprintf(w->printf_buf, w->printf_buf_size, fmt, args);
		va_end(args);
		if (sz < w->printf_buf_size) {
			yoctonw_prop(w, name, w->printf_buf);
			return;
		}
		if (!increase_buffer(w, sz + 1)) {
			return;
		}
	}

	// This should never happen.
	// TODO: Better error reporting?
	w->error = 1;
}
