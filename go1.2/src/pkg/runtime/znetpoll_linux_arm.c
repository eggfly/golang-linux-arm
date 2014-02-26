// auto generated by go tool dist
// goos=linux goarch=arm

#include "runtime.h"
#include "defs_GOOS_GOARCH.h"
#include "arch_GOARCH.h"
#include "malloc.h"
#define READY ((G*)1)

#line 24 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
struct PollDesc 
{ 
PollDesc* link; 
Lock; 
uintptr fd; 
bool closing; 
uintptr seq; 
G* rg; 
Timer rt; 
int64 rd; 
G* wg; 
Timer wt; 
int64 wd; 
} ; 
#line 39 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
static struct 
{ 
Lock; 
PollDesc* first; 
#line 48 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
} pollcache; 
#line 50 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
static bool netpollblock ( PollDesc* , int32 ) ; 
static G* netpollunblock ( PollDesc* , int32 , bool ) ; 
static void deadline ( int64 , Eface ) ; 
static void readDeadline ( int64 , Eface ) ; 
static void writeDeadline ( int64 , Eface ) ; 
static PollDesc* allocPollDesc ( void ) ; 
static intgo checkerr ( PollDesc *pd , int32 mode ) ; 
#line 58 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
static FuncVal deadlineFn = { ( void ( * ) ( void ) ) deadline } ; 
static FuncVal readDeadlineFn = { ( void ( * ) ( void ) ) readDeadline } ; 
static FuncVal writeDeadlineFn = { ( void ( * ) ( void ) ) writeDeadline } ; 
void
net·runtime_pollServerInit()
{
#line 62 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"

	runtime·netpollinit();
}
void
net·runtime_pollOpen(uintptr fd, PollDesc* pd, intgo errno)
{
#line 66 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"

	pd = allocPollDesc();
	runtime·lock(pd);
	if(pd->wg != nil && pd->wg != READY)
		runtime·throw("runtime_pollOpen: blocked write on free descriptor");
	if(pd->rg != nil && pd->rg != READY)
		runtime·throw("runtime_pollOpen: blocked read on free descriptor");
	pd->fd = fd;
	pd->closing = false;
	pd->seq++;
	pd->rg = nil;
	pd->rd = 0;
	pd->wg = nil;
	pd->wd = 0;
	runtime·unlock(pd);

	errno = runtime·netpollopen(fd, pd);
	FLUSH(&pd);
	FLUSH(&errno);
}
void
net·runtime_pollClose(PollDesc* pd)
{
#line 85 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"

	if(!pd->closing)
		runtime·throw("runtime_pollClose: close w/o unblock");
	if(pd->wg != nil && pd->wg != READY)
		runtime·throw("runtime_pollClose: blocked write on closing descriptor");
	if(pd->rg != nil && pd->rg != READY)
		runtime·throw("runtime_pollClose: blocked read on closing descriptor");
	runtime·netpollclose(pd->fd);
	runtime·lock(&pollcache);
	pd->link = pollcache.first;
	pollcache.first = pd;
	runtime·unlock(&pollcache);
}
void
net·runtime_pollReset(PollDesc* pd, intgo mode, intgo err)
{
#line 99 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"

	runtime·lock(pd);
	err = checkerr(pd, mode);
	if(err)
		goto ret;
	if(mode == 'r')
		pd->rg = nil;
	else if(mode == 'w')
		pd->wg = nil;
ret:
	runtime·unlock(pd);
	FLUSH(&err);
}
void
net·runtime_pollWait(PollDesc* pd, intgo mode, intgo err)
{
#line 112 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"

	runtime·lock(pd);
	err = checkerr(pd, mode);
	if(err == 0) {
		while(!netpollblock(pd, mode)) {
			err = checkerr(pd, mode);
			if(err != 0)
				break;
			// Can happen if timeout has fired and unblocked us,
			// but before we had a chance to run, timeout has been reset.
			// Pretend it has not happened and retry.
		}
	}
	runtime·unlock(pd);
	FLUSH(&err);
}
void
net·runtime_pollWaitCanceled(PollDesc* pd, intgo mode)
{
#line 128 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"

	runtime·lock(pd);
	// wait for ioready, ignore closing or timeouts.
	while(!netpollblock(pd, mode))
		;
	runtime·unlock(pd);
}
void
net·runtime_pollSetDeadline(PollDesc* pd, int64 d, intgo mode)
{
#line 136 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"

	G *rg, *wg;

	runtime·lock(pd);
	if(pd->closing) {
		runtime·unlock(pd);
		return;
	}
	pd->seq++;  // invalidate current timers
	// Reset current timers.
	if(pd->rt.fv) {
		runtime·deltimer(&pd->rt);
		pd->rt.fv = nil;
	}
	if(pd->wt.fv) {
		runtime·deltimer(&pd->wt);
		pd->wt.fv = nil;
	}
	// Setup new timers.
	if(d != 0 && d <= runtime·nanotime())
		d = -1;
	if(mode == 'r' || mode == 'r'+'w')
		pd->rd = d;
	if(mode == 'w' || mode == 'r'+'w')
		pd->wd = d;
	if(pd->rd > 0 && pd->rd == pd->wd) {
		pd->rt.fv = &deadlineFn;
		pd->rt.when = pd->rd;
		// Copy current seq into the timer arg.
		// Timer func will check the seq against current descriptor seq,
		// if they differ the descriptor was reused or timers were reset.
		pd->rt.arg.type = (Type*)pd->seq;
		pd->rt.arg.data = pd;
		runtime·addtimer(&pd->rt);
	} else {
		if(pd->rd > 0) {
			pd->rt.fv = &readDeadlineFn;
			pd->rt.when = pd->rd;
			pd->rt.arg.type = (Type*)pd->seq;
			pd->rt.arg.data = pd;
			runtime·addtimer(&pd->rt);
		}
		if(pd->wd > 0) {
			pd->wt.fv = &writeDeadlineFn;
			pd->wt.when = pd->wd;
			pd->wt.arg.type = (Type*)pd->seq;
			pd->wt.arg.data = pd;
			runtime·addtimer(&pd->wt);
		}
	}
	// If we set the new deadline in the past, unblock currently pending IO if any.
	rg = nil;
	wg = nil;
	if(pd->rd < 0)
		rg = netpollunblock(pd, 'r', false);
	if(pd->wd < 0)
		wg = netpollunblock(pd, 'w', false);
	runtime·unlock(pd);
	if(rg)
		runtime·ready(rg);
	if(wg)
		runtime·ready(wg);
}
void
net·runtime_pollUnblock(PollDesc* pd)
{
#line 200 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"

	G *rg, *wg;

	runtime·lock(pd);
	if(pd->closing)
		runtime·throw("runtime_pollUnblock: already closing");
	pd->closing = true;
	pd->seq++;
	rg = netpollunblock(pd, 'r', false);
	wg = netpollunblock(pd, 'w', false);
	if(pd->rt.fv) {
		runtime·deltimer(&pd->rt);
		pd->rt.fv = nil;
	}
	if(pd->wt.fv) {
		runtime·deltimer(&pd->wt);
		pd->wt.fv = nil;
	}
	runtime·unlock(pd);
	if(rg)
		runtime·ready(rg);
	if(wg)
		runtime·ready(wg);
}

#line 225 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
uintptr 
runtime·netpollfd ( PollDesc *pd ) 
{ 
return pd->fd; 
} 
#line 232 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
void 
runtime·netpollready ( G **gpp , PollDesc *pd , int32 mode ) 
{ 
G *rg , *wg; 
#line 237 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
rg = wg = nil; 
runtime·lock ( pd ) ; 
if ( mode == 'r' || mode == 'r' +'w' ) 
rg = netpollunblock ( pd , 'r' , true ) ; 
if ( mode == 'w' || mode == 'r' +'w' ) 
wg = netpollunblock ( pd , 'w' , true ) ; 
runtime·unlock ( pd ) ; 
if ( rg ) { 
rg->schedlink = *gpp; 
*gpp = rg; 
} 
if ( wg ) { 
wg->schedlink = *gpp; 
*gpp = wg; 
} 
} 
#line 254 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
static intgo 
checkerr ( PollDesc *pd , int32 mode ) 
{ 
if ( pd->closing ) 
return 1; 
if ( ( mode == 'r' && pd->rd < 0 ) || ( mode == 'w' && pd->wd < 0 ) ) 
return 2; 
return 0; 
} 
#line 265 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
static bool 
netpollblock ( PollDesc *pd , int32 mode ) 
{ 
G **gpp; 
#line 270 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
gpp = &pd->rg; 
if ( mode == 'w' ) 
gpp = &pd->wg; 
if ( *gpp == READY ) { 
*gpp = nil; 
return true; 
} 
if ( *gpp != nil ) 
runtime·throw ( "netpollblock: double wait" ) ; 
*gpp = g; 
runtime·park ( runtime·unlock , &pd->Lock , "IO wait" ) ; 
runtime·lock ( pd ) ; 
if ( g->param ) 
return true; 
return false; 
} 
#line 287 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
static G* 
netpollunblock ( PollDesc *pd , int32 mode , bool ioready ) 
{ 
G **gpp , *old; 
#line 292 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
gpp = &pd->rg; 
if ( mode == 'w' ) 
gpp = &pd->wg; 
if ( *gpp == READY ) 
return nil; 
if ( *gpp == nil ) { 
#line 300 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
if ( ioready ) 
*gpp = READY; 
return nil; 
} 
old = *gpp; 
#line 306 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
old->param = ( void* ) ioready; 
*gpp = nil; 
return old; 
} 
#line 311 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
static void 
deadlineimpl ( int64 now , Eface arg , bool read , bool write ) 
{ 
PollDesc *pd; 
uint32 seq; 
G *rg , *wg; 
#line 318 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
USED ( now ) ; 
pd = ( PollDesc* ) arg.data; 
#line 322 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
seq = ( uintptr ) arg.type; 
rg = wg = nil; 
runtime·lock ( pd ) ; 
if ( seq != pd->seq ) { 
#line 327 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
runtime·unlock ( pd ) ; 
return; 
} 
if ( read ) { 
if ( pd->rd <= 0 || pd->rt.fv == nil ) 
runtime·throw ( "deadlineimpl: inconsistent read deadline" ) ; 
pd->rd = -1; 
pd->rt.fv = nil; 
rg = netpollunblock ( pd , 'r' , false ) ; 
} 
if ( write ) { 
if ( pd->wd <= 0 || ( pd->wt.fv == nil && !read ) ) 
runtime·throw ( "deadlineimpl: inconsistent write deadline" ) ; 
pd->wd = -1; 
pd->wt.fv = nil; 
wg = netpollunblock ( pd , 'w' , false ) ; 
} 
runtime·unlock ( pd ) ; 
if ( rg ) 
runtime·ready ( rg ) ; 
if ( wg ) 
runtime·ready ( wg ) ; 
} 
#line 351 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
static void 
deadline ( int64 now , Eface arg ) 
{ 
deadlineimpl ( now , arg , true , true ) ; 
} 
#line 357 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
static void 
readDeadline ( int64 now , Eface arg ) 
{ 
deadlineimpl ( now , arg , true , false ) ; 
} 
#line 363 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
static void 
writeDeadline ( int64 now , Eface arg ) 
{ 
deadlineimpl ( now , arg , false , true ) ; 
} 
#line 369 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
static PollDesc* 
allocPollDesc ( void ) 
{ 
PollDesc *pd; 
uint32 i , n; 
#line 375 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
runtime·lock ( &pollcache ) ; 
if ( pollcache.first == nil ) { 
n = PageSize/sizeof ( *pd ) ; 
if ( n == 0 ) 
n = 1; 
#line 382 "/home/eggfly/go_build/go/src/pkg/runtime/netpoll.goc"
pd = runtime·persistentalloc ( n*sizeof ( *pd ) , 0 , &mstats.other_sys ) ; 
for ( i = 0; i < n; i++ ) { 
pd[i].link = pollcache.first; 
pollcache.first = &pd[i]; 
} 
} 
pd = pollcache.first; 
pollcache.first = pd->link; 
runtime·unlock ( &pollcache ) ; 
return pd; 
} 