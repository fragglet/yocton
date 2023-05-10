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

#ifndef YOCTONW_H
#define YOCTONW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

/**
 * @file yoctonw.h
 *
 * Functions for writing a Yocton file. The entrypoint is to use
 * @ref yocton_write_with or @ref yocton_write_from.
 */

/**
 * Callback invoked to write more data to the output.
 *
 * @param buf       Buffer containing data to write.
 * @param buf_size  Size of buffer in bytes.
 * @param handle    Arbitrary pointer, passed through from
 *                  @ref yocton_write_with.
 * @return          1 for success; 0 for failure. If a failure status
 *                  is returned, the callback will not be invoked
 *                  again.
 */
typedef int (*yoctonw_write)(void *buf, size_t nbytes, void *handle);

struct yoctonw_buffer {
	const uint8_t *data;
	size_t len;
};

struct yoctonw_writer;

/**
 * Start writing a new stream of yocton-encoded data, using the given
 * callback to write more data.
 *
 * @param callback   Callback function to invoke to write data.
 * @param handle     Arbitrary pointer passed through when callback is
 *                   invoked.
 * @return           A @ref yoctonw_writer that can be used to output data.
 */
struct yoctonw_writer *yoctonw_write_with(yoctonw_write callback, void *handle);

/**
 * Start writing a new stream of yocton-encoded data, using the given
 * FILE handle.
 *
 * @param fstream    File handle.
 * @return           A @ref yoctonw_writer that can be used to output data.
 */
struct yoctonw_writer *yoctonw_write_to(FILE *fstream);

/**
 * Free the writer and stop writing the output stream.
 *
 * @param w  The @ref yoctonw_writer.
 */
void yoctonw_free(struct yoctonw_writer *w);

/**
 * Write a new field and value to the output.
 *
 * @param w       Writer.
 * @param name    Field name.
 * @param value   Field value.
 */
void yoctonw_field(struct yoctonw_writer *w, const char *name,
                   const char *value);

/**
 * Start writing a new subobject.
 *
 * The @ref yoctonw_end function should be called to end the subobject.
 *
 * @param w       Writer.
 * @param name    Field name for subobject.
 */
void yoctonw_subobject(struct yoctonw_writer *w, const char *name);

/**
 * End the current subobject.
 *
 * @param w       Writer.
 */
void yoctonw_end(struct yoctonw_writer *w);

/**
 * Check if an error occurred.
 *
 * @return    Non-zero if an error occurred during output (ie. the output
 *            callback function returned zero).
 */
int yoctonw_have_error(struct yoctonw_writer *w);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef YOCTONW_H */

