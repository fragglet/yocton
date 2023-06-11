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

#ifndef YOCTON_H
#define YOCTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

/**
 * @file yocton.h
 *
 * Functions for parsing the contents of a Yocton file. The entrypoint
 * for reading is to use @ref yocton_read_with or @ref yocton_read_from.
 */

/**
 * Callback invoked to read more data from the input.
 *
 * @param buf       Buffer to populate with new data.
 * @param buf_size  Size of buffer in bytes.
 * @param handle    Arbitrary pointer, passed through from
 *                  @ref yocton_read_with.
 * @return          Number of bytes written into the buffer, or zero to
 *                  indicate end of file.
 */
typedef size_t (*yocton_read)(void *buf, size_t buf_size, void *handle);

/** Type of a @ref yocton_field. */
enum yocton_field_type {
	/**
	 * Field that has a string value. @ref yocton_field_value can be
	 * used to get the value.
	 */
	YOCTON_FIELD_STRING,

	/**
	 * Field that has an object value. @ref yocton_field_inner can be
	 * used to read the inner object.
	 */
	YOCTON_FIELD_OBJECT,
};

struct yocton_object;
struct yocton_field;

#ifdef __DOXYGEN__

/**
 * The object is the main abstraction of the Yocton format. Each object
 * can have multiple fields (@ref yocton_field), which can themselves
 * contain more objects.
 */
typedef struct yocton_object yocton_object;

/**
 * An object has multiple fields. Each field has a name which is always
 * a string. It also always has a value, which is either a string
 * (@ref YOCTON_FIELD_STRING) or an object (@ref YOCTON_FIELD_OBJECT).
 * Fields only have a very limited lifetime and are only valid until
 * @ref yocton_next_field is called to read the next field of their
 * parent object.
 */
typedef struct yocton_field yocton_field;

#endif

/**
 * Start reading a new stream of yocton-encoded data, using the given
 * callback to read more data.
 *
 * Simple example of how to use a custom read callback:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   static size_t read_callback(void *buf, size_t buf_size, void *handle) {
 *       static int first = 1;
 *       const char *value = (const char *) handle;
 *       size_t bytes = 0;
 *       if (first) {
 *           bytes = strlen(value) + 1;
 *           assert(buf_size >= bytes);
 *           memcpy(buf, value, bytes);
 *           first = 0;
 *       }
 *       return bytes;
 *   }
 *
 *   obj = yocton_read_with(read_callback, "foo: bar");
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param callback   Callback function to invoke to read more data.
 * @param handle     Arbitrary pointer passed through when callback is
 *                   invoked.
 * @return           A @ref yocton_object representing the top-level object.
 */
struct yocton_object *yocton_read_with(yocton_read callback, void *handle);

/**
 * Start reading a new stream of yocton-encoded data, using the given
 * FILE handle to read more data.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~
 *   FILE *fs = fopen("filename.yocton", "r");
 *   assert(fs != NULL);
 *
 *   struct yocton_object *obj = yocton_read_from(fs);
 * ~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param fstream    File handle.
 * @return           A @ref yocton_object representing the top-level object.
 */
struct yocton_object *yocton_read_from(FILE *fstream);

/**
 * Query whether an error occurred during parsing. This should be called
 * once no more data is returned from obj (ie. when @ref yocton_next_field
 * returns NULL for the top-level object).
 *
 * @param obj        Top-level @ref yocton_object.
 * @param lineno     If an error occurs and this is not NULL, the line number
 *                   on which the error occurred is saved to the pointer
 *                   address.
 * @param error_msg  If an error occurs and this is not NULL, an error message
 *                   describing the error is saved to the pointer address.
 * @return           Non-zero if an error occurred.
 */
int yocton_have_error(struct yocton_object *obj, int *lineno,
                      const char **error_msg);

/**
 * Free the top-level object and stop reading from the input stream.
 *
 * @param obj        Top-level @ref yocton_object.
 */
void yocton_free(struct yocton_object *obj);

/**
 * Perform an assertion and fail with an error if it isn't true.
 *
 * @param obj            @ref yocton_object; may or may not be the top-level
 *                       object.
 * @param error_msg      The error message to log if normally_true is zero.
 * @param normally_true  If this is zero, an error is logged.
 */
void yocton_check(struct yocton_object *obj, const char *error_msg,
                  int normally_true);

/**
 * Read the next field of an object.
 *
 * Example that prints the names and values of all string fields:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   struct yocton_field *f;
 *   while ((f = yocton_next_field(obj)) != NULL) {
 *       if (yocton_field_type(f) == YOCTON_FIELD_STRING) {
 *           printf("field %s has value %s\n",
 *                  yocton_field_name(f), yocton_field_value(f));
 *       }
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param obj  @ref yocton_object to read from.
 * @return     @ref yocton_field or NULL if there are no more fields to be
 *             read. NULL is also returned if an error occurs in parsing
 *             the input; @ref yocton_have_error should be used to
 *             distinguish the two. If a field is returned, it is only
 *             valid until the next field is read from the same object.
 */
struct yocton_field *yocton_next_field(struct yocton_object *obj);

/**
 * Get the type of a @ref yocton_field.
 *
 * See @ref yocton_next_field for an example of how this might be used.
 *
 * @param f  The field.
 * @return   Type of the field.
 */
enum yocton_field_type yocton_field_type(struct yocton_field *f);

/**
 * Get the name of a @ref yocton_field. Multiple fields of the same object
 * may have the same name. Encoding of the name depends on the encoding of
 * the input file.
 *
 * See @ref yocton_next_field for an example of how this might be used.
 *
 * @param f  The field.
 * @return   Name of the field. The returned string is only valid for the
 *           lifetime of the field itself.
 */
const char *yocton_field_name(struct yocton_field *f);

/**
 * Get the string value of a @ref yocton_field of type
 * @ref YOCTON_FIELD_STRING. It is an error to call this for a field that
 * is not of this type. Encoding of the string depends on the input file.
 *
 * See @ref yocton_next_field for an example of how this might be used.
 *
 * @param f  The field.
 * @return   String value of this field, or NULL if it is not a field of
 *           type @ref YOCTON_FIELD_STRING. The returned string is only
 *           valid for the lifetime of the field itself.
 */
const char *yocton_field_value(struct yocton_field *f);

/**
 * Get newly-allocated string value of a @ref yocton_field.
 *
 * Unlike @ref yocton_field_value, the returned value is a mutable string that
 * will survive beyond the lifetime of the field. It is the job of the caller
 * to free the string. Calling multiple times returns a newly-allocated string
 * each time.
 *
 * It is an error to call this for a field that is not of type @ref
 * YOCTON_FIELD_STRING. String encoding depends on the input file.
 *
 * It may be more convenient to use @ref YOCTON_FIELD_STRING which is a wrapper
 * around this function.
 *
 * @param f  The field.
 * @return   String value of this field, or NULL if it is not a field of
 *           type @ref YOCTON_FIELD_STRING, or if a memory allocation failure
 *           occurred.
 */
char *yocton_field_value_dup(struct yocton_field *f);

#define YOCTON_IF_FIELD(field, name, then) \
	do { \
		if (!strcmp(yocton_field_name(field), #name)) { \
			then \
		} \
	} while (0)

int yocton_reserve_array(struct yocton_field *f, void **array,
                         size_t nmemb, size_t size);

#define YOCTON_IF_ARRAY_FIELD(field, my_struct, name, name_len, then) \
	YOCTON_IF_FIELD(field, name, { \
		if (yocton_reserve_array(field, (void **) &((my_struct).name), \
		                         (my_struct).name_len, \
		                         sizeof((my_struct).name))) { \
			then \
		} \
	}

/**
 * Set the value of a string struct field if appropriate.
 *
 * If the Yocton field currently being parsed has the same name as the given
 * struct field, the struct field will be initialized to a newly-allocated
 * buffer containing a copy of the string value.
 *
 * If the string field has an existing value it will be freed. It is therefore
 * important that the field is initialized to NULL before this macro is used.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   struct s {
 *     char *bar;
 *   };
 *   struct s foo = {NULL};
 *   struct yocton_field *f;
 *
 *   while ((f = yocton_next_field(obj)) != NULL) {
 *       YOCTON_FIELD_STRING(f, foo, bar);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param field      Yocton field.
 * @param my_struct  C struct to initialize.
 * @param name       Name of field to initialize.
 */
#define YOCTON_FIELD_STRING(field, my_struct, name) \
	YOCTON_IF_FIELD(field, name, { \
		free((my_struct).name); \
		(my_struct).name = yocton_field_value_dup(field); \
	})

#define YOCTON_FIELD_STRING_ARRAY(field, my_struct, name, name_len) \
	YOCTON_IF_ARRAY_FIELD(field, my_struct, name, name_len, { \
		char *__v = yocton_field_value_dup(field); \
		if (__v) { \
			(my_struct).name[(my_struct).name_len] = __v; \
			++(my_struct).name_len; \
		} \
	})

/**
 * Get the inner object associated with a @ref yocton_field of type
 * @ref YOCTON_FIELD_OBJECT. It is an error to call this for a field that
 * is not of this type.
 *
 * Example of a function that recursively reads inner objects:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   void recurse_obj(struct yocton_object *obj) {
 *       struct yocton_field *f;
 *       while ((f = yocton_next_field(obj)) != NULL) {
 *           if (yocton_field_type(f) == YOCTON_FIELD_OBJECT) {
 *               printf("subobject %s\n", yocton_field_value(f));
 *               recurse_obj(yocton_field_inner(f));
 *           }
 *       }
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param f  The field.
 * @return   Inner @ref yocton_object, or NULL if the field is not of type
 *           @ref YOCTON_FIELD_OBJECT. The returned object is only only
 *           valid for the lifetime of the field itself.
 */
struct yocton_object *yocton_field_inner(struct yocton_field *f);

/**
 * Parse the field value as a signed integer.
 *
 * If the field value is not a valid integer of the given size, zero is
 * returned and an error is set.
 *
 * It may be more convenient to use @ref YOCTON_FIELD_INT which is a wrapper
 * around this function.
 *
 * @param f   The field.
 * @param n   Size of the expected field in bytes, eg. sizeof(uint16_t).
 * @return    The integer value, or zero if it cannot be parsed as an
 *            integer of that size. Although the return value is a long
 *            long type, it will always be in the range of an integer
 *            of the given size and can be safely cast to one.
 */
signed long long yocton_field_int(struct yocton_field *f, size_t n);

/**
 * Set the value of a signed integer struct field if appropriate.
 *
 * If the Yocton field currently being parsed has the same name as the given
 * struct field, the struct field will be initialized to a signed integer value
 * parsed from the Yocton field value. If the Yocton field value cannot be
 * parsed as a signed integer, it will be set to zero and an error is set.
 *
 * This will work with any kind of signed integer field, but the type of the
 * field must be provided.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   struct s {
 *     int bar;
 *   };
 *   struct s foo;
 *   struct yocton_field *f;
 *
 *   while ((f = yocton_next_field(obj)) != NULL) {
 *       YOCTON_FIELD_INT(f, foo, int, bar);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param field       Yocton field.
 * @param my_struct   C struct to initialize.
 * @param field_type  Type of the field, eg. `int` or `ssize_t`.
 * @param name        Name of field to initialize.
 */
#define YOCTON_FIELD_INT(field, my_struct, field_type, name) \
	YOCTON_IF_FIELD(field, name, { \
		(my_struct).name = (field_type) \
			yocton_field_int(field, sizeof(field_type)); \
	})

#define YOCTON_FIELD_INT_ARRAY(field, my_struct, field_type, name, name_len) \
	YOCTON_IF_ARRAY_FIELD(field, my_struct, name, name_len, { \
		(my_struct).name[(my_struct).name_len] = (field_type) \
			yocton_field_int(field, sizeof(field_type)); \
		++(my_struct).name_len; \
	})

/**
 * Parse the field value as a unsigned integer.
 *
 * If the field value is not a valid integer of the given size, zero is
 * returned and an error is set.
 *
 * It may be more convenient to use @ref YOCTON_FIELD_UINT which is a wrapper
 * around this function.
 *
 * @param f   The field.
 * @param n   Size of the expected field in bytes, eg. sizeof(uint16_t).
 * @return    The integer value, or zero if it cannot be parsed as an
 *            signed integer of that size. Although the return value is a
 *            long long type, it will always be in the range of an integer
 *            of the given size and can be safely cast to one.
 */
unsigned long long yocton_field_uint(struct yocton_field *f, size_t n);

/**
 * Set the value of a unsigned integer struct field if appropriate.
 *
 * If the Yocton field currently being parsed has the same name as the given
 * struct field, the struct field will be initialized to a unsigned integer
 * value parsed from the Yocton field value. If the Yocton field value cannot
 * be parsed as a unsigned integer, it will be set to zero and an error is set.
 *
 * This will work with any kind of unsigned integer field, but the type of the
 * field must be provided.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   struct s {
 *     unsigned int bar;
 *   };
 *   struct s foo;
 *   struct yocton_field *f;
 *
 *   while ((f = yocton_next_field(obj)) != NULL) {
 *       YOCTON_FIELD_INT(f, foo, unsigned int, bar);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param field       Yocton field.
 * @param my_struct   C struct to initialize.
 * @param field_type  Type of the field, eg. `unsigned int` or `size_t`.
 * @param name        Name of field to initialize.
 */
#define YOCTON_FIELD_UINT(field, my_struct, field_type, name) \
	YOCTON_IF_FIELD(field, name, { \
		(my_struct).name = (field_type) \
			yocton_field_uint(field, sizeof(field_type)); \
	})

#define YOCTON_FIELD_UINT_ARRAY(field, my_struct, field_type, name, name_len) \
	YOCTON_IF_ARRAY_FIELD(field, my_struct, name, name_len, { \
		(my_struct).name[(my_struct).name_len] = (field_type) \
			yocton_field_uint(field, sizeof(field_type)); \
		++(my_struct).name_len; \
	})

/**
 * Parse the field value as an enumeration.
 *
 * Enumeration values are assumed to be contiguous and start from zero.
 * values[e] gives the string representing enum value e. If the field
 * value is not found in the values array, an error is set.
 *
 * Note that the lookup of name to enum value is a linear scan so it is
 * relatively inefficient. If efficiency is concerned, an alternative
 * approach should be used (eg. a hash table).
 *
 * It may be more convenient to use @ref YOCTON_FIELD_ENUM which is a wrapper
 * around this function.
 *
 * @param f       The field.
 * @param values  Pointer to a NULL-terminated array of strings representing
 *                enum values. values[e] is a string representing enum
 *                value e.
 * @return        The identified enum value. If not found, an error is set
 *                and zero is returned.
 */
unsigned int yocton_field_enum(struct yocton_field *f, const char **values);

/**
 * Set the value of an enum struct field if appropriate.
 *
 * If the Yocton field currently being parsed has the same name as the given
 * struct field, the struct field will be initialized to an enum value that
 * matches a name from the given list.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   enum e { FIRST, SECOND, THIRD };
 *   const char *enum_values[] = {"FIRST", "SECOND", "THIRD", NULL};
 *   struct s {
 *     enum e bar;
 *   };
 *   struct s foo;
 *   struct yocton_field *f;
 *
 *   while ((f = yocton_next_field(obj)) != NULL) {
 *       YOCTON_FIELD_ENUM(f, foo, bar, enum_values);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param field      Yocton field.
 * @param my_struct  C struct to initialize.
 * @param name       Name of field to initialize.
 * @param values     NULL-terminated array of strings representing enum values
 *                   (same as values parameter to @ref yocton_field_enum).
 */
#define YOCTON_FIELD_ENUM(field, my_struct, name, values) \
	YOCTON_IF_FIELD(field, name, { \
		(my_struct).name = yocton_field_enum(field, values); \
	})

#define YOCTON_FIELD_ENUM_ARRAY(field, my_struct, name, name_len, values) \
	YOCTON_IF_ARRAY_FIELD(field, my_struct, name, name_len, { \
		(my_struct).name[(my_struct).name_len] = \
			yocton_field_enum(field, values) \
		++(my_struct).name_len; \
	})

#ifdef __cplusplus
}
#endif

#endif /* #ifndef YOCTON_H */

