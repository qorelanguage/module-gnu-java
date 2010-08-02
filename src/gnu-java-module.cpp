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
  "java.lang.VMFloat", "java.lang.VMProcess", "java.lang.VMThrowable", "java.lang.Void", "java.lang.Win32Process"
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

QoreClass *getQoreClass(const char *name, java::lang::Class *jc) {
   const char *sn = rindex(name, '.');
   if (!sn)
      sn = name;

   QoreClass *qc = new QoreClass(sn);

   JArray<java::lang::reflect::Method *> *methods = jc->getDeclaredMethods();
   for (int i = 0; i < methods->length; ++i) {
      java::lang::reflect::Method *m = elements(methods)[i];

      QoreString mname;
      getQoreString(m->getName(), mname);

#ifdef DEBUG
      QoreString mstr;
      getQoreString(m->toString(), mstr);
      printd(0, "adding %s.%s() (%s)\n", name, mname.getBuffer(), mstr.getBuffer());
#endif

      //jclass mrt = m->getReturnType();
   }

   return qc;
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

      for (unsigned i = 0; i < BOOT_LEN; ++i) {
	 jclass jc = loader->findClass(JvNewStringLatin1(bootlist[i]));

	 printd(0, "+ creating %s from %p\n", bootlist[i], jc);
	 QoreClass *qc = getQoreClass(bootlist[i], jc);
	 if (!qc) {
	    QoreStringNode *err = new QoreStringNode;
	    err->sprintf("error initializing boot class %s", bootlist[i]);
	    return err;
	 }

	 lang->addSystemClass(qc);
      }

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
      QoreStringNode *err = new QoreStringNode;
      // the JVM has not been initialized here, can only get the exception message, no stack trace
      err->sprintf("error initializing gnu JVM: %s", getJavaExceptionMessage(t));
      return err;
   }

   return 0;
}

void gnu_java_module_ns_init(QoreNamespace *rns, QoreNamespace *qns) {
   qns->addNamespace(gns.copy());
}

void gnu_java_module_delete() {
}
