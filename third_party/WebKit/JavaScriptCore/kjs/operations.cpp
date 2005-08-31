// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#ifndef HAVE_FUNC_ISINF
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#endif /* HAVE_FUNC_ISINF */

#if HAVE_FLOAT_H
#include <float.h>
#endif

#include "operations.h"
#include "object.h"

using namespace KJS;

#if !APPLE_CHANGES

bool KJS::isNaN(double d)
{
#ifdef HAVE_FUNC_ISNAN
  return isnan(d);
#elif defined HAVE_FLOAT_H
  return _isnan(d) != 0;
#else
  return !(d == d);
#endif
}

bool KJS::isInf(double d)
{
#if defined(HAVE_FUNC_ISINF)
  return isinf(d);
#elif HAVE_FUNC_FINITE
  return finite(d) == 0 && d == d;
#elif HAVE_FUNC__FINITE
  return _finite(d) == 0 && d == d;
#else
  return false;
#endif
}

bool KJS::isPosInf(double d)
{
#if APPLE_CHANGES
  return isinf(d) && d > 0;
#else
#if defined(HAVE_FUNC_ISINF)
  return (isinf(d) == 1);
#elif HAVE_FUNC_FINITE
  return finite(d) == 0 && d == d; // ### can we distinguish between + and - ?
#elif HAVE_FUNC__FINITE
  return _finite(d) == 0 && d == d; // ###
#else
  return false;
#endif
#endif
}

bool KJS::isNegInf(double d)
{
#if APPLE_CHANGES
  return isinf(d) && d < 0;
#else
#if defined(HAVE_FUNC_ISINF)
  return (isinf(d) == -1);
#elif HAVE_FUNC_FINITE
  return finite(d) == 0 && d == d; // ###
#elif HAVE_FUNC__FINITE
  return _finite(d) == 0 && d == d; // ###
#else
  return false;
#endif
#endif
}

#endif

// ECMA 11.9.3
bool KJS::equal(ExecState *exec, ValueImp *v1, ValueImp *v2)
{
    Type t1 = v1->type();
    Type t2 = v2->type();

    if (t1 != t2) {
        if (t1 == UndefinedType)
            t1 = NullType;
        if (t2 == UndefinedType)
            t2 = NullType;

        if (t1 == BooleanType)
            t1 = NumberType;
        if (t2 == BooleanType)
            t2 = NumberType;

        if (t1 == NumberType && t2 == StringType) {
            // use toNumber
        } else if (t1 == StringType && t2 == NumberType) {
            t1 = NumberType;
            // use toNumber
        } else {
            if ((t1 == StringType || t1 == NumberType) && t2 >= ObjectType)
                return equal(exec, v1, v2->toPrimitive(exec));
            if (t1 >= ObjectType && (t2 == StringType || t2 == NumberType))
                return equal(exec, v1->toPrimitive(exec), v2);
            if (t1 != t2)
                return false;
        }
    }

    if (t1 == UndefinedType || t1 == NullType)
        return true;

    if (t1 == NumberType) {
        double d1 = v1->toNumber(exec);
        double d2 = v2->toNumber(exec);
        // FIXME: Isn't this already how NaN behaves?
        // Why the extra line of code?
        if (isNaN(d1) || isNaN(d2))
            return false;
        return d1 == d2; /* TODO: +0, -0 ? */
    }

    if (t1 == StringType)
        return v1->toString(exec) == v2->toString(exec);

    if (t1 == BooleanType)
        return v1->toBoolean(exec) == v2->toBoolean(exec);

    // types are Object
    return v1 == v2;
}

bool KJS::strictEqual(ExecState *exec, ValueImp *v1, ValueImp *v2)
{
  Type t1 = v1->type();
  Type t2 = v2->type();

  if (t1 != t2)
    return false;
  if (t1 == UndefinedType || t1 == NullType)
    return true;
  if (t1 == NumberType) {
    double n1 = v1->toNumber(exec);
    double n2 = v2->toNumber(exec);
    // FIXME: Isn't this already how NaN behaves?
    // Why the extra line of code?
    if (isNaN(n1) || isNaN(n2))
      return false;
    if (n1 == n2)
      return true;
    /* TODO: +0 and -0 */
    return false;
  } else if (t1 == StringType) {
    return v1->toString(exec) == v2->toString(exec);
  } else if (t2 == BooleanType) {
    return v1->toBoolean(exec) == v2->toBoolean(exec);
  }
  if (v1 == v2)
    return true;
  /* TODO: joined objects */

  return false;
}

int KJS::relation(ExecState *exec, ValueImp *v1, ValueImp *v2)
{
  ValueImp *p1 = v1->toPrimitive(exec,NumberType);
  ValueImp *p2 = v2->toPrimitive(exec,NumberType);

  if (p1->isString() && p2->isString())
    return p1->toString(exec) < p2->toString(exec) ? 1 : 0;

  double n1 = p1->toNumber(exec);
  double n2 = p2->toNumber(exec);
  if (n1 < n2)
    return 1;
  if (n1 >= n2)
    return 0;
  return -1; // must be NaN, so undefined
}

int KJS::maxInt(int d1, int d2)
{
  return (d1 > d2) ? d1 : d2;
}

int KJS::minInt(int d1, int d2)
{
  return (d1 < d2) ? d1 : d2;
}

// ECMA 11.6
ValueImp *KJS::add(ExecState *exec, ValueImp *v1, ValueImp *v2, char oper)
{
  // exception for the Date exception in defaultValue()
  Type preferred = oper == '+' ? UnspecifiedType : NumberType;
  ValueImp *p1 = v1->toPrimitive(exec, preferred);
  ValueImp *p2 = v2->toPrimitive(exec, preferred);

  if ((p1->isString() || p2->isString()) && oper == '+') {
    return jsString(p1->toString(exec) + p2->toString(exec));
  }

  bool n1KnownToBeInteger;
  double n1 = p1->toNumber(exec, n1KnownToBeInteger);
  bool n2KnownToBeInteger;
  double n2 = p2->toNumber(exec, n2KnownToBeInteger);

  bool resultKnownToBeInteger = n1KnownToBeInteger && n2KnownToBeInteger;

  if (oper == '+')
    return jsNumber(n1 + n2, resultKnownToBeInteger);
  else
    return jsNumber(n1 - n2, resultKnownToBeInteger);
}

// ECMA 11.5
ValueImp *KJS::mult(ExecState *exec, ValueImp *v1, ValueImp *v2, char oper)
{
  bool n1KnownToBeInteger;
  double n1 = v1->toNumber(exec, n1KnownToBeInteger);
  bool n2KnownToBeInteger;
  double n2 = v2->toNumber(exec, n2KnownToBeInteger);

  double result;
  bool resultKnownToBeInteger;

  if (oper == '*') {
    result = n1 * n2;
    resultKnownToBeInteger = n1KnownToBeInteger && n2KnownToBeInteger;
  } else if (oper == '/') {
    result = n1 / n2;
    resultKnownToBeInteger = false;
  } else {
    result = fmod(n1, n2);
    resultKnownToBeInteger = n1KnownToBeInteger && n2KnownToBeInteger && n2 != 0;
  }

  return jsNumber(result, resultKnownToBeInteger);
}
