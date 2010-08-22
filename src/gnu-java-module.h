/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  gnu-java-module.h

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

#ifndef _QORE_GNU_JAVA_MODULE_H

#define _QORE_GNU_JAVA_MODULE_H

#include "../config.h"

#include <qore/Qore.h>
#include <qore/AbstractThreadResource.h>

#include <gcj/cni.h>

#pragma GCC java_exceptions

#include "gnu-java-util.h"

#define QORE_FEATURE_NAME "gnu-java"

#ifdef LINUX
// the boehm GC needs to catch the following signals on Linux
#define NEED_BOEHM_SIGNALS SIGSEGV, SIGPWR, SIGXCPU
#endif

#include <map>
#include <vector>
#include <string>

// Qore class ID for java::lang::Object class
DLLLOCAL extern qore_classid_t CID_OBJECT;
// thread resource ID for java threads in qore
DLLLOCAL extern q_trid_t gnu_java_trid;

class OptLocker {
protected:
   QoreThreadLock *lck;

public:
   DLLLOCAL OptLocker(QoreThreadLock *l) : lck(l) {
      if (lck)
         lck->lock();
   }
   
   DLLLOCAL ~OptLocker() {
      if (lck)
         lck->unlock();
   }
};

// map of java class names (i.e. java.lang.String) to QoreClass objects
typedef std::map<std::string, QoreClass *> jcmap_t;
// map of java class objects to QoreClass objects
typedef std::map<jclass, QoreClass *> jcpmap_t;

class QoreJavaClassMap {
protected:
   // parent namespace for gnu-java module functionality
   QoreNamespace default_gns;
   jcmap_t jcmap;
   jcpmap_t jcpmap;
   bool init_done;

   DLLLOCAL void add_intern(const char *name, java::lang::Class *jc, QoreClass *qc) {
      //printd(0, "QoreJavaClassMap::add_intern() name=%s jc=%p qc=%p (%s)\n", name, jc, qc, qc->getName());

      assert(jcmap.find(name) == jcmap.end());
      jcmap[name] = qc;

      assert(jcpmap.find(jc) == jcpmap.end());
      jcpmap[jc] = qc;
   }

   DLLLOCAL int getArgTypes(type_vec_t &argTypeInfo, JArray<jclass> *params);

   DLLLOCAL void doConstructors(QoreClass &qc, java::lang::Class *jc, ExceptionSink *xsink = 0);
   DLLLOCAL void doMethods(QoreClass &qc, java::lang::Class *jc, ExceptionSink *xsink = 0);

   DLLLOCAL void populateQoreClass(QoreClass &qc, java::lang::Class *jc, ExceptionSink *xsink = 0);
   DLLLOCAL void addQoreClass();

   DLLLOCAL void addSuperClass(QoreClass &qc, java::lang::Class *jsc);

public:
   mutable QoreThreadLock m;

   DLLLOCAL QoreJavaClassMap() : default_gns("gnu"), init_done(false) {
   }

   DLLLOCAL void init();

   DLLLOCAL QoreClass *createQoreClass(QoreNamespace &gns, const char *name, java::lang::Class *jc, ExceptionSink *xsink = 0);

   DLLLOCAL QoreClass *find(java::lang::Class *jc) const {
      jcpmap_t::const_iterator i = jcpmap.find(jc);
      return i == jcpmap.end() ? 0 : i->second;
   }

   DLLLOCAL QoreClass *findCreate(java::lang::Class *jc) {
      QoreClass *qc = find(jc);
      if (qc)
         return qc;
      QoreString cname;
      getQoreString(jc->getName(), cname);	 
      return createQoreClass(default_gns, cname.getBuffer(), jc);
   }

   DLLLOCAL const QoreTypeInfo *getQoreType(java::lang::Class *jc, bool &err);

   DLLLOCAL void initDone() {
      init_done = true;
   }

   DLLLOCAL QoreNamespace &getRootNS() {
      return default_gns;
   }

   DLLLOCAL java::lang::Object *toJava(java::lang::Class *jc, const AbstractQoreNode *n, ExceptionSink *xsink);
   DLLLOCAL AbstractQoreNode *toQore(java::lang::Object *jobj, ExceptionSink *xsink);

   DLLLOCAL QoreClass *loadClass(QoreNamespace &gns, java::lang::ClassLoader *loader, const char *cstr, java::lang::String *jstr = 0, ExceptionSink *xsink = 0);
};

class QoreJavaThreadHelper {
public:
   DLLLOCAL QoreJavaThreadHelper() {
      QoreString tstr;
      tstr.sprintf("qore-thread-%d", gettid());
      java::lang::String *msg = JvNewStringLatin1(tstr.getBuffer());

      JvAttachCurrentThread(msg, 0);
   }

   DLLLOCAL ~QoreJavaThreadHelper() {
      JvDetachCurrentThread();
   }
};

class QoreJavaPrivateData : public AbstractPrivateData {
protected:
   java::lang::Object *jobj;

public:
   DLLLOCAL QoreJavaPrivateData(java::lang::Object *n_jobj) : jobj(n_jobj) {
   }

   DLLLOCAL void destructor() {
      delete jobj;
   }

   DLLLOCAL java::lang::Object *getObject() const {
      return jobj;
   }
};

class QoreJavaThreadResource : public AbstractThreadResource {
public:
   DLLLOCAL ~QoreJavaThreadResource() {
   }

   // called when the Qore thread is terminating
   DLLLOCAL virtual void cleanup(ExceptionSink *xsink);

   DLLLOCAL void check_thread();
};

#endif
