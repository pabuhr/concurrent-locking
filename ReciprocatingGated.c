// Dave Dice and Alex Kogan
// Reciprocating Locks
// PPoPP 2025
// https://doi.org/10.1145/3710848.3710862 (ACM)
// https://arxiv.org/abs/2501.02380 (Long form)
// The Reciprocating lock gains performance by a controlled (small) amount of unfairness.
//
// The form below corresponds to Listing-8 in arXiv.  
//
// Particapating threads are in one of the following states : 
// @  Owner
// @  waiting on detached arrival segment
// @  leader waiting on Gate 
// @  waiting on attached entry segment 
//
// We use Gate as an interlock to separate entry and arrival segment generations.  Each generation has one distinguished
// thread in the leader role.  At most one such leader thread waits on Gate at any one time.  Note the analogy here to
// MCSH and Fissile Locks.

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdatomic.h>

typedef struct _WaitElement {
	_Atomic(struct _WaitElement *) EndOfSegment CALIGN;
} WaitElement;

typedef struct {
	_Atomic(WaitElement *) Arrivals;
	_Atomic(int) Gate CALIGN; 
} ReciprocatingLock;

static ReciprocatingLock lock CALIGN;

// Usage of pass() isn't necessary for correctness, but it inhibits some compile-time optimizations that turn out to be
// detrimental to performance.  The effect does not appear on ARMv8+LSE but does on x86, although there it is dependent
// on compiler version and whether we place "E" on-stack or in TLS.

static inline WaitElement * pass( WaitElement * v ) { 
	asm volatile( "" : "+r" (v) );
	return v; 
} 

static inline WaitElement * Acquire( ReciprocatingLock * lock, WaitElement ** _EndOfSegment ) {
	// E can safely reside either on-frame or in a TLS singleton See the arXiv form Appendix-B for a discussion of the
	// trade-offs.
	WaitElement E CALIGN;
	Str( E.EndOfSegment, NULL );
	WaitElement * EndOfSegment = pass( &E );
	WaitElement * prv = Fas( lock->Arrivals, &E );
	assert(prv != &E);
	if ( prv != NULL ) { 
		// Follower role ... 
		// contention : waiting phase
		// Consider : could use HemLock CTR optimization here and spin using exchange
		// That, in turn, would obviate the need to clear Gate at the top of Acquire
		// and would avoid the MESI/MOESI/MESIF S->M coherence upgrade.
		// We have private and local busy waiting ...
		for ( ;; ) {
			EndOfSegment = Ld( E.EndOfSegment );
			if ( EndOfSegment != NULL ) break;
			Pause();
		}
		assert( EndOfSegment != &E );
	} else {
		// Leader role ...
		// Wait for the previous generation to vacate 
		// We have at most 1v1 contention on Gate at this juncture : 1 waiter vs 1 owner
		//
		// We enjoy private spinning, below, with at most one thread spinning on Gate at any given time, but Gate is not
		// necessarly local (NUMA co-homed) to the waiting thread, which can make a performance difference on Intel
		// home-based UPI fabrics.  This is easy to fix via indirection, however, where the waiting thread installs a
		// pointer to a local waiting location into lock->Gate.  This approach also permits us to retain performance
		// even if we place the Gate and Arrivals fields on the same cache line, which in turn results in a very small
		// lock body.
		// We might then also employ the HemLock CTR "coherence traffic reduction" optimization, where we busy-wait with
		// an atomic exchange.  Currently, to avoid false sharing between Gate and Arrivals, we sequester those fields
		// via alignment.  This works perfectly well, but bloats the lock body size to 2 sectors (lines).
		while ( Ld( lock->Gate ) != 0 ) { 
			Pause(); 
		}
		Str( lock->Gate, 1 ); 

		// Note that we can safely detach the next segment here, early, before we run the critical section, or later,
		// after the critical section.  Early release, here and now, helps unify the Release() code.
		//
		// When we place E on-frame and concerns about UB dangling references ...  Critically, E remains live, extant
		// and in-scope at the point in time, immediately below, where we detach and privatize the segment on which E is
		// resident.  E resides at the distal end of that segment and may be buried or submerged on the chain -- zombie.
		// The segment, once detached, is "closed" and no threads can join by pushing (prepending) onto the concurrent
		// pop-stack.  As no threads can join, we are not exposed to aliasing false-equal address-based end-of-segment
		// sentinel check pathologies.  Placing E in TLS obviates all such lifecycle concerns.
		prv = Fas( lock->Arrivals, NULL ); 
		assert( prv != NULL ); 
	}

	assert( Ld( lock->Gate) != 0 ); 
	// Consider: for the leader thread, we know EndOfSegment (==&E) early, so might lift the following store "up" above
	// the leader waiting loop to reduce code in the critical path.
	*_EndOfSegment = EndOfSegment;
	return prv;
}

static inline void Release( ReciprocatingLock * lock, WaitElement * EndOfSegment, WaitElement * prv ) {
	assert( prv != NULL );
	assert( EndOfSegment != NULL ); 
	assert( Ld( lock->Gate ) != 0 ); 

	if ( prv != EndOfSegment ) { 
		assert( Ld( prv->EndOfSegment) == NULL ); 
		Str( prv->EndOfSegment, EndOfSegment ); 
	} else {
		Str( lock->Gate, 0 );
	}
}

static void * Worker( void * arg ) {
    TYPE id = (size_t)arg;
    uint64_t entry;

#ifdef FAST
    unsigned int cnt = 0, oid = id;
#endif // FAST

    NCS_DECL;

    // Note that E could either reside in thread_local, or, in this case, on-stack

    for ( int r = 0; r < RUNS; r += 1 ) {
        RTYPE randomThreadChecksum = 0;

        for ( entry = 0; stop == 0; entry += 1 ) {
            NCS;

            // EndOfSegment and prv reflect context to be passed from Acquire to corresponding Release
            // CLH, MCS, and MCSH similarly are _not context-free.
            // Could also pass the context via fields in the lock body or into TLS.
            // Might also use C++ RAII std::scoped_lock or friends to carry context.
            WaitElement * EndOfSegment;
            WaitElement * prv = Acquire( &lock, &EndOfSegment );
            randomThreadChecksum += CS( id );
            Release( &lock, EndOfSegment, prv );

#ifdef FAST
            id = startpoint( cnt );                     // different starting point each experiment
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
	Str( lock.Gate, 0); 
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=ReciprocatingGated Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
