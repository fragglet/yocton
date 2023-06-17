/**

@page parse_api Yocton Parse API

The Yocton API is a pull parser. The APIs for other formats tend to be document
based - where serialized data is deserialized into a document object after
deserialization is complete. With a pull parser, it is up to the caller to read
the deserialized data one item at a time. This avoids the need for either
autogenerated code (as with protobufs) or complicated APIs (JSON's data model
fits well with object oriented languages, but less so with C).

This model of deserialization has been chosen with a particular approach in
mind to using deserialized data to populate data structures.
It is assumed that when deserialized, most Yocton objects will correspond to C
structs, and object properties will correspond to C struct fields. Here's a
simple example of how a struct might be read and populated; the example struct
here is a minimal one containing a single string field:

```c
struct foo {
    char *bar;
};

struct foo *read_foo(struct yocton_object *obj) {
  struct foo *result = calloc(1, sizeof(struct foo));
  struct yocton_prop *p;

  while ((p = yocton_next_prop(obj)) != NULL) {
    if (!strcmp(yocton_prop_name(p), "bar")) {
      result->bar = yocton_prop_value_dup(p);
    }
  }
  return result;
}
```

While this is relatively easy to understand, it looks quite verbose. It is
therefore important to note that there are convenience functions and macros to
make things much simpler; see below.

Yocton is a recursive format where objects can also contain other objects. The
assumption is that a subobject likely corresponds to a field with a struct
type. For example:

```c
struct baz {
  struct foo *my_foo;
};

struct qux {
  struct baz my_baz;
};
```

might be serialized in Yocton format as:

```js
my_baz {
  my_foo {
    bar: "hello world!"
  }
}
```

When subobjects are mapped to struct types in this way, a function can be
written that reads and populates each type of struct. In the examples above,
`read_foo()` might be complemented with `read_baz()` and `read_qux()`
functions. This makes for clear and readable deserialization code; the approach
also means that the individual functions can be tested in isolation.

## Struct population

Yocton properties can contain arbitrary strings, the contents of which are open
to interpretation by the code that is reading them. In practice though, the
values are often likely to be one of several common base types which every C
programmer is familiar with. There are convenience functions to help parse
values into these types:

| Function                | Purpose
|-------------------------|----------------------|
| yocton_prop_int()       | Parse value as a signed integer. Works with all integer types, performs bounds checking, etc. |
| yocton_prop_uint()      | Parse value as an unsigned integer. Works with all unsigned integer types, performs bounds checking, etc. |
| yocton_prop_value_dup() | Returns the value as a plain, freshly allocated string, performing the appropriate checking for memory allocation failure.  Useful for populating string fields. |
| yocton_prop_enum()      | Parse value as an enum value by looking it up in an array of strings. Useful for representing values symbolically rather than just as raw integers. |

While these functions are useful, in most cases it is more convenient to use
the preprocessor macros which are specifically intended for populating struct
fields.

| Type                   | Variable                     | Struct field                   |
|------------------------|------------------------------|--------------------------------|
| String                 | @ref YOCTON_VAR_STRING       | @ref YOCTON_FIELD_STRING       |
| Signed integer         | @ref YOCTON_VAR_INT          | @ref YOCTON_FIELD_INT          |
| Unsigned integer       | @ref YOCTON_VAR_UINT         | @ref YOCTON_FIELD_UINT         |
| Enum                   | @ref YOCTON_VAR_ENUM         | @ref YOCTON_FIELD_ENUM         |

For the following example we will populate the `foo` struct:

```c
enum e { FIRST, SECOND, THIRD };
struct foo {
  enum e enum_value;
  int signed_value;
  unsigned int unsigned_value;
  char *string_value;
};
```

The input can be parsed and the struct populated using the following code:

```c
const char *enum_names[] = {"FIRST", "SECOND", "THIRD", NULL};
struct foo my_struct = {0, 0, 0, NULL};
struct yocton_prop *p;
while ((p = yocton_next_prop(obj)) != NULL) {
  YOCTON_FIELD_ENUM(p, my_struct, enum_value, enum_names);
  YOCTON_FIELD_INT(p, my_struct, int, signed_value) ;
  YOCTON_FIELD_UINT(p, my_struct, unsigned int, unsigned_value);
  YOCTON_FIELD_STRING(p, my_struct, string_value);
}
```

An important point to note is that these macros are internally designed to
provide a simple and convenient API, not for efficiency. If performance is
essential or becomes a bottleneck, it may be preferable to avoid using these
macros.

## Representing lists

The Yocton format has no special way of representing lists. Since property
names do not have to be unique, it is simple enough to represent a list using
multiple properties with the same name.

As with the previous example that described how to populate struct fields,
convenience macros also exist for constructing arrays. The main difference is
that an extra field is needed to store the array length.

| Type                   | Variable                     | Struct field                   |
|------------------------|------------------------------|--------------------------------|
| String array           | @ref YOCTON_VAR_STRING_ARRAY | @ref YOCTON_FIELD_STRING_ARRAY |
| Signed integer array   | @ref YOCTON_VAR_INT_ARRAY    | @ref YOCTON_FIELD_INT_ARRAY    |
| Unsigned integer array | @ref YOCTON_VAR_UINT_ARRAY   | @ref YOCTON_FIELD_UINT_ARRAY   |
| Enum array             | @ref YOCTON_VAR_ENUM_ARRAY   | @ref YOCTON_FIELD_ENUM_ARRAY   |
| Generic array          | @ref YOCTON_VAR_ARRAY        | @ref YOCTON_FIELD_ARRAY        |

For example:

```c
enum e { FIRST, SECOND, THIRD };
struct bar {
  enum e *enum_values;
  size_t num_enum_values;
  int *signed_values;
  size_t num_signed_values;
  unsigned int *unsigned_values;
  size_t num_unsigned_values;
  char **string_values;
  size_t num_string_values;
};
```

The parsing code looks very similar to the previous example:

```c
const char *enum_names[] = {"FIRST", "SECOND", "THIRD", NULL};
struct bar my_struct = {NULL, 0, NULL, 0, NULL, 0, NULL, 0};
struct yocton_prop *p;
while ((p = yocton_next_prop(obj)) != NULL) {
  YOCTON_FIELD_ENUM_ARRAY(p, my_struct, enum_values, num_enum_values, enum_names);
  YOCTON_FIELD_INT_ARRAY(p, my_struct, int, signed_values, num_signed_values);
  YOCTON_FIELD_UINT_ARRAY(p, my_struct, unsigned int, unsigned_values, num_unsigned_values);
  YOCTON_FIELD_STRING_ARRAY(p, my_struct, string_values, num_string_values);
}
```

While these macros are convenient for building arrays of base types, often it
is preferable to construct arrays of more complex data structures. The above
macros are built on another macro named `YOCTON_FIELD_ARRAY` which can be used
to achieve this goal. `YOCTON_FIELD_ARRAY` does the following:

1. Check if the name of the property matches a particular name.
2. If the name matches, an array pointer is reallocated to allocate space for a
new element at the end of the array.
3. An arbitrary block of code is executed that can (optionally) populate the
new array element.

Here is an example:

```c
struct foo {
  struct bar *elements;
  size_t num_elements;
};
struct foo my_struct;
struct yocton_prop *p;

while ((p = yocton_next_prop(obj)) != NULL) {
  YOCTON_FIELD_ARRAY(p, my_struct, elements, num_elements, {
    populate_element(yocton_prop_inner(p),
             &my_struct.elements[my_struct.num_elements]);
    my_struct.num_elements++;
  });
}
```


## Error handling

There are many different types of error that can occur while parsing a Yocton
file. For example:

* Syntax error
* Memory allocation failure
* Property has unexpected type (string for an object property, or vice versa)
* Invalid property value (eg. overflow when parsing an integer value)
* Violation of a user-provided constraint

Continual checking for error conditions can make for complicated code. The
Yocton API instead adopts an "error state" mechanism for error reporting. Write
your code assuming success, and at the end, check once if an error occurred.

Here's how this works in practice: most parsing code involves continually
calling `yocton_next_prop()` to read new properties from the file. If an error
condition is reached, this function will stop returning any more properties. In
effect it is like reaching the end of file. So when "end of file" is reached,
simply check if an error occurred or whether the document was successfully
parsed.

Here is a simple example of what this might look like:

```c
// Returns true if file was successfully parsed:
bool parse_config_file(const char *filename, struct config_data *cfg)
{
  FILE *fs;
  struct yocton_object *obj;
  const char *error_msg;
  int lineno;
  bool success;

  fs = fopen(filename, "r");
  if (fs == NULL) {
    return false;
  }

  obj = yocton_read_from(fs);
  parse_config_toplevel(obj, cfg);

  success = !yocton_have_error(obj, &lineno, &error_msg);
  if (!success) {
    fprintf(stderr, "Error in parsing config:\n%s:%d:%s\n",
            filename, lineno, error_msg);
  }

  yocton_free(obj);
  fclose(fs);
  return success;
}
```

Some of the API functions will also trigger the error state. It may be tempting
to add extra checks in your code to avoid this happening, but it is better that
you do not. If an error is triggered in this way, it is likely that it is due
to an error in the file being parsed. Your API calls implicitly document the
expected format of the input file. If the file does not conform to that format,
it is the file that is wrong, not your code.

An example may be illustrative. Suppose your Yocton files contain a property
called `name` which is expected to have a string value. If the property has an
object value instead, a call to `yocton_prop_value()` to get the expected
string value will trigger the error state. That is not a misuse of the API;
your code is implicitly indicating that a string was expected, and the input is
therefore erroneous. The line number where the error occurred is logged, just
the same as if the file itself was syntactically incorrect.

*/
