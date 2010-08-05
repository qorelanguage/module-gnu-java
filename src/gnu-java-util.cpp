/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  gnu-java-util.cpp

  Qore Programming Language gnu-java Module

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

#include "gnu-java-module.h"

#include <java/lang/Throwable.h>
#include <java/lang/StackTraceElement.h>
#include <java/lang/Long.h>
#include <java/lang/Integer.h>
#include <java/lang/Short.h>
#include <java/lang/Byte.h>
#include <java/lang/Boolean.h>
#include <java/lang/Float.h>
#include <java/lang/Double.h>
#include <java/lang/Character.h>
#include <java/lang/String.h>
#include <java/lang/Void.h>

//#include <java/lang/System.h>
//#include <java/lang/reflect/Method.h>
//#include <java/lang/ClassNotFoundException.h>
//#include <java/lang/ClassLoader.h>

AbstractQoreNode *javaToQore(java::lang::Object *jobj, ExceptionSink *xsink) {
   if (!jobj)
      return 0;

   jclass jc = jobj->getClass();

   if (jc == &java::lang::String::class$)
      return javaToQore((jstring)jobj);

   if (jc == &java::lang::Long::class$)
      return javaToQore(((java::lang::Long *)jobj)->longValue());

   if (jc == &java::lang::Integer::class$)
      return javaToQore(((java::lang::Integer *)jobj)->longValue());

   if (jc == &java::lang::Short::class$)
      return javaToQore(((java::lang::Short *)jobj)->longValue());

   if (jc == &java::lang::Byte::class$)
      return javaToQore(((java::lang::Byte *)jobj)->longValue());

   if (jc == &java::lang::Boolean::class$)
      return javaToQore(((java::lang::Boolean *)jobj)->booleanValue());

   if (jc == &java::lang::Double::class$)
      return javaToQore(((java::lang::Double *)jobj)->doubleValue());

   if (jc == &java::lang::Float::class$)
      return javaToQore(((java::lang::Float *)jobj)->doubleValue());

   if (jc == &java::lang::Character::class$)
      return javaToQore(((java::lang::Character *)jobj)->charValue());

   QoreString cname;
   getQoreString(jobj->getClass()->getName(), cname);

   xsink->raiseException("JAVA-UNSUPPORTED-TYPE", "cannot convert from Java class '%s' to a Qore value", cname.getBuffer());
   return 0;
}

java::lang::Object *qoreToJava(java::lang::Class *jc, const AbstractQoreNode *n, ExceptionSink *xsink) {
   // handle NULL pointers first
   if (!n)
      return 0;

   if (jc == &java::lang::String::class$) {
      QoreStringValueHelper str(n, QCS_UTF8, xsink);
      if (*xsink)
	 return 0;

      return JvNewStringUTF(str->getBuffer());
   }

   if (jc == &java::lang::Long::class$)
      return toJava((jlong)n->getAsBigInt());

   if (jc == &java::lang::Integer::class$)
      return new java::lang::Integer((jint)n->getAsInt());

   if (jc == &java::lang::Short::class$)
      return new java::lang::Short((jshort)n->getAsInt());

   if (jc == &java::lang::Byte::class$)
      return new java::lang::Byte((jbyte)n->getAsInt());

   if (jc == &java::lang::Boolean::class$)
      return toJava((jboolean)n->getAsBool());

   if (jc == &java::lang::Double::class$)
      return toJava((jdouble)n->getAsFloat());

   if (jc == &java::lang::Float::class$)
      return toJava((jfloat)n->getAsFloat());

   if (jc == &java::lang::Character::class$) {
      QoreStringValueHelper str(n, QCS_UTF8, xsink);
      if (*xsink)
	 return 0;

      return new java::lang::Character((jchar)str->getUnicodePointFromUTF8(0));
   }

   QoreString cname;
   getQoreString(jc->getName(), cname);

   xsink->raiseException("JAVA-UNSUPPORTED-TYPE", "cannot convert from Qore '%s' to Java '%s'", get_type_name(n), cname.getBuffer());
   return 0;   
}

AbstractQoreNode *javaToQore(jbyte i) {
   return new QoreBigIntNode(i);
}

AbstractQoreNode *javaToQore(jshort i) {
   return new QoreBigIntNode(i);
}

AbstractQoreNode *javaToQore(jint i) {
   return new QoreBigIntNode(i);
}

AbstractQoreNode *javaToQore(jlong i) {
   return new QoreBigIntNode(i);
}

AbstractQoreNode *javaToQore(jboolean b) {
   return get_bool_node(b);
}

AbstractQoreNode *javaToQore(jfloat f) {
   return new QoreFloatNode(f);
}

AbstractQoreNode *javaToQore(jdouble f) {
   return new QoreFloatNode(f);
}

AbstractQoreNode *javaToQore(jchar c) {
   QoreStringNode *str = new QoreStringNode(QCS_UTF8);
   str->concatUTF8FromUnicode(c);
   return str;
}

AbstractQoreNode *javaToQore(java::lang::String *jstr) {
   if (!jstr)
      return 0;

   QoreStringNode *str = new QoreStringNode(QCS_UTF8);

   int size = JvGetStringUTFLength(jstr);
   
   str->allocate(size + 1);
   JvGetStringUTFRegion(jstr, 0, jstr->length(), (char *)str->getBuffer());
   str->terminate(size);
   return str;
}

java::lang::Object *toJava(const char *str) {
   return JvNewStringLatin1(str);
}

java::lang::Object *toJava(jlong i) {
   return new java::lang::Long(i);
}

java::lang::Object *toJava(jboolean b) {
   return new java::lang::Boolean(b);
}

java::lang::Object *toJava(jfloat f) {
   return new java::lang::Float(f);
}

java::lang::Object *toJava(jdouble f) {
   return new java::lang::Double(f);
}

java::lang::Object *toJava(QoreStringNode &str, ExceptionSink *xsink) {
   if (str.getEncoding() == QCS_UTF8)
      return JvNewStringUTF(str.getBuffer());
   
   if (str.getEncoding() == QCS_ISO_8859_1)
      return JvNewStringLatin1(str.getBuffer());

   TempEncodingHelper ustr(str, QCS_UTF8, xsink);
   if (!ustr)
      return 0;

   return JvNewStringUTF(ustr->getBuffer());
}

java::lang::Object *toJava(QoreBigIntNode &i) {
   return new java::lang::Long(i.val);
}

java::lang::Object *toJava(QoreBoolNode &b) {
   return new java::lang::Boolean(b.getValue());
}

java::lang::Object *toJava(QoreFloatNode &f) {
   return new java::lang::Double((jdouble)f.f);
}

void getQoreString(java::lang::String *jstr, QoreString &qstr) {
   int size = JvGetStringUTFLength(jstr);
   qstr.clear();
   qstr.allocate(size + 1);
   JvGetStringUTFRegion(jstr, 0, jstr->length(), (char *)qstr.getBuffer());
   qstr.setEncoding(QCS_UTF8);
   qstr.terminate(size);
}

void appendQoreString(java::lang::String *jstr, QoreString &qstr) {
   int size = JvGetStringUTFLength(jstr);
   qstr.allocate(qstr.strlen() + size + 1);
   JvGetStringUTFRegion(jstr, 0, jstr->length(), (char *)qstr.getBuffer() + qstr.strlen());
   qstr.terminate(qstr.strlen() + size);
}

QoreStringNode *getJavaExceptionMessage(java::lang::Throwable *t) {
   //printd(0, "getJavaExceptionMessage() t=%p\n", t);

   QoreStringNode *desc = new QoreStringNode;
   getQoreString(t->getMessage(), *desc);

   return desc;
}

void getQoreException(java::lang::Throwable *t, ExceptionSink &xsink) {
   //fprintf(stderr, "Unhandled Java exception %p:\n", t);

   xsink.raiseException("JAVA-EXCEPTION", getJavaExceptionMessage(t));
   //fprintf(stderr, "%s\n", str.getBuffer());

   /*
   // get stack trace
   JArray<java::lang::StackTraceElement *> *stack = t->getStackTrace();
   
   java::lang::StackTraceElement **e = elements(stack);
   for (int i = 0; i < stack->length; ++i) {
      QoreString file, cls, meth;
      getQoreString(e[i]->getFileName(), file);
      getQoreString(e[i]->getClassName(), cls);
      getQoreString(e[i]->getMethodName(), meth);

      int line = e[i]->getLineNumber();

      fprintf(stderr, "%s:%d: %s::%s() (%s)\n", file.getBuffer(), line > 0 ? line : 0, cls.getBuffer(), meth.getBuffer(), e[i]->isNativeMethod() ? "native" : "java");
   }
   */
}
