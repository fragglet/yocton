
Yocton is a minimalist, typeless object notation, intended to fit nicely
with the C programming language and intended for situations where the features
of other formats such as JSON or protocol buffers are not necessary.

![Captain Yocton](cpt-yocton.png)

The mascot for Yocton is Captain Yocton, who is a tiny alien superhero.

## Data model

A Yocton document consists of an object. Every object has a number of fields.
Each field has a name and either a string value or an inner object. For example:
```
field_name1: "hello"
string_field: "world"
object_field {
  inner_field1: "foo"
  inner_field2: "bar"
}
```
Strings can be represented in two different ways:

1. The first is the traditional C-style syntax, with surrounding double-quotes
   and backslash-escaping for special characters.
1. The second is "symbol" format. The surrounding quotes can be omitted
   if the string matches `^[A-Za-z0-9_+-\.]+$`, or in plain English, if every
   character is either alphanumeric, or one of underscore, plus, minus,
   or period.

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

As seen in the example, the symbol notation allows a variety of different
formats to be represented, including C symbol names, floating-point
numbers, IP addresses or even temperatures.

## Unicode

Yocton is "UTF-8 friendly" but does not include special support for
Unicode. This is in keeping with its typeless format - strings are really
just arbitrary sequences of bytes which can contain any kind of binary data
(UTF-8, ASCII with extended code pages, or even other Unicode encodings).
This also means that it can be used to contain any arbitrary binary data,
such as image files, encrypted data, or Doom WAD files.

However, Yocton is intended mainly for encoding textual data and UTF-8 is
the recommended encoding to use to use for this purpose. You will find that
it fits naturally with the format - simply encode the input file as UTF-8
and interpret any strings returned from the API as UTF-8, and everything
should work as intended. Yocton also recognizes (and ignores) the UTF-8
[BOM](https://en.wikipedia.org/wiki/Byte_order_mark), ensuring that any
valid UTF-8 input file should be parsed correctly.

## API

The Yocton API is a "pull parser" that is designed to work nicely for
statically compiled languages like C.

/* TODO: Code example here. */

## What Yocton doesn't have

Compared with JSON:

* Types: Yocton is ["stringly typed"](https://wiki.c2.com/?StringlyTyped),
  only providing a basic hierarchical structure for strings to be arranged
  in. It is up to the programmer to interpret the content of these strings,
  placing some additional burden not found with other formats.
* Arrays: since there is no restriction that an object can only have one field
  with any particular name, repeated values can be represented as repeated
  fields with the same name. An alternative is an object where sequential
  numbers are used as field names.
* Number types: strings can be used to store a representation of a number.
  The set of characters allowed in symbols includes alphanumeric
  characters, period, plus and minus, allowing natural representation of
  numbers in almost any format - integer, decimal, hexadecimal,
  exponential/scientific, etc. without the need for quotes.
* Booleans: strings can be used to store the word "true" or "false". The
  symbol format allows a natural representation.
* Null: strings can be used to store the word "null". The symbol format
  allows a natural representation.

## What's with the name?

yocto- is the Si unit prefix for a factor of 10^−24. The -on suffix is the
same as in JSON where it stands for Object Notation. So it's a very small
object notation.

