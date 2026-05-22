#include "BarrierCallback.h"

typedef struct CALIGN {
	VTYPE release;
    VTYPE ord;											// position in stack
} Pair; 

typedef struct {
	TYPE group;
	Pair * head; 
	CBDECL();
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline bool block( Barrier * b ) {
	CBSTART();											// must be first
	
	Pair mm = { .release = false, .ord = 0 };
	Pair * prev = Fas( b->head, &mm ); 					// atomic instruction

	if ( LIKELY( prev ) ) {
		TYPE temp;
		await( temp = prev->ord );
		mm.ord = 1 + temp;
	} else {
		mm.ord = 1;
	} // if

	if ( LIKELY( mm.ord < b->group ) ) {
		await( mm.release );
		if ( LIKELY( prev ) ) prev->release = true;
		return false;
	} // if
	CBEND();										// must appear in safe location
	b->head = NULL;
	if ( LIKELY( prev ) ) prev->release = true;
	return true;
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .group = N, .head = NULL CBINIT() };
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierStack Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
