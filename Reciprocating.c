// Dave Dice and Alex Kogan
// Reciprocating Locks
// PPoPP 2025
// https://doi.org/10.1145/3710848.3710862 (ACM)
// The code below closely follows Listing-1 in the above.
// https://arxiv.org/abs/2501.02380 (Long form)

// The Reciprocating lock gains performance by a controlled amount of unfairness, while ensuring an anti-starvation
// guarantee. If thread T1 is waiting and T2 arrives after T1, then T2 can bypass T1 at most once before T1 gains
// ownership. The maximum possible long-term unfairness induced by the lock's admission schedule is 2x, where the
// fastest thread can only be admitted twice as often as the most unlucky thread over long periods.

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdatomic.h>

typedef struct _WaitElement {
	_Atomic(struct _WaitElement *) Gate CALIGN;
} WaitElement;

static WaitElement * const LOCKEDEMPTY = (WaitElement *)(uintptr_t) 1;

typedef struct {
	_Atomic(WaitElement *) Arrivals;
} ReciprocatingLock;

static __thread WaitElement E CALIGN;

static ReciprocatingLock lock CALIGN;

static inline WaitElement * Acquire( ReciprocatingLock * lock, WaitElement ** _EndOfSegment ) {
	Str( E.Gate, NULL );
	WaitElement * succ = NULL;
	WaitElement * EndOfSegment = &E;
	WaitElement * const tail = Fas( lock->Arrivals, &E );
	assert( tail != &E );
	if ( tail != NULL ) {
		// coerce LOCKEDEMPTY to null
		// succ will be our successor when we subsequently release
		succ = (WaitElement *)(((uintptr_t) tail) & ~1);
		assert(succ != &E);

		// contention : waiting phase
		// Consider : could use HemLock CTR optimization here and spin using exchange
		// That, in turn, would obviate the need to clear Gate at the top of Acquire
		// and would avoid the MESI/MOESI/MESIF S->M coherence upgrade.
		for ( ;; ) {
			EndOfSegment = Ld( E.Gate );
			if ( EndOfSegment != NULL ) break;
			Pause();
		}
		assert( EndOfSegment != &E );

		// Detect logical end-of-segment terminus address
		if ( succ == EndOfSegment ) {
			succ = NULL;								// quash
			EndOfSegment = LOCKEDEMPTY;
		}
	}

	*_EndOfSegment = EndOfSegment;
	return succ;
}

static inline void Release( ReciprocatingLock * lock, WaitElement * EndOfSegment, WaitElement * succ ) {
	assert( EndOfSegment != NULL );
	assert( Ld( lock->Arrivals ) != NULL );

	if ( succ != NULL ) {
		assert( Ld( succ->Gate ) == NULL );
		Str( succ->Gate, EndOfSegment );
		return;
	}

	assert( EndOfSegment == LOCKEDEMPTY || EndOfSegment == &E );
#if 0
	WaitElement * v = EndOfSegment;
	if ( Casv( lock->Arrivals, v, NULL ) ) {
		// uncontended fast-path return
		return;
	}
#else
	if ( Ld( lock->Arrivals ) == EndOfSegment ) {
		WaitElement * v = EndOfSegment;
		if ( Casv( lock->Arrivals, v, NULL ) ) {
			// uncontended fast-path return
			return;
		}
	}
#endif

	WaitElement * w = Fas( lock->Arrivals, LOCKEDEMPTY );
	assert( w != NULL );
	assert( w != LOCKEDEMPTY );
	assert( w != &E );
	assert( Ld( w->Gate ) == NULL );
	Str( w->Gate, EndOfSegment );
}

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	NCS_DECL;

    // Note that E could either reside in thread_lock, or, in this case, on-stack

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

            // EndOfSegment and succ reflect context to be passed from Acquire to corresponding Release
            // CLH, MCS, and MCSH similarly are _not context-free.
            // With a slightly more clever encoding we can readily collapse the 2 fields into just 1.
            // But 2 fields is easier for the purpose of explication.
            // Could also pass the context via fields in the lock body or into TLS.
            // Might also use C++ RAII std::scoped_lock or friends to carry context.
            WaitElement * EndOfSegment;
            WaitElement * succ = Acquire( &lock, &EndOfSegment );

			randomThreadChecksum += CS( id );

            Release( &lock, EndOfSegment, succ );

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		Fai( sumOfThreadChecksums, randomThreadChecksum );

		#ifdef FAST
		id = oid;
		#endif // FAST
		entries[r][id] = entry;
		Fai( Arrived, 1 );
		while ( stop != 0 ) Pause();
		Fai( Arrived, -1 );
	} // for

	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	Str( lock.Arrivals, NULL );
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=Reciprocating Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
