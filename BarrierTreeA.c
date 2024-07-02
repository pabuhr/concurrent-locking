typedef VTYPE CALIGN * Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( b, p );

static inline void block( Barrier aa, TYPE p ) {
	TYPE aap = aa[p];									// optimization
	if ( p == 0 ) {
		#if 0
		TYPE sp, stack[N];								// Lamport attempt to reduce busy wait
		for ( sp = 1; sp < N; sp += 1 ) stack[sp] = sp;
		while ( sp > 1 ) {								// skip p[0]
			for ( TYPE i = 1; i < sp; ) {
				if ( aap != aa[stack[i]] ) {
					sp -= 1;
					stack[i] = stack[sp];
				} else {
					i += 1;
				} // if
			} // for
		} // while
		#else
		for ( TYPE kk = 1; kk < N ; kk += 1 ) {			// skip p[0]
			await( aap != aa[kk] );
		} // for
		#endif // 0
		aa[0] = ! aap;									// release waiting threads
		Fence();
	} else {
		aap = ! aap;
		aa[p] = aap;
		Fence();
		await( aap == aa[0] );							// wait for p0 to see all waiting threads
	} // if
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = Allocator( sizeof(typeof(b[0])) * N );
	for ( size_t i = 0; i < N; i += 1 ) {
		b[i] = false;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	free( (void *)b );
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierTreeA Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
