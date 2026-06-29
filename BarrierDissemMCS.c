// John M. Mellor-Crummey and Michael L. Scott, Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors,
// ACM Transactions on Computer Systems, 9(1), February 1991, Fig. 10, p. 38

// Note algorithm uses ranges 0..Clog2( N ) - 1 == Clog2( N ) for C dimension.

// For TSO, no fencing required because reads and writes are disjoint: 0 != p + 1, so eventual progress updates the
// reader. However, there is a delay seeing the read value. Unclear whether the fence or delay is more costly.

// Cannot have callback/distinguished-thread without changing from symmetric.

struct flags {
	VTYPE ** my_flags, *** partner_flags;
};

typedef struct {
	TYPE CALIGN group;
	TYPE exponent;
	struct flags * allnodes;
} Barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL TYPE sense = true, parity = false;
#define BARRIER_CALL block( &b, p, &sense, &parity );

static inline void block( Barrier * b, TYPE p, TYPE * sense, TYPE * parity ) { 
	TYPE lsense = *sense, lparity = *parity;			// optimization (compiler probably does it)

	for ( TYPE i = 0; i < b->exponent; i += 1 ) {
		*b->allnodes[p].partner_flags[lparity][i] = lsense;
		// Fence(); // TSO optional
		await( b->allnodes[p].my_flags[lparity][i] == lsense );
	} // for
	if ( lparity ) *sense = ! lsense;
	*parity = ! lparity;
	Fence();
} // block

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (Barrier){ .exponent = Clog2( N ), .allnodes = Allocator( N * sizeof(b.allnodes[0]) ) };
	// for each node
	for ( TYPE i = 0; i < N; i += 1 ) {
		// alloc my flag array
		b.allnodes[i].my_flags = Allocator( 2 * sizeof(b.allnodes[i].my_flags[0]) );
		b.allnodes[i].my_flags[0] = Allocator( b.exponent * sizeof(b.allnodes[i].my_flags[0][0]) );
		b.allnodes[i].my_flags[1] = Allocator( b.exponent * sizeof(b.allnodes[i].my_flags[1][0]) );
		
		// alloc partner flag arrays
		b.allnodes[i].partner_flags = Allocator( 2 * sizeof(b.allnodes[i].partner_flags[0]) );
		b.allnodes[i].partner_flags[0] = Allocator( b.exponent * sizeof(b.allnodes[i].partner_flags[0][0]) );
		b.allnodes[i].partner_flags[1] = Allocator( b.exponent * sizeof(b.allnodes[i].partner_flags[1][0]) );
	} // for

	// init flag arrays
	TYPE pow2 = 1, partner_idx;
	for ( TYPE c = 0; c < b.exponent; c += 1 ) {
		for ( TYPE i = 0; i < N; i += 1 ) {
			partner_idx = (pow2 + i) % N;
			for ( TYPE r = 0; r < 2; r += 1 ) {
				b.allnodes[i].my_flags[r][c] = 0;
				b.allnodes[i].partner_flags[r][c] = &b.allnodes[partner_idx].my_flags[r][c];
			} // for
		} // for
		pow2 *= 2;
	} // for
} // ctor

void __attribute__((noinline)) dtor() {
	for ( TYPE i = 0; i < N; i += 1 ) {
		free( b.allnodes[i].partner_flags[1] );
		free( b.allnodes[i].partner_flags[0] );
		free( b.allnodes[i].partner_flags );

		free( (void *)b.allnodes[i].my_flags[1] );
		free( (void *)b.allnodes[i].my_flags[0] );
		free( b.allnodes[i].my_flags );
	} // for
	free( b.allnodes );
	worker_dtor();
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DBARRIER -DAlgorithm=BarrierDissemMCS Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
