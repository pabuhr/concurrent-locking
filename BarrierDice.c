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
	WaitElement * pred = Fas( b, &W );
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
		WaitElement * DetachedList = Fas( b, NULL );
		assert( DetachedList == &W );
		assert( DetachedList->Gate == 0 );
		#endif
		if ( pred != NULL ) {
			assert( pred->Gate == 0 );
			pred->Gate = 1;								// propagate notification through the stack
		} // if
	} // if
} // block

//#define TESTING
#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierDice Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
