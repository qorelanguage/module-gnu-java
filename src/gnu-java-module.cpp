/* indent-tabs-mode: nil -*- */
/*
  gnu-java Qore module

  Copyright (C) 2010 - 2012 David Nichols

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
#include <java/lang/reflect/Field.h>
#include <java/lang/reflect/Array.h>
#include <java/lang/reflect/InvocationTargetException.h>

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

#include <java/util/Vector.h>

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

q_trid_t gnu_java_trid;

qore_classid_t CID_OBJECT;

QoreStringNode *gnu_java_module_init();
void gnu_java_module_ns_init(QoreNamespace *rns, QoreNamespace *qns);
void gnu_java_module_delete();
void gnu_java_module_parse_cmd(const QoreString &cmd, ExceptionSink *xsink);

static QoreClass *gnu_java_class_handler(QoreNamespace *ns, const char *cname);

// qore module symbols
DLLEXPORT char qore_module_name[] = QORE_FEATURE_NAME;
DLLEXPORT char qore_module_version[] = PACKAGE_VERSION;
DLLEXPORT char qore_module_description[] = "provides java functionality through the gnu java JVM (http://gcc.gnu.org/java/)";
DLLEXPORT char qore_module_author[] = "David Nichols";
DLLEXPORT char qore_module_url[] = "http://qore.org";
DLLEXPORT int qore_module_api_major = QORE_MODULE_API_MAJOR;
DLLEXPORT int qore_module_api_minor = QORE_MODULE_API_MINOR;
DLLEXPORT qore_module_init_t qore_module_init = gnu_java_module_init;
DLLEXPORT qore_module_ns_init_t qore_module_ns_init = gnu_java_module_ns_init;
DLLEXPORT qore_module_delete_t qore_module_delete = gnu_java_module_delete;
DLLEXPORT qore_module_parse_cmd_t qore_module_parse_cmd = gnu_java_module_parse_cmd;

DLLEXPORT qore_license_t qore_module_license = QL_LGPL;

QoreJavaClassMap qjcm;
QoreJavaThreadResource qjtr;

#ifdef NEED_BOEHM_SIGNALS
static void unblock_thread_signals() {
   sigset_t mask;
   // setup signal mask
   sigemptyset(&mask);
   for (unsigned i = 0; i < NUM_BOEHM_SIGS; ++i) {
      //printd(0, "unblock_thread_signals() unblocking signal %d\n", boehm_sigs[i]);
      sigaddset(&mask, boehm_sigs[i]);
   }
   // unblock threads
   pthread_sigmask(SIG_UNBLOCK, &mask, 0);
}
#else
#define unblock_thread_signals(a)
#endif

// called when the Qore thread is terminating
// we can now detach the thread from the JVM
void QoreJavaThreadResource::cleanup(ExceptionSink *xsink) {
   JvDetachCurrentThread();
}

void QoreJavaThreadResource::check_thread() {
   if (check_thread_resource(this))
      return;

   // make sure signals needed for garbage collection are unblocked in this thread
   unblock_thread_signals();

   // attach the thread to the JVM
   //QoreString tstr;
   //tstr.sprintf("qore-thread-%d", gettid());
   //java::lang::String *msg = JvNewStringLatin1(tstr.getBuffer());
   JvAttachCurrentThread(0, 0);

   // save thread resource so that we can detach from the java thread when the qore thread terminates
   set_thread_resource(this);
}

void QoreJavaClassMap::addSuperClass(QoreClass &qc, java::lang::Class *jsc) {
#ifdef DEBUG_0
   QoreString sn;
   getQoreString(jsc->getName(), sn);
   printd(0, "QoreJavaClassMap::addSuperClass() %s has super class %p %s\n", qc.getName(), jsc, sn.getBuffer());
#endif

   qc.addBuiltinVirtualBaseClass(findCreate(jsc));
}

static QoreNamespace *findCreateNamespace(QoreNamespace &gns, const char *name, const char *&sn) {
   sn = rindex(name, '.');

   QoreNamespace *ns;
   // find parent namespace
   if (!sn) {
      ns = &gns;
      sn = name;
   }
   else {
      QoreString nsn(name, sn - name);
      nsn.replaceAll(".", "::");
      // add a dummy symbol on the end
      nsn.concat("::x");
      ++sn;
      ns = gns.findCreateNamespacePath(nsn.getBuffer());
      //printd(5, "findCreateNamespace() gns: %p %s nsn: %s ns: %p (%s)\n", &gns, gns.getName(), nsn.getBuffer(), ns, ns->getName());
   }

   return ns;
}

// creates a QoreClass and adds it in the appropriate namespace
QoreClass *QoreJavaClassMap::createQoreClass(QoreNamespace &gns, const char *name, java::lang::Class *jc, ExceptionSink *xsink) {
   QoreClass *qc = find(jc);
   if (qc) {
      if (&gns == &default_gns)
	 return qc;
   }
   else
      JvInitClass(jc);

   const char *sn;

   // find/create parent namespace
   QoreNamespace *ns = findCreateNamespace(gns, name, sn);

   if (qc) {
      // see if class already exists in this namespace
      QoreClass *nqc = ns->findLocalClass(qc->getName());

      if (!nqc) {
	 nqc = new QoreClass(*qc);
	 ns->addSystemClass(nqc);
      }
      return nqc;
   }

   qc = new QoreClass(sn);
   // save pointer to java class info in QoreClass
   qc->setUserData(jc);

   // save class in namespace
   ns->addSystemClass(qc);

   //printd(5, "QoreJavaClassMap::createQoreClass() qc: %p (%s) %s::%s\n", qc, name, ns->getName(), sn);

   // add to class maps
   add_intern(name, jc, qc);

   // get and process superclass
   java::lang::Class *jsc = jc->getSuperclass();

   // add superclass
   if (jsc)
      addSuperClass(*qc, jsc);

   // add interface classes as superclasses
   JArray<jclass>* ifc = jc->getInterfaces();
   if (ifc && ifc->length) {
      for (int i = 0; i < ifc->length; ++i)
	 addSuperClass(*qc, elements(ifc)[i]);
   }

   populateQoreClass(*qc, jc, xsink);

   return qc;
}

static jobjectArray get_java_args(JArray<jclass>* params, const QoreListNode *args, ExceptionSink *xsink) {
   //printd(0, "get_java_args() params: %p (%d) args: %p (%d)\n", params, params->length, args, args ? args->size() : 0);

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

static void exec_java_constructor(const QoreClass &qc, const type_vec_t &typeList, void *ip, QoreObject *self, const QoreListNode *args, ExceptionSink *xsink) {
   // unblock signals and attach to java thread if necessary
   qjtr.check_thread();

   // get java args
   try {
      int i = (long)ip;
      jclass jc = (jclass)qc.getUserData();
      JArray<java::lang::reflect::Constructor*>* methods = jc->getDeclaredConstructors(false);
      java::lang::reflect::Constructor* method = elements(methods)[i];

      //printd(0, "exec_java_constructor() %s::constructor() method: %p\n", qc.getName(), method);

      JArray<jclass>* params = method->getParameterTypes();
      //printd(0, "exec_java_constructor() %s::constructor() method: %p params: %p (%d\n", qc.getName(), method);

      jobjectArray jargs = get_java_args(params, args, xsink);

      if (*xsink)
	 return;

      method->setAccessible(true);
      java::lang::Object *jobj = method->newInstance(jargs);
      //printd(0, "exec_java_constructor() %s::constructor() method: %p jobj: %p\n", qc.getName(), method, jobj);

      if (!jobj)
	 xsink->raiseException("JAVA-CONSTRUCTOR-ERROR", "%s::constructor() did not return any object", qc.getName());
      else {
	 QoreJavaPrivateData *pd = new QoreJavaPrivateData(jobj);
	 //printd(0, "exec_java_constructor() %s::constructor() private data: %p, jobj: %p (%p)\n", qc.getName(), pd, pd->getObject(), jobj);
	 self->setPrivate(qc.getID(), pd);
      }
   }
   catch (java::lang::Throwable *e) {
      getQoreException(e, *xsink);
   }
}

//static AbstractQoreNode *exec_java_static(const QoreMethod &qm, const type_vec_t &typeList, java::lang::reflect::Method *method, const QoreListNode *args, ExceptionSink *xsink) {
static AbstractQoreNode *exec_java_static(const QoreMethod &qm, const type_vec_t &typeList, void *ip, const QoreListNode *args, ExceptionSink *xsink) {
   // unblock signals and attach to java thread if necessary
   qjtr.check_thread();

   // get java args
   try {
      int i = (long)ip;
      jclass jc = (jclass)qm.getClass()->getUserData();
      JArray<java::lang::reflect::Method*>* methods = jc->getDeclaredMethods();
      java::lang::reflect::Method *method = elements(methods)[i];

      JArray<jclass>* params = method->getParameterTypes();
      // get java args
      jobjectArray jargs = get_java_args(params, args, xsink);
      if (*xsink)
	 return 0;

      method->setAccessible(true);
      java::lang::Object *jrv = method->invoke(0, jargs);

      return qjcm.toQore(jrv, xsink);
   }
   catch (java::lang::reflect::InvocationTargetException* e) {
      getQoreException(e->getCause(), *xsink);
   }
   catch (java::lang::Throwable *e) {
      getQoreException(e, *xsink);
   }
   return 0;
}

//static AbstractQoreNode *exec_java(const QoreMethod &qm, const type_vec_t &typeList, java::lang::reflect::Method *method, QoreObject *self, QoreJavaPrivateData *pd, const QoreListNode *args, ExceptionSink *xsink) {
static AbstractQoreNode *exec_java(const QoreMethod &qm, const type_vec_t &typeList, void *ip, QoreObject *self, QoreJavaPrivateData *pd, const QoreListNode *args, ExceptionSink *xsink) {
   // unblock signals and attach to java thread if necessary
   qjtr.check_thread();

   // get java args
   try {
      int i = (long)ip;
      jclass jc = (jclass)qm.getClass()->getUserData();
      JArray<java::lang::reflect::Method*>* methods = jc->getDeclaredMethods();
      java::lang::reflect::Method *method = elements(methods)[i];

#ifdef DEBUG
      QoreString mname;
      getQoreString(method->toString(), mname);
      //printd(0, "exec_java() %s::%s() %s method: %p args: %p (%d)\n", qm.getClassName(), qm.getName(), mname.getBuffer(), method, args, args ? args->size() : 0);
#endif

      JArray<jclass>* params = method->getParameterTypes();
      // get java args
      jobjectArray jargs = get_java_args(params, args, xsink);
      if (*xsink)
	 return 0;

      jobject jobj = pd->getObject();
      //printd(0, "exec_java() %s::%s() pd: %p jobj: %p args: %p (%d) jargs: %p (%d)\n", qm.getClassName(), qm.getName(), pd, jobj, args, args ? args->size() : 0, jargs, jargs ? jargs->length : 0);
      method->setAccessible(true);
      java::lang::Object *jrv = method->invoke(jobj, jargs);

      return qjcm.toQore(jrv, xsink);
   }
   catch (java::lang::reflect::InvocationTargetException* e) {
      getQoreException(e->getCause(), *xsink);
   }
   catch (java::lang::Throwable *e) {
      getQoreException(e, *xsink);
   }
   return 0;
}

int QoreJavaClassMap::getArgTypes(type_vec_t &argTypeInfo, JArray<jclass>* params) {
   argTypeInfo.reserve(params->length);

   for (int i = 0; i < params->length; ++i) {
      java::lang::Class *jc = elements(params)[i];

      bool err;
      const QoreTypeInfo *typeInfo = getQoreType(jc, err);
      if (err)
	 return -1;

      //printd(0, "QoreJavaClassMap::getArgTypes() jc: %p (%s), qore: %s\n", );

      argTypeInfo.push_back(typeInfo);
   }
   return 0;
}

void QoreJavaClassMap::doConstructors(QoreClass &qc, java::lang::Class *jc, ExceptionSink *xsink) {
   try {
      // get constructor methods
      JArray<java::lang::reflect::Constructor*>* methods = jc->getDeclaredConstructors(false);

      for (size_t i = 0; i < (size_t)methods->length; ++i) {
	 java::lang::reflect::Constructor *m = elements(methods)[i];

#ifdef DEBUG
	 QoreString mstr;
	 getQoreString(m->toString(), mstr);
	 //if (!strcmp(qc.getName(), "URL"))
	 //printd(0, "  + adding %s.constructor() (%s) m: %p\n", qc.getName(), mstr.getBuffer(), m);
#endif

	 // get parameter type array
	 JArray<jclass>* params = m->getParameterTypes();

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
   catch (java::lang::Throwable *t) {
      if (xsink)
	 getQoreException(t, *xsink);
   }
}

void QoreJavaClassMap::doMethods(QoreClass &qc, java::lang::Class *jc, ExceptionSink *xsink) {
   //printd(0, "QoreJavaClassMap::doMethods() %s qc: %p jc: %p\n", name, qc, jc);

   try {
      JArray<java::lang::reflect::Method*>* methods = jc->getDeclaredMethods();
      for (size_t i = 0; i < (size_t)methods->length; ++i) {
	 java::lang::reflect::Method *m = elements(methods)[i];

	 QoreString mname;
	 getQoreString(m->getName(), mname);
	 assert(!mname.empty());

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

	    printd(5, "  + adding static %s%s::%s() (%s) qc: %p\n", priv ? "private " : "", qc.getName(), mname.getBuffer(), mstr.getBuffer(), &qc);
	    qc.addStaticMethodExtendedList3((void *)i, mname.getBuffer(), (q_static_method3_t)exec_java_static, priv, flags, QDOM_DEFAULT, returnTypeInfo, argTypeInfo);
	 }
	 else {
	    if (mname == "copy")
	       mname.prepend("java_");
	    const QoreMethod *qm = qc.findLocalMethod(mname.getBuffer());
	    if (qm && qm->existsVariant(argTypeInfo)) {
	       //printd(0, "QoreJavaClassMap::doMethods() skipping already-created variant %s::%s()\n", qc.getName(), mname.getBuffer());
	       continue;
	    }

	    printd(5, "  + adding %s%s::%s() (%s) qc: %p\n", priv ? "private " : "", qc.getName(), mname.getBuffer(), mstr.getBuffer(), &qc);
	    qc.addMethodExtendedList3((void *)i, mname.getBuffer(), (q_method3_t)exec_java, priv, flags, QDOM_DEFAULT, returnTypeInfo, argTypeInfo);
	 }
      }
   }
   catch (java::lang::Throwable *t) {
      if (xsink)
	 getQoreException(t, *xsink);
   }
}

void QoreJavaClassMap::doFields(QoreClass &qc, java::lang::Class *jc, ExceptionSink *xsink) {
   printd(5, "QoreJavaClassMap::doFields() %s qc: %p jc: %p\n", qc.getName(), &qc, jc);

   try {
      JArray<java::lang::reflect::Field*>* fields = jc->getDeclaredFields();
      for (int i = 0; i < fields->length; ++i) {
	 java::lang::reflect::Field* f = elements(fields)[i];

	 f->setAccessible(true);

	 int mod = f->getModifiers();

	 QoreString fname;
	 getQoreString(f->getName(), fname);

	 assert(fname.strlen());

#ifdef DEBUG
	 QoreString fstr;
	 getQoreString(f->toString(), fstr);
	 printd(5, "  + adding %s.%s (%s)\n", qc.getName(), fname.getBuffer(), fstr.getBuffer());
#endif

	 bool priv = mod & (java::lang::reflect::Modifier::PRIVATE|java::lang::reflect::Modifier::PROTECTED);

	 java::lang::Class *typec = f->getType();
	 const QoreTypeInfo *type = toTypeInfo(typec);

	 if (mod & java::lang::reflect::Modifier::STATIC) {
	    AbstractQoreNode *val = toQore(f->get(0), xsink);
	    if (*xsink)
	       break;

	    if (mod & java::lang::reflect::Modifier::FINAL) {
	       if (val)
		  qc.addBuiltinConstant(fname.getBuffer(), val, priv);
	    }
	    else
	       qc.addBuiltinStaticVar(fname.getBuffer(), val, priv, type);
	 }
	 else {
	    if (priv)
	       qc.addPrivateMember(fname.getBuffer(), type);
	    else
	       qc.addPublicMember(fname.getBuffer(), type);
	 }
      }
   }
   catch (java::lang::Throwable *t) {
      if (xsink)
	 getQoreException(t, *xsink);
   }
}

void QoreJavaClassMap::populateQoreClass(QoreClass &qc, java::lang::Class *jc, ExceptionSink *xsink) {
   // do constructors
   doConstructors(qc, jc, xsink);

   // do methods
   doMethods(qc, jc, xsink);

   // do fields
   doFields(qc, jc, xsink);
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

   default_gns.addSystemClass(qc);
}

QoreClass *QoreJavaClassMap::loadClass(QoreNamespace &gns, java::lang::ClassLoader *loader, const char *cstr, java::lang::String *jstr, ExceptionSink *xsink) {
   assert(cstr || jstr);

   if (!loader)
      loader = java::lang::ClassLoader::getSystemClassLoader();

   QoreString qstr;
   if (!cstr) {
      getQoreString(jstr, qstr);
      cstr = qstr.getBuffer();
   }
   else if (!jstr)
      jstr = JvNewStringLatin1(cstr);

   jclass jc;
   try {
      jc = loader->loadClass(jstr);
   }
   catch (java::lang::ClassNotFoundException *e) {
      //printd(0, "ERROR: cannot map %s: ClassNotFoundException\n", cstr);
      if (xsink)
	 getQoreException(e, *xsink);
      return 0;
   }
   catch (java::lang::Throwable *t) {
      //printd(0, "ERROR: cannot map %s: other exception\n", cstr);
      if (xsink)
	 getQoreException(t, *xsink);
      return 0;
   }

   return createQoreClass(gns, cstr, jc, xsink);
}

void QoreJavaClassMap::init() {
   java::lang::ClassLoader *loader = java::lang::ClassLoader::getSystemClassLoader();

   printd(5, "QoreJavaClassMap::init() loader: %p\n", loader);

   // create java.lang namespace with automatic class loader handler
   QoreNamespace* javans = new QoreNamespace("java");
   QoreNamespace* langns = new QoreNamespace("lang");
   langns->setClassHandler(gnu_java_class_handler);
   javans->addInitialNamespace(langns);

   // add to "gnu" namespace
   default_gns.addInitialNamespace(javans);

   // add "Object" class
   qjcm.loadClass(default_gns, loader, "java.lang.Object");

   // add "Qore" class to gnu namespace
   addQoreClass();

/*
   {
      QoreHashNode *h = qjcm.getRootNS().getInfo();
      QoreNodeAsStringHelper str(h, FMT_NONE, 0);
      printd(0, "init qjcm.getRootNS() %p: %s\n", &qjcm.getRootNS(), str->getBuffer());
   }
*/
}

const QoreTypeInfo *QoreJavaClassMap::toTypeInfo(java::lang::Class *jc) {
   if (jc == &java::lang::String::class$
       || jc == &java::lang::Character::class$)
      return stringTypeInfo;

   if (jc == &java::lang::Long::class$
       || jc == &java::lang::Integer::class$
       || jc == &java::lang::Short::class$
       || jc == &java::lang::Byte::class$)
      return bigIntTypeInfo;

   if (jc == &java::lang::Boolean::class$)
      return boolTypeInfo;

   if (jc == &java::lang::Double::class$
       || jc == &java::lang::Float::class$)
      return floatTypeInfo;

   QoreClass *qc = find(jc);
   return qc ? qc->getTypeInfo() : 0;
}

AbstractQoreNode *QoreJavaClassMap::toQore(java::lang::Object *jobj, ExceptionSink *xsink) {
   if (!jobj)
      return 0;

   jclass jc = jobj->getClass();

   if (jc->isArray()) {
      ReferenceHolder<QoreListNode> rv(new QoreListNode, xsink);

      jint len = java::lang::reflect::Array::getLength(jobj);
      for (jint i = 0; i < len; ++i) {
	 java::lang::Object* elem = java::lang::reflect::Array::get(jobj, i);
	 AbstractQoreNode* qe = toQore(elem, xsink);
	 if (*xsink) {
	    assert(!qe);
	    return 0;
	 }
	 rv->push(qe);
      }

      return rv.release();
   }

   if (jc == &java::util::Vector::class$) {
      java::util::Vector* vec = reinterpret_cast<java::util::Vector*>(jobj);
      ReferenceHolder<QoreListNode> rv(new QoreListNode, xsink);

      jint len = vec->size();
      for (jint i = 0; i < len; ++i) {
	 java::lang::Object* elem = vec->elementAt(i);
	 AbstractQoreNode* qe = toQore(elem, xsink);
	 if (*xsink) {
	    assert(!qe);
	    return 0;
	 }
	 rv->push(qe);
      }

      return rv.release();
   }

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
   if (qc) // set Program to NULL as java objects should not depend on code in the current Program object
      return new QoreObject(qc, 0, new QoreJavaPrivateData(jobj));

   if (!xsink)
      return 0;

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
   printd(5, "QoreJavaClassMap::toJava() jc: %p %s n: %p %s\n", jc, pname.getBuffer(), n, get_type_name(n));
#endif
*/

   // handle NULL pointers first
   if (!n || jc == JvPrimClass(void))
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
         if (!is_nothing(n)) {
            elements(array)[0] = toJava(cc, n, xsink);
	    if (*xsink)
	       return 0;
	 }
         return array;
      }
   }

   if (jc == &java::lang::String::class$) {
      QoreStringValueHelper str(n, QCS_UTF8, xsink);
      if (*xsink)
	 return 0;

      return JvNewStringUTF(str->getBuffer());
   }

   if (jc == &java::lang::Long::class$ || jc == JvPrimClass(long))
      return ::toJava((jlong)n->getAsBigInt());

   if (jc == &java::lang::Integer::class$ || jc == JvPrimClass(int))
      return new java::lang::Integer((jint)n->getAsInt());

   if (jc == &java::lang::Short::class$ || jc == JvPrimClass(short))
      return new java::lang::Short((jshort)n->getAsInt());

   if (jc == &java::lang::Byte::class$ || jc == JvPrimClass(byte))
      return new java::lang::Byte((jbyte)n->getAsInt());

   if (jc == &java::lang::Boolean::class$ || jc == JvPrimClass(boolean))
      return ::toJava((jboolean)n->getAsBool());

   if (jc == &java::lang::Double::class$ || jc == JvPrimClass(double))
      return ::toJava((jdouble)n->getAsFloat());

   if (jc == &java::lang::Float::class$ || jc == JvPrimClass(float))
      return ::toJava((jfloat)n->getAsFloat());

   if (jc == &java::lang::Character::class$ || jc == JvPrimClass(char)) {
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

static QoreClass *gnu_java_class_handler(QoreNamespace *ns, const char *cname) {
   // get full class path
   QoreString cp(ns->getName());
   cp.concat('.');
   cp.concat(cname);

   const QoreNamespace *gns = ns;
   while (true) {
      //printd(5, "gnu_java_class_handler() ns: %p (%s) gns: %p (%s) cname: %s\n", ns, ns->getName(), gns, gns->getName(), cname);
      gns = gns->getParent();
      assert(gns);
      if (!strcmp(gns->getName(), "gnu"))
	 break;
      cp.prepend(".");
      cp.prepend(gns->getName());
   }

   //printd(0, "gnu_java_class_handler() ns: %p cname: %s cp: %s\n", ns, cname, cp.getBuffer());

   // unblock signals and attach to java thread if necessary
   qjtr.check_thread();

   // parsing can occur in parallel in different QoreProgram objects
   // so we need to protect the load with a lock
   AutoLocker al(qjcm.m);
   QoreClass *qc = qjcm.loadClass(*const_cast<QoreNamespace *>(gns), 0, cp.getBuffer(), 0);

   //printd(5, "gnu_java_class_handler() cp: %s returning qc: %p\n", cp.getBuffer(), qc);
   return qc;
}

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
      JvAttachCurrentThread(0, 0);

      // set thread resource for java thread
      set_thread_resource(&qjtr);

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
   assert(qjcm.getRootNS().findLocalNamespace("java"));
   QoreNamespace* gns = qjcm.getRootNS().copy();
   rns->addNamespace(gns);
}

void gnu_java_module_delete() {
   ExceptionSink xsink;
   qjcm.getRootNS().deleteData(&xsink);
}

void gnu_java_module_parse_cmd(const QoreString &cmd, ExceptionSink *xsink) {
   const char *p = strchr(cmd.getBuffer(), ' ');

   if (!p) {
      xsink->raiseException("GNU-JAVA-PARSE-COMMAND-ERROR", "missing command name in parse command: '%s'", cmd.getBuffer());
      return;
   }

   QoreString str(&cmd, p - cmd.getBuffer());
   if (strcmp(str.getBuffer(), "import")) {
      xsink->raiseException("GNU-JAVA-PARSE-COMMAND-ERROR", "unrecognized command '%s' in '%s' (valid command: 'import')", str.getBuffer(), cmd.getBuffer());
      return;
   }

   QoreString arg(cmd);

   arg.replace(0, p - cmd.getBuffer() + 1, (const char *)0);
   arg.trim();

   QoreProgram *pgm = getProgram();
   bool has_feature = pgm ? pgm->checkFeature(QORE_FEATURE_NAME) : false;

   // process import statement

   //printd(5, "gnu_java_module_parse_cmd() pgm: %p arg: %s c: %c\n", pgm, arg.getBuffer(), arg[-1]);

   // see if there is a wildcard at the end
   bool wc = false;
   if (arg[-1] == '*') {
      if (arg[-2] != '.' || arg.strlen() < 3) {
	 xsink->raiseException("GNU-JAVA-IMPORT-ERROR", "invalid import argument: '%s'", arg.getBuffer());
	 return;
      }

      arg.terminate(arg.strlen() - 2);

      arg.replaceAll(".", "::");

      QoreNamespace *gns;

      // create gnu namespace in root namespace if necessary
      if (!has_feature)
	 gns = &qjcm.getRootNS();
      else {
	 QoreNamespace *rns = pgm->getRootNS();
	 gns = rns->findCreateNamespacePath("gnu");
      }

      QoreNamespace *ns = gns->findCreateNamespacePath(arg.getBuffer());
      ns->setClassHandler(gnu_java_class_handler);
      wc = true;
   }
   else {
      // unblock signals and attach to java thread if necessary
      qjtr.check_thread();

      {
	 // parsing can occur in parallel in different QoreProgram objects
	 // so we need to protect the load with a lock
	 AutoLocker al(qjcm.m);
	 qjcm.loadClass(qjcm.getRootNS(), 0, arg.getBuffer(), 0, xsink);
      }
   }

   // now try to add to current program
   //printd(5, "gnu_java_module_parse_cmd() pgm: %p arg: %s\n", pgm, arg.getBuffer());
   if (!pgm)
      return;

   QoreNamespace *ns = pgm->getRootNS();

   //printd(5, "gnu_java_module_parse_cmd() feature %s = %s (default_gns: %p)\n", QORE_FEATURE_NAME, pgm->checkFeature(QORE_FEATURE_NAME) ? "true" : "false", &qjcm.getRootNS());

   if (!pgm->checkFeature(QORE_FEATURE_NAME)) {
      assert(qjcm.getRootNS().findLocalNamespace("java"));
      QoreNamespace *gns = qjcm.getRootNS().copy();

      //printd(5, "gns: %p %s\n", gns, gns->getName());

      assert(gns->findLocalNamespace("java"));

      ns->addNamespace(gns);
      pgm->addFeature(QORE_FEATURE_NAME);

      //assert(ns->findLocalNamespace("gnu"));
      ns = ns->findLocalNamespace("gnu");
      assert(ns);

      ns = ns->findLocalNamespace("java");
      assert(ns);
      //assert(ns->findLocalNamespace("gnu")->findLocalNamespace("java"));

      return;
   }

   if (!wc) {
      ns = ns->findLocalNamespace("gnu");
      assert(ns);
      qjcm.loadClass(*ns, 0, arg.getBuffer(), 0, xsink);
   }
}
