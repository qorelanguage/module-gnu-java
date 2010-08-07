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
#include <java/lang/reflect/Constructor.h>

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
#include <java/lang/Character.h>

//#include <java/util/Enumeration.h>
//#include <java/net/URL.h>
//#include <java/lang/Package.h>
//#include <java/util/HashMap.h>
//#include <java/util/Set.h>
//#include <java/util/Iterator.h>

//#include <gnu/gcj/runtime/BootClassLoader.h>

#include <strings.h>

#ifdef NEED_BOEHM_SIGNALS
#include <signal.h>
#include <pthread.h>
static int boehm_sigs[] = { NEED_BOEHM_SIGNALS };
#define NUM_BOEHM_SIGS (sizeof(boehm_sigs) / sizeof(int))
#endif

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
#if 1//0 // for testing only
  , "java.util.Vector", "java.util.HashMap", "java.net.URL",
  "org.apache.xmlrpc.client.XmlRpcClient",
  "org.apache.xmlrpc.client.XmlRpcClientConfigImpl",
  "org.apache.xmlrpc.XmlRpcException"
#endif
 };

#define BOOT_LEN (sizeof(bootlist) / sizeof(const char *))

qore_classid_t CID_OBJECT;

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

QoreJavaClassMap qjcm;

void QoreJavaClassMap::addSuperClass(QoreClass &qc, java::lang::Class *jsc) {
#ifdef DEBUG
   //QoreString sn;
   //getQoreString(jsc->getName(), sn);
   //printd(0, "QoreJavaClassMap::createQoreClass() %s has super class %p %s\n", qc.getName(), jsc, sn.getBuffer());
#endif

   qc.addBuiltinVirtualBaseClass(findCreate(jsc));
}

// creates a QoreClass and adds it in the appropriate namespace
QoreClass *QoreJavaClassMap::createQoreClass(const char *name, java::lang::Class *jc) {
   QoreClass *qc = find(jc);
   if (qc)
      return qc;

   const char *sn = rindex(name, '.');

   QoreNamespace *ns;
   // find parent namespace
   if (!sn) {
      ns = &gns;
      sn = name;
   }
   else {
      QoreString nsn(name, sn - name);
      nsn.replaceAll(".", "::");
      
      ++sn;

      ns = gns.findCreateNamespacePath(nsn.getBuffer());
   }

   qc = new QoreClass(sn);
   // save pointer to java class info in QoreClass
   qc->setUserData(jc);

   // save class in namespace
   ns->addSystemClass(qc);

   //printd(5, "QoreJavaClassMap::createQoreClass() qc=%p %s::%s\n", qc, ns->getName(), sn);

   // add to class maps
   add_intern(name, jc, qc);

   // get and process superclass
   java::lang::Class *jsc = jc->getSuperclass();

   // add superclass
   if (jsc)
      addSuperClass(*qc, jsc);

   // add interface classes as superclasses
   JArray<jclass> *ifc = jc->getInterfaces();
   if (ifc && ifc->length) {      
      for (int i = 0; i < ifc->length; ++i)
	 addSuperClass(*qc, elements(ifc)[i]);
   }

   populateQoreClass(*qc, jc);

   return qc;
}

static jobjectArray get_java_args(JArray<jclass> *params, const QoreListNode *args, ExceptionSink *xsink) {
   //printd(0, "get_java_args() params=%p (%d) args=%p (%d)\n", params, params->length, args, args ? args->size() : 0);

   // make argument array
   jobjectArray jargs = 0;
   if (params->length) {
      jargs = JvNewObjectArray(params->length, &java::lang::Object::class$, NULL);

      jclass *dparam = elements(params);
      for (int i = 0; i < params->length; ++i) {
	 elements(jargs)[i] = qjcm.toJava(dparam[i], args ? args->retrieve_entry(i) : 0, xsink);
	 if (*xsink)
	    return 0;
      }
   }

   return jargs;
}
/*
struct jc_t {
   java::lang::reflect::Constructor *method;
   JArray<jclass> *params;

   DLLLOCAL jc_t(java::lang::reflect::Constructor *n_method, JArray<jclass> *n_params) : method(n_method), params(n_params) {
   }
};
*/

static void exec_java_constructor(const QoreClass &qc, const type_vec_t &typeList, void *ip, QoreObject *self, const QoreListNode *args, ExceptionSink *xsink) {
   // attach to thread
   QoreJavaThreadHelper jth;

   // get java args
   try {
      int i = (long)ip;
      jclass jc = (jclass)qc.getUserData();
      JArray<java::lang::reflect::Constructor *> *methods = jc->getDeclaredConstructors(false);
      java::lang::reflect::Constructor *method = elements(methods)[i];

      //printd(0, "exec_java_constructor() %s::constructor() method=%p\n", qc.getName(), method);

      JArray<jclass> *params = method->getParameterTypes();
      //printd(0, "exec_java_constructor() %s::constructor() method=%p params=%p (%d\n", qc.getName(), method);

      jobjectArray jargs = get_java_args(params, args, xsink);

      if (*xsink)
	 return;

      java::lang::Object *jobj = method->newInstance(jargs);
      //printd(0, "exec_java_constructor() %s::constructor() method=%p jobj=%p\n", qc.getName(), method, jobj);

      if (!jobj)
	 xsink->raiseException("JAVA-CONSTRUCTOR-ERROR", "%s::constructor() did not return any object", qc.getName());
      else {
	 QoreJavaPrivateData *pd = new QoreJavaPrivateData(jobj);
	 //printd(0, "exec_java_constructor() %s::constructor() private data=%p, jobj=%p (%p)\n", qc.getName(), pd, pd->getObject(), jobj);
	 self->setPrivate(qc.getID(), pd);
      }
   }
   catch (java::lang::Throwable *e) {
      getQoreException(e, *xsink);
   }
}

//static AbstractQoreNode *exec_java_static(const QoreMethod &qm, const type_vec_t &typeList, java::lang::reflect::Method *method, const QoreListNode *args, ExceptionSink *xsink) {
static AbstractQoreNode *exec_java_static(const QoreMethod &qm, const type_vec_t &typeList, void *ip, const QoreListNode *args, ExceptionSink *xsink) {
   // attach to thread
   QoreJavaThreadHelper jth;

   // get java args
   try {
      int i = (long)ip;
      jclass jc = (jclass)qm.getClass()->getUserData();
      JArray<java::lang::reflect::Method *> *methods = jc->getDeclaredMethods();
      java::lang::reflect::Method *method = elements(methods)[i];

      JArray<jclass> *params = method->getParameterTypes();
      // get java args
      jobjectArray jargs = get_java_args(params, args, xsink);
      if (*xsink)
	 return 0;

      java::lang::Object *jrv = method->invoke(0, jargs);

      return qjcm.toQore(jrv, xsink);
   }
   catch (java::lang::Throwable *e) {
      getQoreException(e, *xsink);
   }
   return 0;
}

//static AbstractQoreNode *exec_java(const QoreMethod &qm, const type_vec_t &typeList, java::lang::reflect::Method *method, QoreObject *self, QoreJavaPrivateData *pd, const QoreListNode *args, ExceptionSink *xsink) {
static AbstractQoreNode *exec_java(const QoreMethod &qm, const type_vec_t &typeList, void *ip, QoreObject *self, QoreJavaPrivateData *pd, const QoreListNode *args, ExceptionSink *xsink) {
   // attach to thread
   QoreJavaThreadHelper jth;

   // get java args
   try {
      int i = (long)ip;
      jclass jc = (jclass)qm.getClass()->getUserData();
      JArray<java::lang::reflect::Method *> *methods = jc->getDeclaredMethods();
      java::lang::reflect::Method *method = elements(methods)[i];

#ifdef DEBUG
      QoreString mname;
      getQoreString(method->toString(), mname);
      //printd(0, "exec_java() %s::%s() %s method=%p args=%p (%d)\n", qm.getClassName(), qm.getName(), mname.getBuffer(), method, args, args ? args->size() : 0);
#endif

      JArray<jclass> *params = method->getParameterTypes();
      // get java args
      jobjectArray jargs = get_java_args(params, args, xsink);
      if (*xsink)
	 return 0;

      jobject jobj = pd->getObject();
      //printd(0, "exec_java() %s::%s() pd=%p jobj=%p args=%p (%d) jargs=%p (%d)\n", qm.getClassName(), qm.getName(), pd, jobj, args, args ? args->size() : 0, jargs, jargs ? jargs->length : 0);
      java::lang::Object *jrv = method->invoke(jobj, jargs);

      return qjcm.toQore(jrv, xsink);
   }
   catch (java::lang::Throwable *e) {
      getQoreException(e, *xsink);
   }
   return 0;
}

int QoreJavaClassMap::getArgTypes(type_vec_t &argTypeInfo, JArray<jclass> *params) {
   argTypeInfo.reserve(params->length);

   for (int i = 0; i < params->length; ++i) {
      java::lang::Class *jc = elements(params)[i];

      bool err;
      const QoreTypeInfo *typeInfo = getQoreType(jc, err);
      if (err) 
	 return -1;

      //printd(0, "QoreJavaClassMap::getArgTypes() jc=%p (%s), qore=%s\n", );

      argTypeInfo.push_back(typeInfo);
   }
   return 0;
}

void QoreJavaClassMap::doConstructors(QoreClass &qc, java::lang::Class *jc) {
   // get constructor methods
   JArray<java::lang::reflect::Constructor *> *methods = jc->getDeclaredConstructors(false);

   for (int i = 0; i < methods->length; ++i) {
      java::lang::reflect::Constructor *m = elements(methods)[i];

#ifdef DEBUG
      QoreString mstr;
      getQoreString(m->toString(), mstr);
      //if (!strcmp(qc.getName(), "URL"))
      //printd(0, "  + adding %s.constructor() (%s) m=%p\n", qc.getName(), mstr.getBuffer(), m);
#endif

      // get parameter type array
      JArray<jclass> *params = m->getParameterTypes();

      // get method's parameter types
      type_vec_t argTypeInfo;
      if (getArgTypes(argTypeInfo, params)) {
	 printd(0, "  + skipping %s.constructor() (%s); unsupported parameter type for arg %d\n", qc.getName(), mstr.getBuffer(), i + 1);
	 continue;
      }

      bool priv = m->getModifiers() & java::lang::reflect::Modifier::PRIVATE;
      int64 flags = QC_NO_FLAGS;
      if (m->isVarArgs())
	 flags |= QC_USES_EXTRA_ARGS;

      const QoreMethod *qm = qc.getConstructor();
      if (qm && qm->existsVariant(argTypeInfo)) {
         //printd(0, "QoreJavaClassMap::doConstructors() skipping already-created variant %s::constructor()\n", qc.getName());
	 continue;
      }

      qc.setConstructorExtendedList3((void*)i, (q_constructor3_t)exec_java_constructor, priv, flags, QDOM_DEFAULT, argTypeInfo);
   }
}

void QoreJavaClassMap::doMethods(QoreClass &qc, java::lang::Class *jc) {
   //printd(0, "QoreJavaClassMap::doMethods() %s qc=%p jc=%p\n", name, qc, jc);

   JArray<java::lang::reflect::Method *> *methods = jc->getDeclaredMethods();
   for (int i = 0; i < methods->length; ++i) {
      java::lang::reflect::Method *m = elements(methods)[i];

      QoreString mname;
      getQoreString(m->getName(), mname);

      assert(mname.strlen());

#ifdef DEBUG
      QoreString mstr;
      getQoreString(m->toString(), mstr);
      //printd(5, "  + adding %s.%s() (%s)\n", qc.getName(), mname.getBuffer(), mstr.getBuffer());
#endif

      // get and map method's return type
      bool err;
      const QoreTypeInfo *returnTypeInfo = getQoreType(m->getReturnType(), err);
      if (err) {
	 printd(0, "  + skipping %s.%s() (%s); unsupported return type\n", qc.getName(), mname.getBuffer(), mstr.getBuffer());
	 continue;
      }

      // get method's parameter types
      type_vec_t argTypeInfo;
      if (getArgTypes(argTypeInfo, m->getParameterTypes())) {
	 printd(0, "  + skipping %s.%s() (%s); unsupported parameter type for arg %d\n", qc.getName(), mname.getBuffer(), mstr.getBuffer(), i + 1);
	 continue;
      }

      bool priv = m->getModifiers() & java::lang::reflect::Modifier::PRIVATE;
      int64 flags = QC_NO_FLAGS;
      if (m->isVarArgs())
	 flags |= QC_USES_EXTRA_ARGS;

      if ((m->getModifiers() & java::lang::reflect::Modifier::STATIC)) {
	 const QoreMethod *qm = qc.findLocalStaticMethod(mname.getBuffer());
	 if (qm && qm->existsVariant(argTypeInfo)) {
	    //printd(0, "QoreJavaClassMap::doMethods() skipping already-created variant %s::%s()\n", qc.getName(), mname.getBuffer());
	    continue;
	 }

	 printd(5, "  + adding static %s%s::%s() (%s) qc=%p\n", priv ? "private " : "", qc.getName(), mname.getBuffer(), mstr.getBuffer(), &qc);
	 qc.addStaticMethodExtendedList3((void *)i, mname.getBuffer(), (q_static_method3_t)exec_java_static, priv, flags, QDOM_DEFAULT, returnTypeInfo, argTypeInfo);      
      }
      else {
	 const QoreMethod *qm = qc.findLocalMethod(mname.getBuffer());
	 if (qm && qm->existsVariant(argTypeInfo)) {
	    //printd(0, "QoreJavaClassMap::doMethods() skipping already-created variant %s::%s()\n", qc.getName(), mname.getBuffer());
	    continue;
	 }

	 printd(5, "  + adding %s%s::%s() (%s) qc=%p\n", priv ? "private " : "", qc.getName(), mname.getBuffer(), mstr.getBuffer(), &qc);
	 qc.addMethodExtendedList3((void *)i, mname.getBuffer(), (q_method3_t)exec_java, priv, flags, QDOM_DEFAULT, returnTypeInfo, argTypeInfo);
      }
   }
}

void QoreJavaClassMap::populateQoreClass(QoreClass &qc, java::lang::Class *jc) {
   // do constructors
   doConstructors(qc, jc);

   // do methods
   doMethods(qc, jc);
}

const QoreTypeInfo *QoreJavaClassMap::getQoreType(java::lang::Class *jc, bool &err) {
   err = false;
   if (jc->isArray())
      return listTypeInfo;

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

   return findCreate(jc)->getTypeInfo();
/*
   //OptLocker ol(init_done ? &m : 0);
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
*/
}

// Qore::toQore(java::lang::Object $obj) returns any
static AbstractQoreNode *f_toQore(const QoreListNode *args, ExceptionSink *xsink) {
   HARD_QORE_OBJ_DATA(jobj, QoreJavaPrivateData, args, 0, CID_OBJECT, "java::lang::Object", "Qore::toQore", xsink);
   if (*xsink)
      return 0;

   return qjcm.toQore(jobj->getObject(), xsink);
}

void QoreJavaClassMap::addQoreClass() {
   // get class for java::lang::Object
   QoreClass *joc = find(&java::lang::Object::class$);
   assert(joc);

   CID_OBJECT = joc->getID();

   QoreClass *qc = new QoreClass("Qore");
   qc->addStaticMethodExtended("toQore", f_toQore, false, QC_NO_FLAGS, QDOM_DEFAULT, anyTypeInfo, 1, joc->getTypeInfo(), QORE_PARAM_NO_ARG);

   gns.addSystemClass(qc);
}

void QoreJavaClassMap::init() {
   java::lang::ClassLoader *loader = java::lang::ClassLoader::getSystemClassLoader();

   printd(5, "QoreJavaClassMap::init() loader=%p, boot len=%d\n", loader, BOOT_LEN);

   // first create QoreClass'es first
   for (unsigned i = 0; i < BOOT_LEN; ++i) {
      jclass jc;
      try {
	 jc = loader->loadClass(JvNewStringLatin1(bootlist[i]));
      }
      catch (java::lang::ClassNotFoundException *e) {
	 printd(0, "ERROR: cannot map %s: ClassNotFoundException\n", bootlist[i]);
	 continue;
      }
      
      //printd(5, "+ creating %s from %p\n", bootlist[i], jc);
      qjcm.createQoreClass(bootlist[i], jc);
   }

   // add "Qore" class to gnu namespace
   addQoreClass();
}

AbstractQoreNode *QoreJavaClassMap::toQore(java::lang::Object *jobj, ExceptionSink *xsink) {
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

   QoreClass *qc = find(jc);
   if (qc)
      return new QoreObject(qc, getProgram(), new QoreJavaPrivateData(jobj));

   QoreString cname;
   getQoreString(jobj->getClass()->getName(), cname);

   xsink->raiseException("JAVA-UNSUPPORTED-TYPE", "cannot convert from Java class '%s' to a Qore value", cname.getBuffer());
   return 0;
}

java::lang::Object *QoreJavaClassMap::toJava(java::lang::Class *jc, const AbstractQoreNode *n, ExceptionSink *xsink) {
/*
#ifdef DEBUG
   QoreString pname;
   getQoreString(jc->getName(), pname);
   printd(0, "QoreJavaClassMap::toJava() jc=%p %s n=%p %s\n", jc, pname.getBuffer(), n, get_type_name(n));
#endif
*/

   // handle NULL pointers first
   if (!n)
      return 0;

   if (jc->isArray()) {
      jclass cc = jc->getComponentType();
      if (n && n->getType() == NT_LIST) {
         const QoreListNode *l = reinterpret_cast<const QoreListNode *>(n);
         jobjectArray array = JvNewObjectArray(l->size(), cc, NULL);
         ConstListIterator li(l);
         while (li.next()) {
            elements(array)[li.index()] = toJava(cc, li.getValue(), xsink);
            if (*xsink)
               return 0;
         }
	 //printd(5, "QoreJavaClassMap::toJava() returning array of size %lld\n", l->size());
         return array;
      }
      else {
         jobjectArray array = JvNewObjectArray(is_nothing(n) ? 0 : 1, cc, NULL);
         if (!is_nothing(n))
            elements(array)[0] = toJava(cc, n, xsink);
         return *xsink ? 0 : array;
      }
   }

   if (jc == &java::lang::String::class$) {
      QoreStringValueHelper str(n, QCS_UTF8, xsink);
      if (*xsink)
	 return 0;

      return JvNewStringUTF(str->getBuffer());
   }

   if (jc == &java::lang::Long::class$)
      return ::toJava((jlong)n->getAsBigInt());

   if (jc == &java::lang::Integer::class$)
      return new java::lang::Integer((jint)n->getAsInt());

   if (jc == &java::lang::Short::class$)
      return new java::lang::Short((jshort)n->getAsInt());

   if (jc == &java::lang::Byte::class$)
      return new java::lang::Byte((jbyte)n->getAsInt());

   if (jc == &java::lang::Boolean::class$)
      return ::toJava((jboolean)n->getAsBool());

   if (jc == &java::lang::Double::class$)
      return ::toJava((jdouble)n->getAsFloat());

   if (jc == &java::lang::Float::class$)
      return ::toJava((jfloat)n->getAsFloat());

   if (jc == &java::lang::Character::class$) {
      QoreStringValueHelper str(n, QCS_UTF8, xsink);
      if (*xsink)
	 return 0;

      return new java::lang::Character((jchar)str->getUnicodePointFromUTF8(0));
   }

   if (jc == &java::lang::Object::class$)
      return ::toJava(n, xsink);

   // find corresponding QoreClass
   QoreClass *qc = find(jc);
   if (qc && n && n->getType() == NT_OBJECT) {
      const QoreObject *o = reinterpret_cast<const QoreObject *>(n);
      PrivateDataRefHolder<QoreJavaPrivateData> jpd(o, qc->getID(), xsink);
      if (!jpd) {
	 if (!*xsink) {
	    QoreString cname;
	    getQoreString(jc->getName(), cname);
	    xsink->raiseException("JAVA-TYPE-ERROR", "java class '%s' expected, but Qore class '%s' supplied", cname.getBuffer(), o->getClassName());
	 }
	 return 0;
      }
      return jpd->getObject();
   }

   QoreString cname;
   getQoreString(jc->getName(), cname);

   if (n && n->getType() == NT_OBJECT)
      xsink->raiseException("JAVA-UNSUPPORTED-TYPE", "cannot convert from Qore class '%s' to Java '%s'", reinterpret_cast<const QoreObject *>(n)->getClassName(), cname.getBuffer());
   else
      xsink->raiseException("JAVA-UNSUPPORTED-TYPE", "cannot convert from Qore '%s' to Java '%s'", get_type_name(n), cname.getBuffer());
   return 0;   
}

#ifdef NEED_BOEHM_SIGNALS
static void unblock_thread_signals() {
   sigset_t mask;
   // setup signal mask
   pthread_sigmask(SIG_UNBLOCK, 0, &mask);
   for (unsigned i = 0; i < NUM_BOEHM_SIGS; ++i) {
      int sig = boehm_sigs[i];
      sigdelset(&mask, sig);
   }
   // unblock threads
   pthread_sigmask(SIG_BLOCK, &mask, 0);
}
#endif

QoreStringNode *gnu_java_module_init() {
#ifdef NEED_BOEHM_SIGNALS
   // reassign signals needed by the boehm GC
   for (unsigned i = 0; i < NUM_BOEHM_SIGS; ++i) {
      int sig = boehm_sigs[i];
      QoreStringNode *err = qore_reassign_signal(sig, "gnu-java");
      if (err) 
	 return err;
   }
#endif
   unblock_thread_signals();

   try {
      // initialize JVM
      JvCreateJavaVM(0);
	 
      // attach to thread
      //QoreJavaThreadHelper jth;

      JvAttachCurrentThread(0, 0);

      qjcm.init();      
   }
   catch (java::lang::Throwable *t) {
      // the JVM has not been initialized here, so we cannot get the exception message reliably
      QoreStringNode *err = new QoreStringNode("error initializing gnu JVM");
      return err;
   }

   return 0;
}

void gnu_java_module_ns_init(QoreNamespace *rns, QoreNamespace *qns) {
   qns->addNamespace(qjcm.getRootNS().copy());
}

void gnu_java_module_delete() {
   JvDetachCurrentThread();
}
