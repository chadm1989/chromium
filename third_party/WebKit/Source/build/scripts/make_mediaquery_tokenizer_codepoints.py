#!/usr/bin/env python

# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import in_generator
import sys
import os

module_basename = os.path.basename(__file__)
module_pyname = os.path.splitext(module_basename)[0] + '.py'

CPP_TEMPLATE = """
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Auto-generated by {module_pyname}

const MediaQueryTokenizer::CodePoint MediaQueryTokenizer::codePoints[{array_size}] = {{
{token_lines}
}};
const unsigned codePointsNumber = {array_size};
"""


def token_type(i):
    codepoints = {'(': 'leftParenthesis',
                  ')': 'rightParenthesis',
                  '+': 'plusOrFullStop',
                  '.': 'plusOrFullStop',
                  '-': 'hyphenMinus',
                  ',': 'comma',
                  '/': 'solidus',
                  '\\': 'reverseSolidus',
                  ':': 'colon',
                  ';': 'semiColon',
                  }
    whitespace = '\n\r\t\f '
    c = chr(i)
    if c in whitespace:
        return 'whiteSpace'
    if c.isdigit():
        return 'asciiDigit'
    if c.isalpha() or c == '_':
        return 'nameStart'
    if i == 0:
        return 'endOfFile'
    return codepoints.get(c)


class MakeMediaQueryTokenizerCodePointsWriter(in_generator.Writer):
    defaults = {
        'Conditional': None,
        'RuntimeEnabled': None,
        'ImplementedAs': None,
    }
    filters = {
    }
    default_parameters = {
        'namespace': '',
        'export': '',
    }

    def __init__(self, in_file_path):
        super(MakeMediaQueryTokenizerCodePointsWriter, self).__init__(in_file_path)

        self._outputs = {
            ('MediaQueryTokenizerCodepoints.cpp'): self.generate,
        }
        self._template_context = {
            'namespace': '',
            'export': '',
        }

    def generate(self):
        array_size = 128  # SCHAR_MAX + 1
        token_lines = ['    &MediaQueryTokenizer::%s,' % token_type(i)
                        if token_type(i) else '    0,'
                        for i in range(array_size)]
        return CPP_TEMPLATE.format(array_size=array_size, token_lines='\n'.join(token_lines), module_pyname=module_pyname)

if __name__ == '__main__':
    in_generator.Maker(MakeMediaQueryTokenizerCodePointsWriter).main(sys.argv)
