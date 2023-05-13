#!/usr/bin/env python3
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

import glob
import unittest
import yocton

TEST_INPUT = """
foo: bar
"ba z": "qux\\n\\\\qux"
sub-obj {
	something: 1
	something: 2
	something: 3
	sub_sub_obj {
		temp: -35C
		temp2: +99C
	}
}
""".strip()

def read_special_comments(fp, s):
	test_param_lines = []
	test_output_lines = []
	for line in fp.readlines():
		if line.startswith(s("//| ")):
			test_param_lines.append(line[4:])
		if line.startswith(s("//> ")):
			test_output_lines.append(line[4:])

	params = dict(yocton.loads(s("").join(test_param_lines)))
	params[s("output")] = s("").join(test_output_lines)
	fp.seek(0)
	return params

def evaluate_obj(obj, output, s):
	for name, value in obj:
		if name == s("special.is_equal"):
			values = dict(value)
			assert values["x"] == values["y"], "values not equal"
		if isinstance(value, yocton.YoctonObject):
			evaluate_obj(value, output, s)
		if name == s("output"):
			output.append(value + s("\n"))

def get_file_encoding(filename):
	if "latin1" in filename:
		return "latin1"
	else:
		return "utf-8"

class YoctonReadTests(unittest.TestCase):

	def process_test_file(self, fp, s):
		params = read_special_comments(fp, s)
		if params.get(s("c_only")) == s("true"):
			return
		stream = yocton.InStream(fp)
		obj = yocton.YoctonObject(stream)
		try:
			output = []
			evaluate_obj(obj, output, s)
			output = s("").join(output)
			self.assertEqual(output, params[s("output")])
		except Exception as e:
			self.assertTrue(s("error_message") in params)
			self.assertEqual(params[s("error_message")],
					 s(e.args[0]))
			# TODO: Check lineno

	def test_from_test_files(self):
		for filename in glob.glob("tests/*.yocton"):
			# TODO: Read in binary mode, too
			with self.subTest(filename=filename, mode="r"):
				encoding = get_file_encoding(filename)
				s = lambda x: x
				with open(filename, "r", encoding=encoding) as fp:
					self.process_test_file(fp, s)
			with self.subTest(filename=filename, mode="rb"):
				s = lambda x: x.encode("utf-8")
				with open(filename, "rb") as fp:
					self.process_test_file(fp, s)

	def test_load(self):
		self.assertEqual(yocton.loads(TEST_INPUT), [
			("foo", "bar"),
			("ba z", "qux\n\\qux"),
			("sub-obj", [
				("something", "1"),
				("something", "2"),
				("something", "3"),
				("sub_sub_obj", [
					("temp", "-35C"),
					("temp2", "+99C"),
				]),
			]),
		])

class YoctonWriteTests(unittest.TestCase):
	def test_dumps(self):
		self.assertEqual(yocton.dumps({
			"foo": "bar",
			"ba z": "qux\n\\qux",
			"sub-obj": [
				("something", "1"),
				("something", "2"),
				("something", "3"),
				("sub_sub_obj", (
					("temp", "-35C"),
					("temp2", "+99C"),
				)),
			],
		}).strip(), TEST_INPUT)


if __name__ == "__main__":
	unittest.main()
