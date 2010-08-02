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
//#include <java/util/Enumeration.h>
//#include <java/net/URL.h>
#include <java/lang/Package.h>

#include <java/util/HashMap.h>
#include <java/util/Set.h>
#include <java/util/Iterator.h>

//#include <gnu/gcj/runtime/BootClassLoader.h>

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

QoreClass *getQoreClass(java::lang::Class *jc) {
   return 0;
}

QoreStringNode *gnu_java_module_init() {
   // create java namespace
   QoreNamespace *javans = new QoreNamespace("java");

   // create lang namespace
   QoreNamespace *lang = new QoreNamespace("lang");
   javans->addNamespace(lang);

   gns.addNamespace(javans);

   try {
      // initialize JVM
      JvCreateJavaVM(0);
	 
      // attach to thread
      QoreJavaThreadHelper jth;

      java::lang::ClassLoader *loader = java::lang::ClassLoader::getSystemClassLoader();
      
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

      /*
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

   //gns.addSystemClass(QC_GCJ);

   return 0;
}

void gnu_java_module_ns_init(QoreNamespace *rns, QoreNamespace *qns) {
   qns->addNamespace(gns.copy());
}

void gnu_java_module_delete() {
}
