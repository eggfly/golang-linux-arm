// auto generated by go tool dist
// goos=linux goarch=arm

#include "runtime.h"
void
runtime·GOMAXPROCS(intgo n, intgo ret)
{
#line 8 "/home/eggfly/go_build/go/src/pkg/runtime/runtime1.goc"

	ret = runtime·gomaxprocsfunc(n);
	FLUSH(&ret);
}
void
runtime·NumCPU(intgo ret)
{
#line 12 "/home/eggfly/go_build/go/src/pkg/runtime/runtime1.goc"

	ret = runtime·ncpu;
	FLUSH(&ret);
}
