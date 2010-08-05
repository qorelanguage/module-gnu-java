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

#include <gcj/cni.h>

#pragma GCC java_exceptions

#include "gnu-java-util.h"

#include <map>
#include <vector>

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

typedef std::map<const char *, QoreClass *> jcmap_t;
typedef std::map<jclass, QoreClass *> jcpmap_t;

class QoreJavaClassMap {
protected:
   bool init_done;
   mutable QoreThreadLock m;
   jcmap_t jcmap;
   jcpmap_t jcpmap;

   DLLLOCAL void add_intern(const char *name, java::lang::Class *jc, QoreClass *qc) {
      assert(jcmap.find(name) == jcmap.end());
      jcmap[name] = qc;

      assert(jcpmap.find(jc) == jcpmap.end());
      jcpmap[jc] = qc;
   }

public:
   DLLLOCAL QoreJavaClassMap() : init_done(false) {      
   }

   DLLLOCAL QoreClass *createQoreClass(const char *name, java::lang::Class *jc);
   DLLLOCAL void populateQoreClass(const char *name);

   DLLLOCAL const QoreTypeInfo *getQoreType(java::lang::Class *jc, bool &err) const;

   DLLLOCAL void populateCoreClasses() {
      for (jcmap_t::iterator i = jcmap.begin(), e = jcmap.end(); i != e; ++i) {
         populateQoreClass(i->first);
      }
   }
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

#endif
