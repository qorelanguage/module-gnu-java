/* indent-tabs-mode: nil -*- */
/*
  gcj Qore module

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

#include "gcj-module.h"

QoreStringNode *gcj_module_init();
void gcj_module_ns_init(QoreNamespace *rns, QoreNamespace *qns);
void gcj_module_delete();

// qore module symbols
DLLEXPORT char qore_module_name[] = "gcj";
DLLEXPORT char qore_module_version[] = PACKAGE_VERSION;
DLLEXPORT char qore_module_description[] = "provides java functionality through GCJ (http://gcc.gnu.org/java/)";
DLLEXPORT char qore_module_author[] = "David Nichols";
DLLEXPORT char qore_module_url[] = "http://qore.org";
DLLEXPORT int qore_module_api_major = QORE_MODULE_API_MAJOR;
DLLEXPORT int qore_module_api_minor = QORE_MODULE_API_MINOR;
DLLEXPORT qore_module_init_t qore_module_init = gcj_module_init;
DLLEXPORT qore_module_ns_init_t qore_module_ns_init = gcj_module_ns_init;
DLLEXPORT qore_module_delete_t qore_module_delete = gcj_module_delete;
DLLEXPORT qore_license_t qore_module_license = QL_LGPL;

QoreNamespace GCJNS("GCJ");

QoreStringNode *gcj_module_init() {

   //GCJNS.addSystemClass(QC_GCJ);

   return 0;
}

void gcj_module_ns_init(QoreNamespace *rns, QoreNamespace *qns) {
   qns->addNamespace(GCJNS.copy());
}

void gcj_module_delete() {
}
