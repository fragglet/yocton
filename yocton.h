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

/** Type of a @ref yocton_prop. */
enum yocton_prop_type {
	/**
	 * Property that has a string value. @ref yocton_prop_value can be
	 * used to get the value.
	 */
	YOCTON_PROP_STRING,

	/**
	 * Property that has an object value. @ref yocton_prop_inner can be
	 * used to read the inner object.
	 */
	YOCTON_PROP_OBJECT,
};

struct yocton_object;
struct yocton_prop;

#ifdef __DOXYGEN__

/**
 * The object is the main abstraction of the Yocton format. Each object
 * can have multiple properties (@ref yocton_prop), which can themselves
 * contain more objects.
 */
typedef struct yocton_object yocton_object;

/**
 * An object can have multiple properties. Each property has a name which is
 * always a string. It also always has a value, which is either a string
 * (@ref YOCTON_PROP_STRING) or an object (@ref YOCTON_PROP_OBJECT). Properties
 * have a very limited lifetime and are only valid until @ref yocton_next_prop
 * is called to read the next property.
 */
typedef struct yocton_prop yocton_prop;

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
 * once no more data is returned from obj (ie. when @ref yocton_next_prop
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
 * Read the next property of an object.
 *
 * Example that prints the names and values of all string properties:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   struct yocton_prop *p;
 *   while ((p = yocton_next_prop(obj)) != NULL) {
 *       if (yocton_prop_type(p) == YOCTON_PROP_STRING) {
 *           printf("property %s has value %s\n",
 *                  yocton_prop_name(p), yocton_prop_value(p));
 *       }
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param obj  @ref yocton_object to read from.
 * @return     @ref yocton_prop or NULL if there are no more properties to be
 *             read. NULL is also returned if an error occurs in parsing
 *             the input; @ref yocton_have_error should be used to
 *             distinguish the two. If a property is returned, it is only
 *             valid until the next call to yocton_next_prop.
 */
struct yocton_prop *yocton_next_prop(struct yocton_object *obj);

/**
 * Get the type of a @ref yocton_prop.
 *
 * See @ref yocton_next_prop for an example of how this might be used.
 *
 * @param p  The property.
 * @return   Type of the property.
 */
enum yocton_prop_type yocton_prop_type(struct yocton_prop *p);

/**
 * Get the name of a @ref yocton_prop. Multiple properties of the same object
 * may have the same name. Encoding of the name depends on the encoding of
 * the input file.
 *
 * See @ref yocton_next_prop for an example of how this might be used.
 *
 * @param p  The property.
 * @return   Name of the property. The returned string is only valid for the
 *           lifetime of the property itself.
 */
const char *yocton_prop_name(struct yocton_prop *p);

/**
 * Get the string value of a @ref yocton_prop of type
 * @ref YOCTON_PROP_STRING. It is an error to call this for a property that
 * is not of this type. Encoding of the string depends on the input file.
 *
 * See @ref yocton_next_prop for an example of how this might be used.
 *
 * @param p  The property.
 * @return   String value of this property, or NULL if it is not a property of
 *           type @ref YOCTON_PROP_STRING. The returned string is only
 *           valid for the lifetime of the property itself.
 */
const char *yocton_prop_value(struct yocton_prop *p);

/**
 * Get newly-allocated copy of a property value.
 *
 * Unlike @ref yocton_prop_value, the returned value is a mutable string that
 * will survive beyond the lifetime of the property. It is the responsibility
 * of the caller to free the string. Calling multiple times returns a
 * newly-allocated string each time.
 *
 * It is an error to call this for a property that is not of type @ref
 * YOCTON_PROP_STRING. String encoding depends on the input file.
 *
 * It may be more convenient to use @ref YOCTON_FIELD_STRING which is a wrapper
 * around this function.
 *
 * @param p  The property.
 * @return   String value of this property, or NULL if it is not a property of
 *           type @ref YOCTON_PROP_STRING, or if a memory allocation failure
 *           occurred.
 */
char *yocton_prop_value_dup(struct yocton_prop *p);

#define YOCTON_IF_PROP(property, name, then) \
	do { \
		if (!strcmp(yocton_prop_name(property), #name)) { \
			then \
		} \
	} while (0)

int yocton_reserve_array(struct yocton_prop *p, void **array,
                         size_t nmemb, size_t size);

#define YOCTON_IF_ARRAY_PROP(prop, propname, varname, varname_len, then) \
	YOCTON_IF_PROP(prop, propname, { \
		if (yocton_reserve_array(prop, (void **) &(varname), \
		                         varname_len, \
		                         sizeof(*(varname)))) { \
			then \
		} \
	})

#define YOCTON_VAR_STRING(prop, propname, varname) \
	YOCTON_IF_PROP(prop, propname, { \
		free(varname); \
		varname = yocton_prop_value_dup(prop); \
	})

#define YOCTON_VAR_STRING_ARRAY(prop, propname, varname, varname_len) \
	YOCTON_IF_ARRAY_PROP(prop, propname, varname, varname_len, { \
		char *__v = yocton_prop_value_dup(prop); \
		if (__v) { \
			(varname)[varname_len] = __v; \
			++(varname_len); \
		} \
	})

/**
 * Set the value of a string struct field if appropriate.
 *
 * If the property currently being parsed has the same name as the given field,
 * the field will be initialized to a newly-allocated buffer containing a copy
 * of the string value.
 *
 * If the field has an existing value it will be freed. It is therefore
 * important that the field is initialized to NULL before the first time this
 * macro is used to set it.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   struct s {
 *       char *bar;
 *   };
 *   struct s foo = {NULL};
 *   struct yocton_prop *p;
 *
 *   while ((p = yocton_next_prop(obj)) != NULL) {
 *       YOCTON_FIELD_STRING(p, foo, bar);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param prop       Property.
 * @param my_struct  C struct to initialize.
 * @param name       Name of field to initialize.
 */
#define YOCTON_FIELD_STRING(prop, my_struct, name) \
	YOCTON_VAR_STRING(prop, name, (my_struct).name)

#define YOCTON_FIELD_STRING_ARRAY(prop, my_struct, name, name_len) \
	YOCTON_VAR_STRING_ARRAY(prop, name, (my_struct).name, \
	                        (my_struct).name_len)

/**
 * Get the inner object associated with a @ref yocton_prop of type
 * @ref YOCTON_PROP_OBJECT. It is an error to call this for a property that
 * is not of this type.
 *
 * Example of a function that recursively reads inner objects:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   void recurse_obj(struct yocton_object *obj) {
 *       struct yocton_prop *p;
 *       while ((p = yocton_next_prop(obj)) != NULL) {
 *           if (yocton_prop_type(p) == YOCTON_PROP_OBJECT) {
 *               printf("subobject %s\n", yocton_prop_value(p));
 *               recurse_obj(yocton_prop_inner(p));
 *           }
 *       }
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param p  The property.
 * @return   Inner @ref yocton_object, or NULL if the property is not of type
 *           @ref YOCTON_PROP_OBJECT. The returned object is only only
 *           valid for the lifetime of the property itself.
 */
struct yocton_object *yocton_prop_inner(struct yocton_prop *p);

/**
 * Parse the property value as a signed integer.
 *
 * If the property value is not a valid integer of the given size, zero is
 * returned and an error is set.
 *
 * It may be more convenient to use @ref YOCTON_FIELD_INT which is a wrapper
 * around this function.
 *
 * @param p   The property.
 * @param n   Size of the expected property in bytes, eg. sizeof(uint16_t).
 * @return    The integer value, or zero if it cannot be parsed as an
 *            integer of that size. Although the return value is a long
 *            long type, it will always be in the range of an integer
 *            of the given size and can be safely cast to one.
 */
signed long long yocton_prop_int(struct yocton_prop *p, size_t n);

#define YOCTON_VAR_INT(prop, propname, var_type, varname) \
	YOCTON_IF_PROP(prop, propname, { \
		varname = (var_type) yocton_prop_int(prop, sizeof(var_type)); \
	})

#define YOCTON_VAR_INT_ARRAY(prop, propname, var_type, varname, varname_len) \
	YOCTON_IF_ARRAY_PROP(prop, propname, varname, varname_len, { \
		(varname)[varname_len] = (var_type) \
			yocton_prop_int(prop, sizeof(var_type)); \
		++(varname_len); \
	})

/**
 * Set the value of a signed integer struct field if appropriate.
 *
 * If the property currently being parsed has the same name as the given struct
 * field, the field will be initialized to a signed integer value parsed from
 * the property value. If the property value cannot be parsed as a signed
 * integer, the field will be set to zero and an error set.
 *
 * This will work with any kind of signed integer field, but the type of the
 * field must be provided.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   struct s {
 *       int bar;
 *   };
 *   struct s foo;
 *   struct yocton_prop *p;
 *
 *   while ((p = yocton_next_prop(obj)) != NULL) {
 *       YOCTON_FIELD_INT(p, foo, int, bar);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param prop           Property.
 * @param my_struct      C struct to initialize.
 * @param field_type     Type of the field, eg. `int` or `ssize_t`.
 * @param name           Name of field to initialize.
 */
#define YOCTON_FIELD_INT(prop, my_struct, field_type, name) \
	YOCTON_VAR_INT(prop, name, field_type, (my_struct).name)

#define YOCTON_FIELD_INT_ARRAY(prop, my_struct, field_type, name, name_len) \
	YOCTON_VAR_INT_ARRAY(prop, name, field_type, (my_struct).name, \
	                     (my_struct).name_len)

/**
 * Parse the property value as an unsigned integer.
 *
 * If the property value is not a valid integer of the given size, zero is
 * returned and an error is set.
 *
 * It may be more convenient to use @ref YOCTON_FIELD_UINT which is a wrapper
 * around this function.
 *
 * @param p   The property.
 * @param n   Size of the expected property in bytes, eg. sizeof(uint16_t).
 * @return    The integer value, or zero if it cannot be parsed as an
 *            signed integer of that size. Although the return value is a
 *            long long type, it will always be in the range of an integer
 *            of the given size and can be safely cast to one.
 */
unsigned long long yocton_prop_uint(struct yocton_prop *p, size_t n);

#define YOCTON_VAR_UINT(prop, propname, var_type, varname) \
	YOCTON_IF_PROP(prop, propname, { \
		varname = (var_type) \
			yocton_prop_uint(prop, sizeof(var_type)); \
	})

#define YOCTON_VAR_UINT_ARRAY(prop, propname, var_type, varname, varname_len) \
	YOCTON_IF_ARRAY_PROP(prop, propname, varname, varname_len, { \
		(varname)[varname_len] = (var_type) \
			yocton_prop_uint(prop, sizeof(var_type)); \
		++(varname_len); \
	})

/**
 * Set the value of an unsigned integer struct field if appropriate.
 *
 * If the property currently being parsed has the same name as the given struct
 * field, the field will be initialized to an unsigned integer value parsed
 * from the property value. If the property value cannot be parsed as an
 * unsigned integer, the field will be set to zero and an error set.
 *
 * This will work with any kind of unsigned integer property, but the type of the
 * property must be provided.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   struct s {
 *       unsigned int bar;
 *   };
 *   struct s foo;
 *   struct yocton_prop *p;
 *
 *   while ((p = yocton_next_prop(obj)) != NULL) {
 *       YOCTON_FIELD_UINT(p, foo, unsigned int, bar);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param prop        Property.
 * @param my_struct   C struct to initialize.
 * @param field_type  Type of the field, eg. `unsigned int` or `size_t`.
 * @param name        Name of field to initialize.
 */
#define YOCTON_FIELD_UINT(prop, my_struct, field_type, name) \
	YOCTON_VAR_UINT(prop, name, field_type, (my_struct).name)

#define YOCTON_FIELD_UINT_ARRAY(prop, my_struct, field_type, name, name_len) \
	YOCTON_VAR_UINT_ARRAY(prop, name, field_type, (my_struct).name, \
	                      (my_struct).name_len)

/**
 * Parse the property value as an enumeration.
 *
 * Enumeration values are assumed to be contiguous and start from zero.
 * values[e] gives the string representing enum value e. If the property
 * value is not found in the values array, an error is set.
 *
 * Note that the lookup of name to enum value is a linear scan so it is
 * relatively inefficient. If efficiency is concerned, an alternative
 * approach should be used (eg. a hash table).
 *
 * It may be more convenient to use @ref YOCTON_FIELD_ENUM which is a wrapper
 * around this function.
 *
 * @param p       The property.
 * @param values  Pointer to a NULL-terminated array of strings representing
 *                enum values. values[e] is a string representing enum
 *                value e.
 * @return        The identified enum value. If not found, an error is set
 *                and zero is returned.
 */
unsigned int yocton_prop_enum(struct yocton_prop *p, const char **values);

#define YOCTON_VAR_ENUM(prop, propname, varname, values) \
	YOCTON_IF_PROP(prop, propname, { \
		(varname) = yocton_prop_enum(prop, values); \
	})

#define YOCTON_VAR_ENUM_ARRAY(prop, propname, varname, varname_len, values) \
	YOCTON_IF_ARRAY_PROP(prop, propname, varname, varname_len, { \
		(varname)[varname_len] = yocton_prop_enum(prop, values); \
		++(varname_len); \
	})

/**
 * Set the value of an enum struct field if appropriate.
 *
 * If the property currently being parsed has the same name as the given struct
 * field, the field will be initialized to an enum value that matches a name
 * from the given list.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   enum e { FIRST, SECOND, THIRD };
 *   const char *enum_values[] = {"FIRST", "SECOND", "THIRD", NULL};
 *   struct s {
 *     enum e bar;
 *   };
 *   struct s foo;
 *   struct yocton_prop *p;
 *
 *   while ((p = yocton_next_prop(obj)) != NULL) {
 *       YOCTON_FIELD_ENUM(p, foo, bar, enum_values);
 *   }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param prop       Property.
 * @param my_struct  C struct to initialize.
 * @param name       Name of field to initialize.
 * @param values     NULL-terminated array of strings representing enum values
 *                   (same as values parameter to @ref yocton_prop_enum).
 */
#define YOCTON_FIELD_ENUM(prop, my_struct, name, values) \
	YOCTON_VAR_ENUM(prop, name, (my_struct).name, values)

#define YOCTON_FIELD_ENUM_ARRAY(prop, my_struct, name, name_len, values) \
	YOCTON_VAR_ENUM_ARRAY(prop, name, (my_struct).name, \
	                      (my_struct).name_len, values)

#ifdef __cplusplus
}
#endif

#endif /* #ifndef YOCTON_H */

