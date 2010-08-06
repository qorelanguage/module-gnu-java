#!/usr/bin/env qore

%requires gnu-java

my java::lang::String $str("hello");
printf("%n\n", dbg_node_info($str));
printf("methods=%n\n", getMethodList($str));

#my Mutex $m();
#printf("methods=%n\n", getMethodList($m));
