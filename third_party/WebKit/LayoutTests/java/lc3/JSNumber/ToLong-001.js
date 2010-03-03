/* ***** BEGIN LICENSE BLOCK *****
 *
 * Copyright (C) 1997, 1998 Netscape Communications Corporation.
 * Copyright (C) 2010 Apple Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * ***** END LICENSE BLOCK ***** */

gTestfile = 'ToLong-001.js';

/**
 *  JavaScript to Java type conversion.
 *
 *  This test passes JavaScript number values to several Java methods
 *  that expect arguments of various types, and verifies that the value is
 *  converted to the correct value and type.
 *
 *  This tests instance methods, and not static methods.
 *
 *  Running these tests successfully requires you to have
 *  com.netscape.javascript.qa.liveconnect.DataTypeClass on your classpath.
 *
 *  Specification:  Method Overloading Proposal for Liveconnect 3.0
 *
 *  @author: christine@netscape.com
 *
 */
var SECTION = "number conversion to long";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

var dt = applet.createQAObject("com.netscape.javascript.qa.liveconnect.DataTypeClass");

var a = new Array();
var i = 0;

// passing a JS number to a method that expects a long:
// round JS number to integral value using round-to-negative-infinity mode
// numbers with a magnitude too large to be represented in the target integral
//  type result in a runtime error.
// Nan converts to 0.

// Special cases:  0, -0, Infinity, -Infinity, and NaN

a[i++] = new TestObject(
  "dt.setLong( 0 )",
  "dt.PUB_LONG",
  "dt.getLong()",
  "typeof dt.getLong()",
  '0',
  "'number'" );

a[i++] = new TestObject(
  "dt.setLong( -0 )",
  "Infinity / dt.PUB_LONG",
  "Infinity / dt.getLong()",
  "typeof dt.getLong()",
  'Infinity',
  '"number"' );

a[i++] = new TestObject(
  "dt.setLong(1234567890123)",
  "dt.PUB_LONG",
  "dt.getLong()",
  "typeof dt.getLong()",
  '1234567890123',
  '"number"' );

a[i++] = new TestObject(
  "dt.setLong(-1234567890123)",
  "dt.PUB_LONG",
  "dt.getLong()",
  "typeof dt.getLong()",
  '-1234567890123',
  '"number"' );

a[i++] = new TestObject(
  "dt.setLong(0x7ffffffffffffff)",
  "dt.PUB_LONG",
  "dt.getLong()",
  "typeof dt.getLong()",
  '0x7ffffffffffffff',
  '"number"' );

a[i++] = new TestObject(
  "dt.setLong(0x7ffffffffffffff)",
  "dt.PUB_LONG",
  "dt.getLong()",
  "typeof dt.getLong()",
  '0x7ffffffffffffff',
  '"number"' );

a[i++] = new TestObject(
  "dt.setLong(0xfffffffffffffff)",
  "dt.PUB_LONG",
  "dt.getLong()",
  "typeof dt.getLong()",
  '0xfffffffffffffff',
  '"number"' );

a[i++] = new TestObject(
  "dt.setLong(0x6fffffffffffffff)",
  "dt.PUB_LONG",
  "dt.getLong()",
  "typeof dt.getLong()",
  '0x6fffffffffffffff',
  '"number"' );

a[i++] = new TestObject(
  "dt.setLong(-0x6fffffffffffffff)",
  "dt.PUB_LONG",
  "dt.getLong()",
  "typeof dt.getLong()",
 ' -0x6fffffffffffffff',
  '"number"' );

/*
  a[i++] = new TestObject(
  "dt.setLong( java.lang.Long.MAX_VALUE )",
  "dt.PUB_LONG",
  "dt.getLong()",
  "typeof dt.getLong()",
  'java.lang.Long.MAX_VALUE',
  '"number"' );

  a[i++] = new TestObject(
  "dt.setLong( java.lang.Long.MIN_VALUE )",
  "dt.PUB_LONG",
  "dt.getLong()",
  "typeof dt.getLong()",
  'java.lang.Long.MIN_VALUE',
  '"number"' );

  a[i++] = new TestObject(
  "dt.setLong( -java.lang.Long.MAX_VALUE )",
  "dt.PUB_LONG",
  "dt.getLong()",
  "typeof dt.getLong()",
  '-java.lang.Long.MAX_VALUE',
  '"number"' );

  a[i++] = new TestObject(
  "dt.setLong( -java.lang.Long.MIN_VALUE )",
  "dt.PUB_LONG",
  "dt.getLong()",
  "typeof dt.getLong()",
  '-java.lang.Long.MIN_VALUE',
  '"number"' );
*/
for ( i = 0; i < a.length; i++ ) {
  shouldBeWithErrorCheck(
    a[i].description +"; "+ a[i].javaFieldName,
    a[i].jsValue);

  shouldBeWithErrorCheck(
    a[i].description +"; " + a[i].javaMethodName,
    a[i].jsValue);

  shouldBeWithErrorCheck(
    a[i].javaTypeName,
    a[i].jsType);
}

function TestObject( description, javaField, javaMethod, javaType,
                     jsValue, jsType )
{
  this.description = description;
  this.javaFieldName = javaField;
  this.javaMethodName = javaMethod;
  this.javaTypeName = javaType,

  this.jsValue   = jsValue;
  this.jsType      = jsType;
}
