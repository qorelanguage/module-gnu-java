#!/usr/bin/env qore

%new-style
%require-types
%enable-all-warnings

%requires gnu-java

java::lang::String str("hello");
printf("%N\n", str);
printf("methods: %y\n", get_method_list(str));

#Mutex m();
#printf("methods: %y\n", getMethodList(m));
