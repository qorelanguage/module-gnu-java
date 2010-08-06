/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  gnu-java-util.h

  Qore Programming Language gcj Module

  Copyright (C) 2010 Qore Technologies

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _QORE_GNU_JAVA_UTIL_H

#define _QORE_GNU_JAVA_UTIL_H

#include <java/lang/String.h>
#include <java/lang/Object.h>
#include <java/lang/Class.h>
#include <java/lang/Throwable.h>

DLLLOCAL AbstractQoreNode *javaToQore(jbyte i);
DLLLOCAL AbstractQoreNode *javaToQore(jshort i);
DLLLOCAL AbstractQoreNode *javaToQore(jint i);
DLLLOCAL AbstractQoreNode *javaToQore(jlong i);
DLLLOCAL AbstractQoreNode *javaToQore(jboolean b);
DLLLOCAL AbstractQoreNode *javaToQore(jfloat f);
DLLLOCAL AbstractQoreNode *javaToQore(jdouble f);
DLLLOCAL AbstractQoreNode *javaToQore(jchar c);

DLLLOCAL AbstractQoreNode *javaToQore(java::lang::String *jstr);

DLLLOCAL java::lang::Object *toJava(const char *str);
DLLLOCAL java::lang::Object *toJava(jlong i);
DLLLOCAL java::lang::Object *toJava(jboolean b);
DLLLOCAL java::lang::Object *toJava(jfloat f);
DLLLOCAL java::lang::Object *toJava(jdouble f);

DLLLOCAL java::lang::Object *toJava(const QoreString &str, ExceptionSink *xsink);
DLLLOCAL java::lang::Object *toJava(QoreBigIntNode &i);
DLLLOCAL java::lang::Object *toJava(QoreBoolNode &b);
DLLLOCAL java::lang::Object *toJava(QoreFloatNode &f);

// default conversions based on Qore type
DLLLOCAL java::lang::Object *toJava(const AbstractQoreNode *n, ExceptionSink *xsink);

DLLLOCAL void getQoreString(java::lang::String *jstr, QoreString &qstr);

DLLLOCAL QoreStringNode *getJavaExceptionMessage(java::lang::Throwable *t);
DLLLOCAL void getQoreException(java::lang::Throwable *t, ExceptionSink &xsink);

#endif
