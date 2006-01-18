#!/usr/bin/perl -w
#
#  Copyright (C) 2006 Apple Computer, Inc.
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Library General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Library General Public License for more details.
#
#  You should have received a copy of the GNU Library General Public License
#  along with this library; see the file COPYING.LIB.  If not, write to
#  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
#  Boston, MA 02111-1307, USA.
#

# Usage: make-css-file-arrays.pl <header> <output> <input> ...

use strict;

my $header = $ARGV[0];
shift;

my $out = $ARGV[0];
shift;

open HEADER, ">", $header or die;
open OUT, ">", $out or die;

print HEADER "namespace WebCore {\n";
print OUT "namespace WebCore {\n";

for my $in (@ARGV) {
    $in =~ /(\w+)\.css$/ or die;
    my $name = $1;

    # Slurp in the CSS file.
    open IN, $in or die;
    my $text; { local $/; $text = <IN>; }
    close IN;

    # Remove comments in a simple-minded way that will work fine for our files.
    # Could do this a fancier way if we were worried about arbitrary CSS source.
    $text =~ s|/\*.*?\*/||gs;

    # Crunch whitespace just to make it a little smaller.
    # Could do work to avoid doing this inside quote marks but our files don't have runs of spaces in quotes.
    # Could crunch further based on places where whitespace is optional.
    $text =~ s|\s+| |gs;
    $text =~ s|^ ||;
    $text =~ s| $||;

    # Write out a C array of the characters.
    my $length = length $text;
    print HEADER "extern const unsigned short ${name}UserAgentStyleSheet[${length}];\n";
    print OUT "extern const unsigned short ${name}UserAgentStyleSheet[${length}] = {\n";
    my $i = 0;
    while ($i < $length) {
        print "    ";
        my $j = 0;
        while ($j < 16 && $i < $length) {
            print OUT ", " unless $j == 0;
            print OUT ord substr $text, $i, 1;
            ++$i;
            ++$j;
        }
        print OUT "," unless $i == $length;
        print OUT "\n";
    }
    print OUT "};\n";

}

print HEADER "}\n";
print OUT "}\n";
