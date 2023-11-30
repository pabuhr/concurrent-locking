typedef struct {
	TYPE free, high, inCS;
} barrier;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static barrier b CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL
#define BARRIER_CALL block( &b );

static inline void block( barrier * b ) {
	enum { M = 1 };

	TYPE ticket = Fai( &b->free, 1 );
	await( ticket <= b->high );
	if ( ticket == b->high ) {
		await( ticket + M <= b->free );
		await( b->inCS == 0 );
		b->inCS = M;
		b->high = ticket + M;
	} // if
} // block

static inline void release( barrier * b ) {
	Fai( &b->inCS, -1 );
} // release

//#define TESTING
static TYPE PAD3 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE CALIGN stopcnt = 0;
#ifdef TESTING
static VTYPE CALIGN * cnt;
#endif // TESTING
static TYPE PAD4 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE p = (size_t)arg;
	TYPE totalentries = 0;
	uint64_t entry;

	NCS_DECL;

	for ( int r = 0; r < RUNS; r += 1 ) {
		for ( entry = 0; stop == 0; entry += 1 ) {
			NCS;

			block( &b );								// may need & (address-of) for struct

			release( &b );
		} // for

		if ( p == 0 ) {
			while ( b->inCS > 0 ) {
				
				await( 
		} // if

		entries[r][p] = totalentries;
		totalentries = 0;
		#ifdef TESTING
		cnt[p] = 0;
		#endif // TESTING

		Fai( &Arrived, 1 );
		while ( stop ) Pause();
		if ( p == 0 ) b->high = 0;
		Fai( &Arrived, -1 );
	} // for

	return NULL;
} // Worker

void __attribute__((noinline)) worker_ctor() {
	#ifdef TESTING
	if ( N == 1 ) printf( " TESTING" );
	cnt = Allocator( sizeof(typeof(cnt[0])) * N );
	#endif // TESTING
} // worker_ctor

void __attribute__((noinline)) worker_dtor() {
	#ifdef TESTING
	free( (void *)cnt );
	#endif // TESTING
} // worker_dtor

void __attribute__((noinline)) ctor() {
	worker_ctor();
	b = (barrier){ 0, 0, 0 };
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierMulti Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //

