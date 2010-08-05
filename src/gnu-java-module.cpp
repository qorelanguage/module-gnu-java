/* indent-tabs-mode: nil -*- */
/*
  gnu-java Qore module

  Copyright (C) 2010 David Nichols, all rights reserved

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

#include <java/lang/ClassLoader.h>
#include <java/lang/reflect/Method.h>

#include <java/lang/ClassNotFoundException.h>

#include <java/lang/Byte.h>
#include <java/lang/Short.h>
#include <java/lang/Integer.h>
#include <java/lang/Long.h>
#include <java/lang/Number.h>
#include <java/lang/Float.h>
#include <java/lang/Double.h>
#include <java/lang/Boolean.h>
#include <java/lang/Void.h>

//#include <java/util/Enumeration.h>
//#include <java/net/URL.h>
//#include <java/lang/Package.h>
//#include <java/util/HashMap.h>
//#include <java/util/Set.h>
//#include <java/util/Iterator.h>

//#include <gnu/gcj/runtime/BootClassLoader.h>

#include <strings.h>

// list of
static const char *bootlist[] = 
{ "java.lang.AbstractMethodError", "java.lang.AbstractStringBuffer", "java.lang.Appendable", "java.lang.ArithmeticException", 
  "java.lang.ArrayIndexOutOfBoundsException", "java.lang.ArrayStoreException", "java.lang.AssertionError", 
  "java.lang.Boolean", "java.lang.Byte", "java.lang.Character", "java.lang.CharSequence", "java.lang.Class", 
  "java.lang.ClassCastException", "java.lang.ClassCircularityError", "java.lang.ClassFormatError", "java.lang.ClassLoader", 
  "java.lang.ClassNotFoundException", "java.lang.Cloneable", "java.lang.CloneNotSupportedException", "java.lang.Comparable", 
  "java.lang.Compiler", "java.lang.Deprecated", "java.lang.Double", "java.lang.EcosProcess", "java.lang.Enum", 
  "java.lang.EnumConstantNotPresentException", "java.lang.Error", "java.lang.Exception", 
  "java.lang.ExceptionInInitializerError", "java.lang.Float", "java.lang.IllegalAccessError", 
  "java.lang.IllegalAccessException", "java.lang.IllegalArgumentException", "java.lang.IllegalMonitorStateException", 
  "java.lang.IllegalStateException", "java.lang.IllegalThreadStateException", "java.lang.IncompatibleClassChangeError", 
  "java.lang.IndexOutOfBoundsException", "java.lang.InheritableThreadLocal", "java.lang.InstantiationError", 
  "java.lang.InstantiationException", "java.lang.Integer", "java.lang.InternalError", "java.lang.InterruptedException", 
  "java.lang.Iterable", "java.lang.LinkageError", "java.lang.Long", "java.lang.Math", "java.lang.NegativeArraySizeException", 
  "java.lang.NoClassDefFoundError", "java.lang.NoSuchFieldError", "java.lang.NoSuchFieldException", 
  "java.lang.NoSuchMethodError", "java.lang.NoSuchMethodException", "java.lang.NullPointerException", "java.lang.Number", 
  "java.lang.NumberFormatException", "java.lang.Object", "java.lang.OutOfMemoryError", "java.lang.Override", 
  "java.lang.Package", "java.lang.PosixProcess", "java.lang.Process", "java.lang.ProcessBuilder", "java.lang.Readable", 
  "java.lang.Runnable", "java.lang.Runtime", "java.lang.RuntimeException", "java.lang.RuntimePermission", 
  "java.lang.SecurityException", "java.lang.SecurityManager", "java.lang.Short", "java.lang.StackOverflowError", 
  "java.lang.StackTraceElement", "java.lang.StrictMath", "java.lang.String", "java.lang.StringBuffer", 
  "java.lang.StringBuilder", "java.lang.StringIndexOutOfBoundsException", "java.lang.SuppressWarnings", "java.lang.System", 
  "java.lang.Thread", "java.lang.ThreadDeath", "java.lang.ThreadGroup", "java.lang.ThreadLocal", "java.lang.ThreadLocalMap", 
  "java.lang.Throwable", "java.lang.TypeNotPresentException", "java.lang.UnknownError", "java.lang.UnsatisfiedLinkError", 
  "java.lang.UnsupportedClassVersionError", "java.lang.UnsupportedOperationException", "java.lang.VerifyError", 
  "java.lang.VirtualMachineError", "java.lang.VMClassLoader", "java.lang.VMCompiler", "java.lang.VMDouble", 
  "java.lang.VMFloat", "java.lang.VMProcess", "java.lang.VMThrowable", "java.lang.Void"
#ifdef _WIN32
  , "java.lang.Win32Process"
#endif
 };

#define BOOT_LEN (sizeof(bootlist) / sizeof(const char *))

QoreStringNode *gnu_java_module_init();
void gnu_java_module_ns_init(QoreNamespace *rns, QoreNamespace *qns);
void gnu_java_module_delete();

// qore module symbols
DLLEXPORT char qore_module_name[] = "gnu-java";
DLLEXPORT char qore_module_version[] = PACKAGE_VERSION;
DLLEXPORT char qore_module_description[] = "provides java functionality through gnu java (http://gcc.gnu.org/java/)";
DLLEXPORT char qore_module_author[] = "David Nichols";
DLLEXPORT char qore_module_url[] = "http://qore.org";
DLLEXPORT int qore_module_api_major = QORE_MODULE_API_MAJOR;
DLLEXPORT int qore_module_api_minor = QORE_MODULE_API_MINOR;
DLLEXPORT qore_module_init_t qore_module_init = gnu_java_module_init;
DLLEXPORT qore_module_ns_init_t qore_module_ns_init = gnu_java_module_ns_init;
DLLEXPORT qore_module_delete_t qore_module_delete = gnu_java_module_delete;
DLLEXPORT qore_license_t qore_module_license = QL_LGPL;

// parent namespace for gnu-java module functionality
QoreNamespace gns("gnu");

QoreJavaClassMap qjcm;

QoreClass *QoreJavaClassMap::createQoreClass(const char *name, java::lang::Class *jc) {
   const char *sn = rindex(name, '.');
   if (!sn)
      sn = name;

   QoreClass *qc = new QoreClass(sn);
   // save pointer to java class info in QoreClass
   qc->setUserData(jc);

   // add to class maps
   add_intern(name, jc, qc);

   return qc;
}

AbstractQoreNode *exec_java(const QoreMethod &qm, const type_vec_t &typeList, java::lang::reflect::Method *method, QoreObject *self, QoreJavaPrivateData *jobj, const QoreListNode *args, ExceptionSink *xsink) {
   JArray<jclass> *params = method->getParameterTypes();

   // make argument array
   jobjectArray jargs = 0;
   if (params->length) {
      jargs = JvNewObjectArray(params->length, &java::lang::Object::class$, NULL);

      jclass *dparam = elements(params);
      for (int i = 0; i < params->length; ++i) {
	 elements(jargs)[i] = qoreToJava(dparam[i], args ? args->retrieve_entry(i) : 0, xsink);
	 if (*xsink)
	    return 0;
      }
   }

   java::lang::Object *jrv = method->invoke(jobj->getObject(), jargs);

   return javaToQore(jrv, xsink);
}

void QoreJavaClassMap::populateQoreClass(const char *name) {
   const char *sn = rindex(name, '.');
   if (!sn)
      sn = name;

   QoreClass *qc = jcmap[name];
   assert(qc);

   java::lang::Class *jc = (java::lang::Class*)qc->getUserData();

   //printd(0, "QoreJavaClassMap::populateQoreClass() %s qc=%p jc=%p\n", name, qc, jc);

   JArray<java::lang::reflect::Method *> *methods = jc->getDeclaredMethods();
   for (int i = 0; i < methods->length; ++i) {
      java::lang::reflect::Method *m = elements(methods)[i];

      QoreString mname;
      getQoreString(m->getName(), mname);

#ifdef DEBUG
      QoreString mstr;
      getQoreString(m->toString(), mstr);
      printd(5, "  + adding %s.%s() (%s)\n", name, mname.getBuffer(), mstr.getBuffer());
#endif

      // get and map method's return type
      bool err;
      const QoreTypeInfo *returnTypeInfo = getQoreType(m->getReturnType(), err);
      if (err) {
	 printd(0, "  + skipping %s.%s() (%s); unsupported return type\n", name, mname.getBuffer(), mstr.getBuffer());
	 continue;
      }

      // get and map method's parameter types
      JArray<jclass> *params = m->getParameterTypes();

      type_vec_t argTypeInfo;
      argTypeInfo.reserve(params->length);

      for (int i = 0; i < params->length; ++i) {
	 const QoreTypeInfo *typeInfo = getQoreType(elements(params)[0], err);
	 if (err) {
	    printd(0, "  + skipping %s.%s() (%s); unsupported parameter type for arg %d\n", name, mname.getBuffer(), mstr.getBuffer(), i + 1);
	    break;
	 }

	 argTypeInfo.push_back(typeInfo);
      }

      if (err)
	 continue;

      bool priv = m->getModifiers() & java::lang::reflect::Modifier::PRIVATE;

      if (!(m->getModifiers() & java::lang::reflect::Modifier::STATIC)) 
	 ;
      else
	 qc->addMethodExtendedList3(m, mname.getBuffer(), (q_method3_t)exec_java, priv, QC_NO_FLAGS, QDOM_GUI, returnTypeInfo, argTypeInfo);
   }
}

const QoreTypeInfo *QoreJavaClassMap::getQoreType(java::lang::Class *jc, bool &err) const {
   err = false;
   if (jc == JvPrimClass(void)
       || jc == &java::lang::Void::class$)
      return nothingTypeInfo;

   if (jc == JvPrimClass(char)
       || jc == &java::lang::String::class$)
      return stringTypeInfo;

   if (jc == JvPrimClass(byte)
       || jc == JvPrimClass(short)
       || jc == JvPrimClass(int)
       || jc == JvPrimClass(long)
       || jc == &java::lang::Byte::class$
       || jc == &java::lang::Short::class$
       || jc == &java::lang::Integer::class$
       || jc == &java::lang::Long::class$
      )
      return bigIntTypeInfo;

   if (jc == JvPrimClass(float)
       || jc == JvPrimClass(double)
       || jc == &java::lang::Float::class$
       || jc == &java::lang::Double::class$
       || jc == &java::lang::Number::class$
      )
      return floatTypeInfo;

   if (jc == JvPrimClass(boolean)
       || jc == &java::lang::Boolean::class$)
      return boolTypeInfo;

   if (jc == &java::lang::Object::class$)
      return 0;

   OptLocker ol(init_done ? &m : 0);
   jcpmap_t::const_iterator i = jcpmap.find(jc);

   if (i != jcpmap.end())
      return i->second->getTypeInfo();

#ifdef DEBUG
   QoreString cname;
   getQoreString(jc->getName(), cname);

   printd(0, "QoreJavaClassMap::getQoreType() cannot map type %p '%s'\n", jc, cname.getBuffer());
#endif

   err = true;
   return 0;
}

QoreStringNode *gnu_java_module_init() {
   // create java namespace
   QoreNamespace *jns = new QoreNamespace("java");

   // create lang namespace
   QoreNamespace *lang = new QoreNamespace("lang");
   jns->addNamespace(lang);

   gns.addNamespace(jns);

   try {
      // initialize JVM
      JvCreateJavaVM(0);
	 
      // attach to thread
      QoreJavaThreadHelper jth;

      java::lang::ClassLoader *loader = java::lang::ClassLoader::getSystemClassLoader();

      printd(0, "gnu_java_module_init() loader=%p, boot len=%d\n", loader, BOOT_LEN);

      // first create QoreClass'es first
      for (unsigned i = 0; i < BOOT_LEN; ++i) {
	 jclass jc;
	 try {
	    jc = loader->loadClass(JvNewStringLatin1(bootlist[i]));
	 }
	 catch (java::lang::ClassNotFoundException *e) {
	    printd(0, "+ ERROR: %s: ClassNotFoundException\n", bootlist[i]);
	    continue;
	 }

	 //printd(5, "+ creating %s from %p\n", bootlist[i], jc);
	 QoreClass *qc = qjcm.createQoreClass(bootlist[i], jc);
	 lang->addSystemClass(qc);
      }

      qjcm.populateCoreClasses();

      /*
      
      java::util::HashMap *cm = loader->loadedClasses;
      java::util::Set *set = cm->keySet();
      java::util::Iterator *i = set->iterator();

      printd(0, "system class loader: %p, classmap=%p, set=%p, i=%p\n", loader, cm, set, i);
      
      while (i->hasNext()) {
	 //java::lang::Object *obj = i->next();
	 //java::lang::Class *jc = obj->getClass();
	 java::lang::String *jstr = (java::lang::String *)i->next();
	 QoreString str;
	 getQoreString(jstr, str);	 
	 printd(0, "+ %p: %s\n", jstr, str.getBuffer());
      }

      java::util::Enumeration *resources = java::lang::ClassLoader::getSystemResources(JvNewStringLatin1("java/lang"));
      while (resources->hasMoreElements()) {
	 java::net::URL *url = (java::net::URL *)resources->nextElement();
	 QoreString str;
	 getQoreString(url->toString(), str);
	 printd(0, " + %p: %s\n", url, str.getBuffer());
      }
      printd(0, "system class loader: %p\n", loader);

      */
      //java::lang::Package *jpkg = java::lang::Package::getPackage(JvNewStringLatin1("java.lang"));
      //printd(0, "java.lang package: %p\n", jpkg);
   }
   catch (java::lang::Throwable *t) {
      // the JVM has not been initialized here, so we cannot get the exception message reliably
      QoreStringNode *err = new QoreStringNode("error initializing gnu JVM");
      return err;
   }

   return 0;
}

void gnu_java_module_ns_init(QoreNamespace *rns, QoreNamespace *qns) {
   qns->addNamespace(gns.copy());
}

void gnu_java_module_delete() {
}
