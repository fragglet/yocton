#!/usr/bin/env python
#
# Copyright (c) 2023, Simon Howard
#
# Permission to use, copy, modify, and/or distribute this software
# for any purpose with or without fee is hereby granted, provided
# that the above copyright notice and this permission notice appear
# in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
# WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
# AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
# NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

from __future__ import unicode_literals, print_function, generators
import collections
import io
import re

VALID_SYMBOL_RE = re.compile(r"^[a-zA-Z0-9_\-\+\.]+$")

TOKEN_STRING = 0
TOKEN_COLON = 1
TOKEN_OPEN_BRACE = 2
TOKEN_CLOSE_BRACE = 3
TOKEN_EOF = 4

ASCII_X = ord('x')
ASCII_QUOTES = ord('"')
ASCII_SLASH = ord('/')
ASCII_BACKSLASH = ord('\\')
ASCII_NEWLINE = ord('\n')
ASCII_OPEN_BRACE = ord('{')
ASCII_CLOSE_BRACE = ord('}')
ASCII_COLON = ord(':')
SIMPLE_ESCAPES = { ord('n'): ASCII_NEWLINE, ord('t'): ord('\t'),
                   ASCII_QUOTES: ASCII_QUOTES,
                   ASCII_BACKSLASH: ASCII_BACKSLASH }
REVERSE_ESCAPES = { "\n": "\\n", "\t": "\\t", '"': "\\\"", "\\": "\\\\" }
BOM = 0xfeff
UTF8_BOM = b'\xef\xbb\xbf'
ERROR_EOF = "unexpected EOF"

def is_symbol_char(c):
	return VALID_SYMBOL_RE.match("%c" % c) is not None

class InStream(object):
	def __init__(self, instream):
		self.instream = instream
		self.buf = ''
		self.buf_index = 0
		self.lineno = 1
		self.root = None
		self.binary = False
		self.error = False

	def syntax_assert(self, error_if_false, error_message="syntax error",
	                  *args):
		if not error_if_false:
			self.error = True
			raise SyntaxError(error_message % args)

	def refill_buffer(self):
		"""Refill read buffer if it's empty; returns False for EOF."""
		if self.buf_index < len(self.buf):
			return True
		self.buf = self.instream.read(128)
		if len(self.buf) == 0:
			return False
		self.binary = not isinstance(self.buf, str)
		self.buf_index = 0
		return True

	def peek_next_char(self):
		"""Inspect the next input character without advancing.

		Returns Unicode or byte value representing character.
		"""
		if not self.refill_buffer():
			return False, None
		result = self.buf[self.buf_index]
		if not self.binary:
			result = ord(result)
		return True, result

	def read_next_char(self):
		"""Read next input character and advance.

		Returns integer representing next character.
		"""
		ok, c = self.peek_next_char()
		self.syntax_assert(ok, ERROR_EOF)
		self.buf_index += 1
		return c

	def read_escape_sequence(self):
		c = self.read_next_char()
		esc = SIMPLE_ESCAPES.get(c)
		if esc is not None:
			return esc
		self.syntax_assert(c == ASCII_X, "unknown string escape: \\%c", c)
		try:
			d1 = self.read_next_char()
			d2 = self.read_next_char()
			c = int("%c%c" % (d1, d2), 16)
		except ValueError:
			self.syntax_assert(False,
				"\\x sequence must be followed "
				"by two hexadecimal characters")
		self.syntax_assert(c != 0,
			"NUL byte not allowed in \\x escape sequence.")
		self.syntax_assert(c < 0x20,
			"\\x escape sequence can only be used for "
			"control characters (ASCII 0x01-0x1f range)")
		return c

	def to_string(self, s):
		if self.binary:
			return bytes(s)
		else:
			return ''.join(chr(c) for c in s)

	def read_string(self):
		result_chars = []
		while True:
			c = self.read_next_char()
			if c == ASCII_QUOTES:
				break
			elif c == ASCII_BACKSLASH:
				c = self.read_escape_sequence()
			else:
				self.syntax_assert(c >= 0x20,
					'control character not allowed '
					'inside string (ASCII char 0x%02x)', c)
			result_chars.append(c)
		return self.to_string(result_chars)

	def read_symbol(self, first):
		self.syntax_assert(is_symbol_char(first),
			'unknown token: not valid symbol character')

		result_chars = [first]
		while True:
			ok, next_c = self.peek_next_char()
			if not ok or not is_symbol_char(next_c):
				# EOF is explicitly okay here
				break
			self.read_next_char()
			result_chars.append(next_c)

		return self.to_string(result_chars)

	def do_read_next_token(self):
		"""The actual logic of read_next_token()."""
		# Skip past spaces, including comments.
		while True:
			ok, _ = self.peek_next_char()
			if not ok:
				return TOKEN_EOF, None
			c = self.read_next_char()
			if c == ASCII_SLASH:
				self.syntax_assert(
					self.read_next_char() == ASCII_SLASH)
				while c != ASCII_NEWLINE:
					ok, c = self.peek_next_char()
					if not ok:
						return TOKEN_EOF, None
					self.read_next_char()
				continue
			elif self.binary and c == UTF8_BOM[0]:
				self.syntax_assert(
					self.read_next_char() == UTF8_BOM[1])
				self.syntax_assert(
					self.read_next_char() == UTF8_BOM[2])
				continue
			elif not self.binary and c == BOM:
				continue
			if not ("%c" % c).isspace():
				break

		if c == ASCII_COLON:
			return TOKEN_COLON, None
		elif c == ASCII_OPEN_BRACE:
			return TOKEN_OPEN_BRACE, None
		elif c == ASCII_CLOSE_BRACE:
			return TOKEN_CLOSE_BRACE, None
		elif c == ASCII_QUOTES:
			return TOKEN_STRING, self.read_string()
		else:
			return TOKEN_STRING, self.read_symbol(c)

	def read_next_token(self):
		if self.error:
			return TOKEN_EOF, None

		return self.do_read_next_token()

YoctonField = collections.namedtuple('YoctonField', ('name', 'value'))

class YoctonObject(object):
	def __init__(self, instream, parent=None):
		self.instream = instream
		self.parent = parent
		self.child = None
		self.done = False
		if parent is None:
			instream.root = self

	def __iter__(self):
		return self

	def __next__(self):
		f = self.next_field()
		if f is None:
			raise StopIteration
		return f

	def next(self):
		return self.__next__()

	def skip_forward(self):
		while self.child is not None and not self.child.done:
			self.child.next_field()
		self.child = None

	def parse_next_field(self):
		tt, value = self.instream.read_next_token()
		if tt == TOKEN_COLON:
			# This is the string:string case.
			tt, value = self.instream.read_next_token()
			if tt != TOKEN_STRING:
				raise SyntaxError(
					"string expected to follow ':'")
			return value

		elif tt == TOKEN_OPEN_BRACE:
			self.child = YoctonObject(self.instream, self)
			return self.child

		raise SyntaxError("':' or '{' expected to follow field name");

	def next_field(self):
		if self.done:
			return None

		self.skip_forward()
		tt, value = self.instream.read_next_token()

		if tt == TOKEN_STRING:
			return YoctonField(value, self.parse_next_field())
		elif tt == TOKEN_CLOSE_BRACE:
			if self == self.instream.root:
				raise SyntaxError("closing brace '}' not "
				                  "expected at top level")
			self.done = True
			return None
		elif tt == TOKEN_EOF:
			if self != self.instream.root:
				raise EOFError(ERROR_EOF)
			self.done = True
			return None
		raise SyntaxError("expected start of next field")

class YoctonWriter(object):
	def __init__(self, outstream):
		self.outstream = outstream
		self.indent_level = 0

	def write_string(self, s):
		if VALID_SYMBOL_RE.match(s):
			self.outstream.write(s)
			return

		# TODO: Write bytes when in binary mode
		self.outstream.write('"')
		for c in s:
			esc = REVERSE_ESCAPES.get(c)
			if esc is not None:
				self.outstream.write(esc)
			elif ord(c) >= 0x20:
				self.outstream.write(c)
			else:
				self.outstream.write("\\x%02x" % ord(c))
		self.outstream.write('"')

	def write_indent(self):
		self.outstream.write("\t" * self.indent_level)

	def write_field(self, name, value):
		self.write_indent()
		self.write_string(name)
		self.outstream.write(": ")
		self.write_string(value)
		self.outstream.write("\n")
		# Always flush after top-level def
		if self.indent_level == 0:
			self.outstream.flush()

	def begin_subobject(self, name):
		self.write_indent()
		self.write_string(name)
		self.outstream.write(" {\n")
		self.indent_level += 1

	def end(self):
		if self.indent_level == 0:
			return
		self.indent_level -= 1
		self.write_indent()
		self.outstream.write("}\n")
		if self.indent_level == 0:
			self.outstream.flush()

def dump(obj, fp):
	if isinstance(obj, dict):
		obj = list(obj.items())

	w = YoctonWriter(fp)

	def write_obj(o):
		for name, value in o:
			if isinstance(value, (str, bytes)):
				w.write_field(name, value)
			else:
				w.begin_subobject(name)
				write_obj(value)
				w.end()

	write_obj(obj)

def dumps(obj):
	buf = io.StringIO()
	dump(obj, buf)
	buf.flush()
	return buf.getvalue()

def load(fp):
	def read_obj(o):
		result = []
		for name, value in o:
			if isinstance(value, YoctonObject):
				result.append((name, read_obj(value)))
			else:
				result.append((name, value))
		return result
	instream = InStream(fp)
	return read_obj(YoctonObject(instream))

def loads(s):
	if isinstance(s, str):
		buf = io.StringIO(s)
	else:
		buf = io.BytesIO(s)
	return load(buf)

