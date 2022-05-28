
## Data model

A Yocton document consists of an object. Every object has a number of fields.
Each field has a name (string) and a value. The value is either a string or
an inner object. For example:
```
field_name1: "hello"
string_field: "world"
object_field {
  inner_field1: "foo"
  inner_field2: "bar"
}
```
Strings have two representations:

1. The first is the traditional C-style syntax, with surrounding double-quotes
   and backslash-escaping for special characters.
1. The second is "bare string" syntax. The surrounding quotes can be omitted
   if the string consists entirely of the following characters: alphanumeric
   characters, underscore, plus, minus, period.

Taking into account the above two representations, here is another example:
```c
// This is a comment
my_integer: 12345
my_float: 1.234e-10
my_boolean: true
error: EAGAIN
temperature_map {
  "ice cream": -18C
  "room temperature": 20C
}
ip_address: 192.168.4.10
my_list {
  element: 123
  element: 456
  element: 789
}
```

## API

The Yocton API is a "pull parser" that is designed to work nicely for
statically compiled languages like C.

/* TODO: Code example here. */

## What Yocton doesn't have

Compared with JSON:

* Arrays: since there is no restriction that an object can only have one field
  with any particular name, repeated values can be represented as repeated
  fields with the same name.
* Number types: strings can be used to store a representation of a number.
  The set of characters allowed in bare strings includes alphanumeric
  characters, period, plus and minus, allowing natural representation of
  numbers in almost any format - integer, decimal, hexadecimal,
  exponential/scientific, etc. without the need for quotes.
* Booleans: strings can be used to store "true" or "false". The bare string
  syntax allows a natural representation.
* Null: strings can be used to store "null". The bare string syntax allows a
  natural representation.

## What's with the name?

yocto- is the Si unit prefix for a factor of 10^−24. The -on suffix is the
same as in JSON where it stands for Object Notation. So it's a very small
object notation.

