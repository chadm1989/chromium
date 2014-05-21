// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/hpack_constants.h"

#include <bitset>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/singleton.h"
#include "net/spdy/hpack_huffman_table.h"

namespace net {

namespace {

uint32 bits32(const std::string& bitstring) {
  return std::bitset<32>(bitstring).to_ulong();
}

// SharedHpackHuffmanTable is a Singleton wrapping a HpackHuffmanTable
// instance initialized with |kHpackHuffmanCode|.
struct SharedHpackHuffmanTable {
 public:
  SharedHpackHuffmanTable() {
    std::vector<HpackHuffmanSymbol> code = HpackHuffmanCode();
    scoped_ptr<HpackHuffmanTable> mutable_table(new HpackHuffmanTable());
    CHECK(mutable_table->Initialize(&code[0], code.size()));
    CHECK(mutable_table->IsInitialized());
    table.reset(mutable_table.release());
  }

  static SharedHpackHuffmanTable* GetInstance() {
    return Singleton<SharedHpackHuffmanTable>::get();
  }

  scoped_ptr<const HpackHuffmanTable> table;
};

}  // namespace

// Produced by applying the python program [1] to tables provided by [2].
// [1]
// import re, sys
// count = 0
// for l in sys.stdin:
//   m = re.match(
//     r"^ +('.+'|EOS)? \( *(\d+)\)  \|([10\|]+) +\w+  \[ ?(\d+)\]", l)
//   if m:
//     g = m.groups()
//     print('  {0b%s, %02s, %03s},%s' % (
//       g[2].replace('|','').ljust(32,'0'), g[3], g[1],
//       ('  // %s' % g[0]) if g[0] else ''))
//     count += 1
// print("Total: %s" % count)
//
// [2] http://tools.ietf.org/html/draft-ietf-httpbis-header-compression-07

// HpackHuffmanSymbol entries are initialized as {code, length, id}.
// Codes are specified in the |length| most-significant bits of |code|.
std::vector<HpackHuffmanSymbol> HpackHuffmanCode() {
  const HpackHuffmanSymbol kHpackHuffmanCode[] = {
    {bits32("11111111111111111110111010000000"), 26,   0},
    {bits32("11111111111111111110111011000000"), 26,   1},
    {bits32("11111111111111111110111100000000"), 26,   2},
    {bits32("11111111111111111110111101000000"), 26,   3},
    {bits32("11111111111111111110111110000000"), 26,   4},
    {bits32("11111111111111111110111111000000"), 26,   5},
    {bits32("11111111111111111111000000000000"), 26,   6},
    {bits32("11111111111111111111000001000000"), 26,   7},
    {bits32("11111111111111111111000010000000"), 26,   8},
    {bits32("11111111111111111111000011000000"), 26,   9},
    {bits32("11111111111111111111000100000000"), 26,  10},
    {bits32("11111111111111111111000101000000"), 26,  11},
    {bits32("11111111111111111111000110000000"), 26,  12},
    {bits32("11111111111111111111000111000000"), 26,  13},
    {bits32("11111111111111111111001000000000"), 26,  14},
    {bits32("11111111111111111111001001000000"), 26,  15},
    {bits32("11111111111111111111001010000000"), 26,  16},
    {bits32("11111111111111111111001011000000"), 26,  17},
    {bits32("11111111111111111111001100000000"), 26,  18},
    {bits32("11111111111111111111001101000000"), 26,  19},
    {bits32("11111111111111111111001110000000"), 26,  20},
    {bits32("11111111111111111111001111000000"), 26,  21},
    {bits32("11111111111111111111010000000000"), 26,  22},
    {bits32("11111111111111111111010001000000"), 26,  23},
    {bits32("11111111111111111111010010000000"), 26,  24},
    {bits32("11111111111111111111010011000000"), 26,  25},
    {bits32("11111111111111111111010100000000"), 26,  26},
    {bits32("11111111111111111111010101000000"), 26,  27},
    {bits32("11111111111111111111010110000000"), 26,  28},
    {bits32("11111111111111111111010111000000"), 26,  29},
    {bits32("11111111111111111111011000000000"), 26,  30},
    {bits32("11111111111111111111011001000000"), 26,  31},
    {bits32("00110000000000000000000000000000"),  5,  32},  // ' '
    {bits32("11111111111000000000000000000000"), 13,  33},  // '!'
    {bits32("11111000000000000000000000000000"),  9,  34},  // '"'
    {bits32("11111111111100000000000000000000"), 14,  35},  // '#'
    {bits32("11111111111110000000000000000000"), 15,  36},  // '$'
    {bits32("01111000000000000000000000000000"),  6,  37},  // '%'
    {bits32("11001000000000000000000000000000"),  7,  38},  // '&'
    {bits32("11111111111010000000000000000000"), 13,  39},  // '''
    {bits32("11111110100000000000000000000000"), 10,  40},  // '('
    {bits32("11111000100000000000000000000000"),  9,  41},  // ')'
    {bits32("11111110110000000000000000000000"), 10,  42},  // '*'
    {bits32("11111111000000000000000000000000"), 10,  43},  // '+'
    {bits32("11001010000000000000000000000000"),  7,  44},  // ','
    {bits32("11001100000000000000000000000000"),  7,  45},  // '-'
    {bits32("01111100000000000000000000000000"),  6,  46},  // '.'
    {bits32("00111000000000000000000000000000"),  5,  47},  // '/'
    {bits32("00000000000000000000000000000000"),  4,  48},  // '0'
    {bits32("00010000000000000000000000000000"),  4,  49},  // '1'
    {bits32("00100000000000000000000000000000"),  4,  50},  // '2'
    {bits32("01000000000000000000000000000000"),  5,  51},  // '3'
    {bits32("10000000000000000000000000000000"),  6,  52},  // '4'
    {bits32("10000100000000000000000000000000"),  6,  53},  // '5'
    {bits32("10001000000000000000000000000000"),  6,  54},  // '6'
    {bits32("10001100000000000000000000000000"),  6,  55},  // '7'
    {bits32("10010000000000000000000000000000"),  6,  56},  // '8'
    {bits32("10010100000000000000000000000000"),  6,  57},  // '9'
    {bits32("10011000000000000000000000000000"),  6,  58},  // ':'
    {bits32("11101100000000000000000000000000"),  8,  59},  // ';'
    {bits32("11111111111111100000000000000000"), 17,  60},  // '<'
    {bits32("10011100000000000000000000000000"),  6,  61},  // '='
    {bits32("11111111111110100000000000000000"), 15,  62},  // '>'
    {bits32("11111111010000000000000000000000"), 10,  63},  // '?'
    {bits32("11111111111111000000000000000000"), 15,  64},  // '@'
    {bits32("11001110000000000000000000000000"),  7,  65},  // 'A'
    {bits32("11101101000000000000000000000000"),  8,  66},  // 'B'
    {bits32("11101110000000000000000000000000"),  8,  67},  // 'C'
    {bits32("11010000000000000000000000000000"),  7,  68},  // 'D'
    {bits32("11101111000000000000000000000000"),  8,  69},  // 'E'
    {bits32("11010010000000000000000000000000"),  7,  70},  // 'F'
    {bits32("11010100000000000000000000000000"),  7,  71},  // 'G'
    {bits32("11111001000000000000000000000000"),  9,  72},  // 'H'
    {bits32("11110000000000000000000000000000"),  8,  73},  // 'I'
    {bits32("11111001100000000000000000000000"),  9,  74},  // 'J'
    {bits32("11111010000000000000000000000000"),  9,  75},  // 'K'
    {bits32("11111010100000000000000000000000"),  9,  76},  // 'L'
    {bits32("11010110000000000000000000000000"),  7,  77},  // 'M'
    {bits32("11011000000000000000000000000000"),  7,  78},  // 'N'
    {bits32("11110001000000000000000000000000"),  8,  79},  // 'O'
    {bits32("11110010000000000000000000000000"),  8,  80},  // 'P'
    {bits32("11111011000000000000000000000000"),  9,  81},  // 'Q'
    {bits32("11111011100000000000000000000000"),  9,  82},  // 'R'
    {bits32("11011010000000000000000000000000"),  7,  83},  // 'S'
    {bits32("10100000000000000000000000000000"),  6,  84},  // 'T'
    {bits32("11110011000000000000000000000000"),  8,  85},  // 'U'
    {bits32("11111100000000000000000000000000"),  9,  86},  // 'V'
    {bits32("11111100100000000000000000000000"),  9,  87},  // 'W'
    {bits32("11110100000000000000000000000000"),  8,  88},  // 'X'
    {bits32("11111101000000000000000000000000"),  9,  89},  // 'Y'
    {bits32("11111101100000000000000000000000"),  9,  90},  // 'Z'
    {bits32("11111111100000000000000000000000"), 11,  91},  // '['
    {bits32("11111111111111111111011010000000"), 26,  92},  // '\'
    {bits32("11111111101000000000000000000000"), 11,  93},  // ']'
    {bits32("11111111111101000000000000000000"), 14,  94},  // '^'
    {bits32("11011100000000000000000000000000"),  7,  95},  // '_'
    {bits32("11111111111111111000000000000000"), 18,  96},  // '`'
    {bits32("01001000000000000000000000000000"),  5,  97},  // 'a'
    {bits32("11011110000000000000000000000000"),  7,  98},  // 'b'
    {bits32("01010000000000000000000000000000"),  5,  99},  // 'c'
    {bits32("10100100000000000000000000000000"),  6, 100},  // 'd'
    {bits32("01011000000000000000000000000000"),  5, 101},  // 'e'
    {bits32("11100000000000000000000000000000"),  7, 102},  // 'f'
    {bits32("10101000000000000000000000000000"),  6, 103},  // 'g'
    {bits32("10101100000000000000000000000000"),  6, 104},  // 'h'
    {bits32("01100000000000000000000000000000"),  5, 105},  // 'i'
    {bits32("11110101000000000000000000000000"),  8, 106},  // 'j'
    {bits32("11110110000000000000000000000000"),  8, 107},  // 'k'
    {bits32("10110000000000000000000000000000"),  6, 108},  // 'l'
    {bits32("10110100000000000000000000000000"),  6, 109},  // 'm'
    {bits32("10111000000000000000000000000000"),  6, 110},  // 'n'
    {bits32("01101000000000000000000000000000"),  5, 111},  // 'o'
    {bits32("10111100000000000000000000000000"),  6, 112},  // 'p'
    {bits32("11111110000000000000000000000000"),  9, 113},  // 'q'
    {bits32("11000000000000000000000000000000"),  6, 114},  // 'r'
    {bits32("11000100000000000000000000000000"),  6, 115},  // 's'
    {bits32("01110000000000000000000000000000"),  5, 116},  // 't'
    {bits32("11100010000000000000000000000000"),  7, 117},  // 'u'
    {bits32("11100100000000000000000000000000"),  7, 118},  // 'v'
    {bits32("11100110000000000000000000000000"),  7, 119},  // 'w'
    {bits32("11101000000000000000000000000000"),  7, 120},  // 'x'
    {bits32("11101010000000000000000000000000"),  7, 121},  // 'y'
    {bits32("11110111000000000000000000000000"),  8, 122},  // 'z'
    {bits32("11111111111111101000000000000000"), 17, 123},  // '{'
    {bits32("11111111110000000000000000000000"), 12, 124},  // '|'
    {bits32("11111111111111110000000000000000"), 17, 125},  // '}'
    {bits32("11111111110100000000000000000000"), 12, 126},  // '~'
    {bits32("11111111111111111111011011000000"), 26, 127},
    {bits32("11111111111111111111011100000000"), 26, 128},
    {bits32("11111111111111111111011101000000"), 26, 129},
    {bits32("11111111111111111111011110000000"), 26, 130},
    {bits32("11111111111111111111011111000000"), 26, 131},
    {bits32("11111111111111111111100000000000"), 26, 132},
    {bits32("11111111111111111111100001000000"), 26, 133},
    {bits32("11111111111111111111100010000000"), 26, 134},
    {bits32("11111111111111111111100011000000"), 26, 135},
    {bits32("11111111111111111111100100000000"), 26, 136},
    {bits32("11111111111111111111100101000000"), 26, 137},
    {bits32("11111111111111111111100110000000"), 26, 138},
    {bits32("11111111111111111111100111000000"), 26, 139},
    {bits32("11111111111111111111101000000000"), 26, 140},
    {bits32("11111111111111111111101001000000"), 26, 141},
    {bits32("11111111111111111111101010000000"), 26, 142},
    {bits32("11111111111111111111101011000000"), 26, 143},
    {bits32("11111111111111111111101100000000"), 26, 144},
    {bits32("11111111111111111111101101000000"), 26, 145},
    {bits32("11111111111111111111101110000000"), 26, 146},
    {bits32("11111111111111111111101111000000"), 26, 147},
    {bits32("11111111111111111111110000000000"), 26, 148},
    {bits32("11111111111111111111110001000000"), 26, 149},
    {bits32("11111111111111111111110010000000"), 26, 150},
    {bits32("11111111111111111111110011000000"), 26, 151},
    {bits32("11111111111111111111110100000000"), 26, 152},
    {bits32("11111111111111111111110101000000"), 26, 153},
    {bits32("11111111111111111111110110000000"), 26, 154},
    {bits32("11111111111111111111110111000000"), 26, 155},
    {bits32("11111111111111111111111000000000"), 26, 156},
    {bits32("11111111111111111111111001000000"), 26, 157},
    {bits32("11111111111111111111111010000000"), 26, 158},
    {bits32("11111111111111111111111011000000"), 26, 159},
    {bits32("11111111111111111111111100000000"), 26, 160},
    {bits32("11111111111111111111111101000000"), 26, 161},
    {bits32("11111111111111111111111110000000"), 26, 162},
    {bits32("11111111111111111111111111000000"), 26, 163},
    {bits32("11111111111111111100000000000000"), 25, 164},
    {bits32("11111111111111111100000010000000"), 25, 165},
    {bits32("11111111111111111100000100000000"), 25, 166},
    {bits32("11111111111111111100000110000000"), 25, 167},
    {bits32("11111111111111111100001000000000"), 25, 168},
    {bits32("11111111111111111100001010000000"), 25, 169},
    {bits32("11111111111111111100001100000000"), 25, 170},
    {bits32("11111111111111111100001110000000"), 25, 171},
    {bits32("11111111111111111100010000000000"), 25, 172},
    {bits32("11111111111111111100010010000000"), 25, 173},
    {bits32("11111111111111111100010100000000"), 25, 174},
    {bits32("11111111111111111100010110000000"), 25, 175},
    {bits32("11111111111111111100011000000000"), 25, 176},
    {bits32("11111111111111111100011010000000"), 25, 177},
    {bits32("11111111111111111100011100000000"), 25, 178},
    {bits32("11111111111111111100011110000000"), 25, 179},
    {bits32("11111111111111111100100000000000"), 25, 180},
    {bits32("11111111111111111100100010000000"), 25, 181},
    {bits32("11111111111111111100100100000000"), 25, 182},
    {bits32("11111111111111111100100110000000"), 25, 183},
    {bits32("11111111111111111100101000000000"), 25, 184},
    {bits32("11111111111111111100101010000000"), 25, 185},
    {bits32("11111111111111111100101100000000"), 25, 186},
    {bits32("11111111111111111100101110000000"), 25, 187},
    {bits32("11111111111111111100110000000000"), 25, 188},
    {bits32("11111111111111111100110010000000"), 25, 189},
    {bits32("11111111111111111100110100000000"), 25, 190},
    {bits32("11111111111111111100110110000000"), 25, 191},
    {bits32("11111111111111111100111000000000"), 25, 192},
    {bits32("11111111111111111100111010000000"), 25, 193},
    {bits32("11111111111111111100111100000000"), 25, 194},
    {bits32("11111111111111111100111110000000"), 25, 195},
    {bits32("11111111111111111101000000000000"), 25, 196},
    {bits32("11111111111111111101000010000000"), 25, 197},
    {bits32("11111111111111111101000100000000"), 25, 198},
    {bits32("11111111111111111101000110000000"), 25, 199},
    {bits32("11111111111111111101001000000000"), 25, 200},
    {bits32("11111111111111111101001010000000"), 25, 201},
    {bits32("11111111111111111101001100000000"), 25, 202},
    {bits32("11111111111111111101001110000000"), 25, 203},
    {bits32("11111111111111111101010000000000"), 25, 204},
    {bits32("11111111111111111101010010000000"), 25, 205},
    {bits32("11111111111111111101010100000000"), 25, 206},
    {bits32("11111111111111111101010110000000"), 25, 207},
    {bits32("11111111111111111101011000000000"), 25, 208},
    {bits32("11111111111111111101011010000000"), 25, 209},
    {bits32("11111111111111111101011100000000"), 25, 210},
    {bits32("11111111111111111101011110000000"), 25, 211},
    {bits32("11111111111111111101100000000000"), 25, 212},
    {bits32("11111111111111111101100010000000"), 25, 213},
    {bits32("11111111111111111101100100000000"), 25, 214},
    {bits32("11111111111111111101100110000000"), 25, 215},
    {bits32("11111111111111111101101000000000"), 25, 216},
    {bits32("11111111111111111101101010000000"), 25, 217},
    {bits32("11111111111111111101101100000000"), 25, 218},
    {bits32("11111111111111111101101110000000"), 25, 219},
    {bits32("11111111111111111101110000000000"), 25, 220},
    {bits32("11111111111111111101110010000000"), 25, 221},
    {bits32("11111111111111111101110100000000"), 25, 222},
    {bits32("11111111111111111101110110000000"), 25, 223},
    {bits32("11111111111111111101111000000000"), 25, 224},
    {bits32("11111111111111111101111010000000"), 25, 225},
    {bits32("11111111111111111101111100000000"), 25, 226},
    {bits32("11111111111111111101111110000000"), 25, 227},
    {bits32("11111111111111111110000000000000"), 25, 228},
    {bits32("11111111111111111110000010000000"), 25, 229},
    {bits32("11111111111111111110000100000000"), 25, 230},
    {bits32("11111111111111111110000110000000"), 25, 231},
    {bits32("11111111111111111110001000000000"), 25, 232},
    {bits32("11111111111111111110001010000000"), 25, 233},
    {bits32("11111111111111111110001100000000"), 25, 234},
    {bits32("11111111111111111110001110000000"), 25, 235},
    {bits32("11111111111111111110010000000000"), 25, 236},
    {bits32("11111111111111111110010010000000"), 25, 237},
    {bits32("11111111111111111110010100000000"), 25, 238},
    {bits32("11111111111111111110010110000000"), 25, 239},
    {bits32("11111111111111111110011000000000"), 25, 240},
    {bits32("11111111111111111110011010000000"), 25, 241},
    {bits32("11111111111111111110011100000000"), 25, 242},
    {bits32("11111111111111111110011110000000"), 25, 243},
    {bits32("11111111111111111110100000000000"), 25, 244},
    {bits32("11111111111111111110100010000000"), 25, 245},
    {bits32("11111111111111111110100100000000"), 25, 246},
    {bits32("11111111111111111110100110000000"), 25, 247},
    {bits32("11111111111111111110101000000000"), 25, 248},
    {bits32("11111111111111111110101010000000"), 25, 249},
    {bits32("11111111111111111110101100000000"), 25, 250},
    {bits32("11111111111111111110101110000000"), 25, 251},
    {bits32("11111111111111111110110000000000"), 25, 252},
    {bits32("11111111111111111110110010000000"), 25, 253},
    {bits32("11111111111111111110110100000000"), 25, 254},
    {bits32("11111111111111111110110110000000"), 25, 255},
    {bits32("11111111111111111110111000000000"), 25, 256},  // EOS
  };
  return std::vector<HpackHuffmanSymbol>(
      kHpackHuffmanCode,
      kHpackHuffmanCode + arraysize(kHpackHuffmanCode));
}

const HpackHuffmanTable& ObtainHpackHuffmanTable() {
  return *SharedHpackHuffmanTable::GetInstance()->table;
}

}  // namespace net
