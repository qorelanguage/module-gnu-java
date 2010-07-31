/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  gcj-module.h

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

#ifndef _QORE_GCJ_MODULE_H

#define _QORE_GCJ_MODULE_H

#include "../config.h"

#include <qore/Qore.h>

#include <gcj/cni.h>

#pragma GCC java_exceptions

#include "gcj-util.h"

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



#endif
