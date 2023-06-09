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
 * @ref yoctonw_write_with or @ref yoctonw_write_to.
 */

/**
 * Callback invoked to write more data to the output.
 *
 * @param buf       Buffer containing data to write.
 * @param nbytes    Size of buffer in bytes.
 * @param handle    Arbitrary pointer, passed through from
 *                  @ref yoctonw_write_with.
 * @return          1 for success; 0 for failure. If a failure status
 *                  is returned, the callback will not be invoked
 *                  again.
 */
typedef int (*yoctonw_write)(void *buf, size_t nbytes, void *handle);

struct yoctonw_writer;

#ifdef __DOXYGEN__

/**
 * Writer object for generating Yocton-formatted output.
 */
typedef struct yoctonw_writer yoctonw_writer;

#endif

/**
 * Start writing a new stream of yocton-encoded data, using the given
 * callback to write more data. Example:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~
 *   static int write_callback(void *buf, size_t nbytes, void *handle) {
 *       printf("Write callback to write %d bytes\n", nbytes);
 *       return 1;
 *   }
 *
 *   w = yoctonw_write_with(write_callback, NULL);
 * ~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param callback   Callback function to invoke to write data.
 * @param handle     Arbitrary pointer passed through when callback is
 *                   invoked.
 * @return           A @ref yoctonw_writer that can be used to output data.
 */
struct yoctonw_writer *yoctonw_write_with(yoctonw_write callback, void *handle);

/**
 * Start writing a new stream of yocton-encoded data, using the given
 * FILE handle. Example:
 *
 * ~~~~~~~~~~~~~~~~~~~~
 *   FILE *fs = fopen("output.txt", "w");
 *   assert(fs != NULL);
 *   struct yocton_writer *w = yoctonw_write_to(fs);
 * ~~~~~~~~~~~~~~~~~~~~
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
 * For example, the following code:
 * ~~~~~~~~~~~~~~~~~
 *   yoctonw_field(w, "foo", "bar");
 *   yoctonw_field(w, "baz", "qux quux");
 * ~~~~~~~~~~~~~~~~~
 * will produce the following output:
 * ~~~~~~~~~~~~~~~~~
 *   foo: bar
 *   baz: "qux quux"
 * ~~~~~~~~~~~~~~~~~
 *
 * @param w       Writer.
 * @param name    Field name.
 * @param value   Field value.
 */
void yoctonw_field(struct yoctonw_writer *w, const char *name,
                   const char *value);

/**
 * Write a new field with the value constructed printf-style.
 *
 * For example, the following code:
 * ~~~~~~~~~~~~~~~~~
 *   yoctonw_printf(w, "string", "Here is a string: %s", "my string");
 *   yoctonw_printf(w, "int", "%d", 1234);
 *   yoctonw_printf(w, "float", "Here is a float: %.02f", 1234.5678);
 * ~~~~~~~~~~~~~~~~~
 * will produce the following output:
 * ~~~~~~~~~~~~~~~~~
 *   string: "Here is a string: my string"
 *   int: 1234
 *   float: "Here is a float: 1234.57"
 * ~~~~~~~~~~~~~~~~~
 *
 * @param w       Writer.
 * @param name    Field name.
 * @param fmt     Format string
 */
void yoctonw_printf(struct yoctonw_writer *w, const char *name,
                    const char *fmt, ...);

/**
 * Start writing a new subobject.
 *
 * The @ref yoctonw_end function should be called to end the subobject.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~
 *   yoctonw_subobject(w, "subobj");
 *   yoctonw_field(w, "value", "my value");
 *   yoctonw_end(w);
 * ~~~~~~~~~~~~~~~~~~~~~~
 * will produce the following output:
 * ~~~~~~~~~~~~~~~~~~~~~~
 *   subobj {
 *       value: "my value"
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param w       Writer.
 * @param name    Field name for subobject.
 */
void yoctonw_subobject(struct yoctonw_writer *w, const char *name);

/**
 * End the current subobject.
 *
 * See @ref yoctonw_subobject for an example.
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

/**
 * Flush output buffer and write all pending data.
 *
 * Note that data is automatically flushed whenever a new top-level field is
 * written, so the main use of this is to force any pending data to be written
 * while writing a subobject.
 *
 * @param w       Writer.
 */
void yoctonw_flush(struct yoctonw_writer *w);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef YOCTONW_H */

