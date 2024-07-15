// Algorithm properties:
//
// * probably invented before (?)
// 
// * not many lines of code
// 
// * uses only swap/exchange/FAS.
// 
// * compact in terms of memory usage
// 
// * no TLS or dynamic memory allocation
// 
// * the algorithm assumes that the number of participating threads in the system is the same as a barrier consensus
//   number.  This isn't true in all uses of barriers, however.  For instance you might have 20 threads circulating and
//   want to hold at a barrier until 10 have arrived and then release those 10.  If we have 20 threads in play and the
//   barrier consensus "N" is 10, we need to support inter-generational waiting.  With a few changes to the algorithm,
//   we could support that mode of use.  This is always something of an interested topic of debate.
// 
// * we could skip the FAS that detaches the list of threads waiting at the barrier and just load it and then store null
//   in a a non-atomic fashion.  The FAS gives us better asserts, though.
// 
// * supports the pthreads requirement that a barrier can be immediately free()ed or destroyed after _any thread returns
//   from the barrier.
// 
// * local spinning
// 
// * amenable to various waiting schemes.  We can use Pause(), park-unpark, a futex, or even embed a mutex/condvar pair
//   in each of the wait elements and then wait in that fashion.  All waking is 1:1.
// 
// * On TSO I don't think we need any additional fences.   On weaker memory models we'll need fences, but not many.
// 
// * The progress properties of waiting for the Ordinal to resolve don't thrill me.  That waiting step is somewhat
//   analogous to that waiting loop we have in the MCS unlock() operator.
// 
// * the wakeup sequence is currently LIFO.  That's doesn't bother me.  But if that was a concern, we could easily
//   propagate the address of the head through the list in the same we currently propagate the ordinal values and also
//   add explicit wait element links ("Next" pointers) that flow from the head towards the tail.  This doesn't add much
//   overhead above and beyond the existing propagation of the Ordinal value.  When we trigger the barrier, we'd then
//   start at the head and propagate notification toward the tail - the reverse direction from what we currently have.
//   That is, with just a bit more effort, we could have FIFO/FCFS wakeup ordering at likely almost no impact on
//   performance. I don't have strong feeling on the wakeup order.
// 
// * The worst-case wakeup time could be pretty bad, as the notifications flow through the detached chain with each
//   thread waking its predecessor in turn.  Things get ugly if we're blocking our waiting threads in the kernel, as it
//   can take a long time to reach those threads buried deep on the stack, given unpark latencies.  It's possible,
//   though, to re-form the list into a binary wakeup tree.  There are also some helping tricks available, but all that
//   I know of involve firing potentially redundant unpark() operations that might in turn cause spurious wakeups.
//   That's fine in the world of park-unpark, but possibly not acceptable in your environment.

//   Inspired by CLH in that there's no explicit lists of threads, and a waiting thread knows only about its immediate
//   successor, whereas MCS, has "next" pointers in memory.  But it is unlike CLH in that it waits on a field in the
//   queued element, instead of the previous element.

typedef struct CALIGN waitelement {
	VTYPE Ordinal;
	VTYPE Gate;
} WaitElement;

typedef CALIGN WaitElement * barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static barrier b CALIGN = NULL;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline void block( barrier * b ) {
	WaitElement W = { .Gate = 0, .Ordinal = 0 };		// mark as not yet resolved
	WaitElement * pred = Fas( *b, &W );
	assert( pred != &W );
	if ( pred != NULL ) {
		await( pred->Ordinal != 0 );					// wait for predecessor's count to resolve
		W.Ordinal = pred->Ordinal + 1;
	} else {
		W.Ordinal = 1 ;
	} // if
	assert( W.Ordinal != 0 );
	if ( W.Ordinal < N ) {
		// intermediate thread
		await( W.Gate != 0 );							// primary waiting loop
		if ( pred != NULL ) {
			assert( pred->Gate == 0 );
			pred->Gate = 1;								// propagate notification through the stack
		} // if
	} else {
		// ultimate thread to arrive
		#ifdef NDEBUG
		*b = NULL;
		#else
		WaitElement * DetachedList = Fas( *b, NULL );
		assert( DetachedList == &W );
		assert( DetachedList->Gate == 0 );
		#endif
		if ( pred != NULL ) {
			assert( pred->Gate == 0 );
			pred->Gate = 1;								// propagate notification through the stack
		} // if
	} // if
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierPopStack Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
