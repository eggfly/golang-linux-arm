// auto generated by go tool dist
// goos=linux goarch=arm

#include "runtime.h"
#include "arch_GOARCH.h"
#include "../../cmd/ld/textflag.h"

#line 25 "/usr/local/go/src/pkg/runtime/sema.goc"
typedef struct SemaWaiter SemaWaiter; 
struct SemaWaiter 
{ 
uint32 volatile* addr; 
G* g; 
int64 releasetime; 
int32 nrelease; 
SemaWaiter* prev; 
SemaWaiter* next; 
} ; 
#line 36 "/usr/local/go/src/pkg/runtime/sema.goc"
typedef struct SemaRoot SemaRoot; 
struct SemaRoot 
{ 
Lock; 
SemaWaiter* head; 
SemaWaiter* tail; 
#line 43 "/usr/local/go/src/pkg/runtime/sema.goc"
uint32 volatile nwait; 
} ; 
#line 47 "/usr/local/go/src/pkg/runtime/sema.goc"
#define SEMTABLESZ 251 
#line 49 "/usr/local/go/src/pkg/runtime/sema.goc"
struct semtable 
{ 
SemaRoot; 
uint8 pad[CacheLineSize-sizeof ( SemaRoot ) ]; 
} ; 
#pragma dataflag NOPTR 
static struct semtable semtable[SEMTABLESZ]; 
#line 57 "/usr/local/go/src/pkg/runtime/sema.goc"
static SemaRoot* 
semroot ( uint32 *addr ) 
{ 
return &semtable[ ( ( uintptr ) addr >> 3 ) % SEMTABLESZ]; 
} 
#line 63 "/usr/local/go/src/pkg/runtime/sema.goc"
static void 
semqueue ( SemaRoot *root , uint32 volatile *addr , SemaWaiter *s ) 
{ 
s->g = g; 
s->addr = addr; 
s->next = nil; 
s->prev = root->tail; 
if ( root->tail ) 
root->tail->next = s; 
else 
root->head = s; 
root->tail = s; 
} 
#line 77 "/usr/local/go/src/pkg/runtime/sema.goc"
static void 
semdequeue ( SemaRoot *root , SemaWaiter *s ) 
{ 
if ( s->next ) 
s->next->prev = s->prev; 
else 
root->tail = s->prev; 
if ( s->prev ) 
s->prev->next = s->next; 
else 
root->head = s->next; 
s->prev = nil; 
s->next = nil; 
} 
#line 92 "/usr/local/go/src/pkg/runtime/sema.goc"
static int32 
cansemacquire ( uint32 *addr ) 
{ 
uint32 v; 
#line 97 "/usr/local/go/src/pkg/runtime/sema.goc"
while ( ( v = runtime·atomicload ( addr ) ) > 0 ) 
if ( runtime·cas ( addr , v , v-1 ) ) 
return 1; 
return 0; 
} 
#line 103 "/usr/local/go/src/pkg/runtime/sema.goc"
void 
runtime·semacquire ( uint32 volatile *addr , bool profile ) 
{ 
SemaWaiter s; 
SemaRoot *root; 
int64 t0; 
#line 111 "/usr/local/go/src/pkg/runtime/sema.goc"
if ( cansemacquire ( addr ) ) 
return; 
#line 120 "/usr/local/go/src/pkg/runtime/sema.goc"
root = semroot ( addr ) ; 
t0 = 0; 
s.releasetime = 0; 
if ( profile && runtime·blockprofilerate > 0 ) { 
t0 = runtime·cputicks ( ) ; 
s.releasetime = -1; 
} 
for ( ;; ) { 
runtime·lock ( root ) ; 
#line 130 "/usr/local/go/src/pkg/runtime/sema.goc"
runtime·xadd ( &root->nwait , 1 ) ; 
#line 132 "/usr/local/go/src/pkg/runtime/sema.goc"
if ( cansemacquire ( addr ) ) { 
runtime·xadd ( &root->nwait , -1 ) ; 
runtime·unlock ( root ) ; 
return; 
} 
#line 139 "/usr/local/go/src/pkg/runtime/sema.goc"
semqueue ( root , addr , &s ) ; 
runtime·park ( runtime·unlock , root , "semacquire" ) ; 
if ( cansemacquire ( addr ) ) { 
if ( t0 ) 
runtime·blockevent ( s.releasetime - t0 , 3 ) ; 
return; 
} 
} 
} 
#line 149 "/usr/local/go/src/pkg/runtime/sema.goc"
void 
runtime·semrelease ( uint32 volatile *addr ) 
{ 
SemaWaiter *s; 
SemaRoot *root; 
#line 155 "/usr/local/go/src/pkg/runtime/sema.goc"
root = semroot ( addr ) ; 
runtime·xadd ( addr , 1 ) ; 
#line 161 "/usr/local/go/src/pkg/runtime/sema.goc"
if ( runtime·atomicload ( &root->nwait ) == 0 ) 
return; 
#line 165 "/usr/local/go/src/pkg/runtime/sema.goc"
runtime·lock ( root ) ; 
if ( runtime·atomicload ( &root->nwait ) == 0 ) { 
#line 169 "/usr/local/go/src/pkg/runtime/sema.goc"
runtime·unlock ( root ) ; 
return; 
} 
for ( s = root->head; s; s = s->next ) { 
if ( s->addr == addr ) { 
runtime·xadd ( &root->nwait , -1 ) ; 
semdequeue ( root , s ) ; 
break; 
} 
} 
runtime·unlock ( root ) ; 
if ( s ) { 
if ( s->releasetime ) 
s->releasetime = runtime·cputicks ( ) ; 
runtime·ready ( s->g ) ; 
} 
} 
#line 188 "/usr/local/go/src/pkg/runtime/sema.goc"
void net·runtime_Semacquire ( uint32 *addr ) 
{ 
runtime·semacquire ( addr , true ) ; 
} 
#line 193 "/usr/local/go/src/pkg/runtime/sema.goc"
void net·runtime_Semrelease ( uint32 *addr ) 
{ 
runtime·semrelease ( addr ) ; 
} 
void
sync·runtime_Semacquire(uint32* addr)
{
#line 198 "/usr/local/go/src/pkg/runtime/sema.goc"

	runtime·semacquire(addr, true);
}
void
sync·runtime_Semrelease(uint32* addr)
{
#line 202 "/usr/local/go/src/pkg/runtime/sema.goc"

	runtime·semrelease(addr);
}

#line 206 "/usr/local/go/src/pkg/runtime/sema.goc"
typedef struct SyncSema SyncSema; 
struct SyncSema 
{ 
Lock; 
SemaWaiter* head; 
SemaWaiter* tail; 
} ; 
void
sync·runtime_Syncsemcheck(uintptr size)
{
#line 214 "/usr/local/go/src/pkg/runtime/sema.goc"

	if(size != sizeof(SyncSema)) {
		runtime·printf("bad SyncSema size: sync:%D runtime:%D\n", (int64)size, (int64)sizeof(SyncSema));
		runtime·throw("bad SyncSema size");
	}
}
void
sync·runtime_Syncsemacquire(SyncSema* s)
{
#line 222 "/usr/local/go/src/pkg/runtime/sema.goc"

	SemaWaiter w, *wake;
	int64 t0;

	w.g = g;
	w.nrelease = -1;
	w.next = nil;
	w.releasetime = 0;
	t0 = 0;
	if(runtime·blockprofilerate > 0) {
		t0 = runtime·cputicks();
		w.releasetime = -1;
	}

	runtime·lock(s);
	if(s->head && s->head->nrelease > 0) {
		// have pending release, consume it
		wake = nil;
		s->head->nrelease--;
		if(s->head->nrelease == 0) {
			wake = s->head;
			s->head = wake->next;
			if(s->head == nil)
				s->tail = nil;
		}
		runtime·unlock(s);
		if(wake)
			runtime·ready(wake->g);
	} else {
		// enqueue itself
		if(s->tail == nil)
			s->head = &w;
		else
			s->tail->next = &w;
		s->tail = &w;
		runtime·park(runtime·unlock, s, "semacquire");
		if(t0)
			runtime·blockevent(w.releasetime - t0, 2);
	}
}
void
sync·runtime_Syncsemrelease(SyncSema* s, uint32 n)
{
#line 264 "/usr/local/go/src/pkg/runtime/sema.goc"

	SemaWaiter w, *wake;

	w.g = g;
	w.nrelease = (int32)n;
	w.next = nil;
	w.releasetime = 0;

	runtime·lock(s);
	while(w.nrelease > 0 && s->head && s->head->nrelease < 0) {
		// have pending acquire, satisfy it
		wake = s->head;
		s->head = wake->next;
		if(s->head == nil)
			s->tail = nil;
		if(wake->releasetime)
			wake->releasetime = runtime·cputicks();
		runtime·ready(wake->g);
		w.nrelease--;
	}
	if(w.nrelease > 0) {
		// enqueue itself
		if(s->tail == nil)
			s->head = &w;
		else
			s->tail->next = &w;
		s->tail = &w;
		runtime·park(runtime·unlock, s, "semarelease");
	} else
		runtime·unlock(s);
}
