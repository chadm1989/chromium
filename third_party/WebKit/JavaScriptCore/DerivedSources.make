# Copyright (C) 2006 Apple Computer, Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer. 
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution. 
# 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
#     its contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission. 
#
# THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

VPATH = \
    $(JavaScriptCore)/kjs \
    $(JavaScriptCore)/VM \
    $(JavaScriptCore)/pcre \
    $(JavaScriptCore)/docs \
#

.PHONY : all
all : \
    ArrayPrototype.lut.h \
    chartables.c \
    date_object.lut.h \
    grammar.cpp \
    lexer.lut.h \
    MathObject.lut.h \
    NumberConstructor.lut.h \
    RegExpObject.lut.h \
    StringPrototype.lut.h \
    $(JavaScriptCore)/docs/bytecode.html \
#

# lookup tables for classes

%.lut.h: create_hash_table %.cpp
	$^ -i > $@
lexer.lut.h: create_hash_table keywords.table
	$^ > $@

# JavaScript language grammar

grammar.cpp: grammar.y
	bison -d -p kjsyy $< -o $@ > bison_out.txt 2>&1
	perl -p -e 'END { if ($$conflict) { unlink "grammar.cpp"; die; } } $$conflict ||= /conflict/' < bison_out.txt
	touch grammar.cpp.h
	touch grammar.hpp
	cat grammar.cpp.h grammar.hpp > grammar.h
	rm -f grammar.cpp.h grammar.hpp bison_out.txt

# character tables for PCRE

chartables.c : dftables
	$^ $@

$(JavaScriptCore)/docs/bytecode.html: make-bytecode-docs.pl Machine.cpp 
	perl $^ $@

