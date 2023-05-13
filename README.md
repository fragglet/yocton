
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

As seen in this example:

* The symbol notation allows a variety of different
formats to be concisely represented, including C symbol names, floating-point
numbers, IP addresses or even temperatures.

* There is no requirement that a particular object can only have one field of a particular name; this provides a way of representing lists even though there is no special syntax for them. 

## Strings

Strings are represented using a subset of the familiar C syntax with the following escape sequences:

* `\n`: Newline
* `\t`: Tab
* `\"`: Double quotes (")
* `\\`: Literal backslash (\) 
* `\xDD`: ASCII control character by hexadecimal number, in range 01h-1Fh (characters outside this range are not valid). 

Control characters are not valid in bare form inside a string and must be escaped. There is no way of representing a NUL character (ASCII 00h); this is a design decision. Strings are intended for storing textual data; binary data can be stored using an encoding like base64 if necessary. 

## Unicode

Yocton is "UTF-8 friendly" but does not include special support for
Unicode. This is in keeping with its typeless format - strings are really
just arbitrary sequences of bytes. The encoding ought to be UTF-8 nowadays,
but could also be a different encoding like ISO-8859-1 (although not
some multi-byte formats like Shift-JIS unfortunately, for
[technical reasons](https://en.wikipedia.org/wiki/Shift_JIS#Description:~:text=0x5C%20byte%20will%20cause%20problems)).
There isn't any validation of the input encoding that forces you to use UTF-8
or any other format, though UTF-8 is strongly recommended.

As a result, Yocton does not include any special support for Unicode; most
notably it does not include support for the `\u` or `\U` escape sequences that
JSON has. This is deliberate; simply encode your file as UTF-8 and put the
characters that you want in the file. These days it is reasonable to assume
UTF-8 as an encoding, all modern text editors support it, and there is no need
to encode everything in plain ASCII.

The one minor piece of special handling for UTF-8 is that Yocton recognizes
(and ignores) the UTF-8
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

