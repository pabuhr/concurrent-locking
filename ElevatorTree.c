// Peter A. Buhr, David Dice and Wim H. Hesselink, High-Contention Mutual Exclusion by Elevator Algorithms, Concurrency
// and Computation: Practice and Experience, 30(18), September 2018, https://doi.org/10.1002/cpe.4475

#include <stdbool.h>

//======================================================

typedef TYPE QElem_t;

typedef struct CALIGN {
	QElem_t * elements;
	unsigned int front, rear;
} Queue;

static inline bool QnotEmpty( volatile Queue * queue ) {
	return queue->front != queue->rear;
} // QnotEmpty

static inline void Qenqueue( volatile Queue * queue, QElem_t element ) {
	queue->elements[queue->rear] = element;
	queue->rear = cycleUp( queue->rear, N );
} // Qenqueue

static inline QElem_t Qdequeue( volatile Queue * queue ) {
	QElem_t element = queue->elements[queue->front];
	queue->front = cycleUp( queue->front, N );
	return element;
} // Qdequeue

static inline void Qctor( Queue * queue ) {
	queue->front = queue->rear = 0;
	queue->elements = Allocator( N * sizeof(typeof(queue->elements[0])) );
} // Qctor

static inline void Qdtor( Queue * queue ) {
	free( queue->elements );
} // Qdtor

Queue queue CALIGN;

//======================================================

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static VTYPE fast CALIGN;
#ifndef CAS
static VTYPE * b CALIGN, x CALIGN, y CALIGN;
#endif // ! CAS
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define await( E ) while ( ! (E) ) Pause()

#ifdef CAS

#define trylock( x ) __sync_bool_compare_and_swap( &(fast), false, true )

#elif defined( WCasBL )

static inline bool trylock( TYPE id ) {					// based on Burns-Lamport algorithm
	b[id] = true;
	Fence();											// force store before more loads
	for ( typeof(id) thr = 0; thr < id; thr += 1 ) {
		if ( FASTPATH( b[thr] ) ) {
			b[id] = false;
			return false ;
		} // if
	} // for
	for ( typeof(id) thr = id + 1; thr < N; thr += 1 ) {
		await( ! b[thr] );
	} // for
	bool leader;
	if ( ! fast ) { fast = true; leader = true; }
	else leader = false;
	b[id] = false;
	return leader;
} // WCas

#elif defined( WCasLF )

static inline bool trylock( TYPE id ) {					// based on Lamport-Fast algorithm
	b[id] = true;
	x = id;
	Fence();											// force store before more loads
	if ( FASTPATH( y != N ) ) {
		b[id] = false;
		return false;
	} // if
	y = id;
	Fence();											// force store before more loads
	if ( FASTPATH( x != id ) ) {
		b[id] = false;
		Fence();										// OPTIONAL, force store before more loads
		for ( typeof(N) k = 0; k < N; k += 1 )
			await( ! b[k] );
		if ( FASTPATH( y != id ) ) return false;
	} // if
	bool leader;
	if ( ! fast ) { fast = true; leader = true; }
	else leader = false;
	y = N;
	b[id] = false;
	return leader;
} // WCas

#else
	#error unsupported architecture
#endif // WCas

#define unlock() fast = false

//======================================================

static TYPE PAD3 CALIGN __attribute__(( unused ));		// protect further false sharing
#ifdef FLAG
static VTYPE * flags CALIGN;
#else
static VTYPE first CALIGN;
#endif // FLAG
static VTYPE * vals CALIGN;
static TYPE PAD4 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	const unsigned int n = N + id, dep = Log2( n );
	// _Atomic qualifier not preserved in typeof => VTYPE
	VTYPE * valId = &vals[n];							// optimizations
	#ifdef FLAG
	VTYPE * flagId = &flags[id];
	VTYPE * flagN = &flags[N];
	#endif // FLAG

	#ifdef FAST
	typeof(id) cnt = 0, oid = id;
	#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			// loop goes from leaf to child of root
			for ( unsigned int j = n; j > 1; j >>= 1 )	// entry protocol
				vals[j] = id;

			if ( FASTPATH( trylock( id ) ) ) {			// true => leader
				#ifndef CAS
				Fence();								// force store before more loads
				#endif // ! CAS

				#ifdef FLAG
				await( *flagId || *flagN );
				*flagN = false;
				#else
				await( first == id || first == N );
				first = id;
				#endif // FLAG
				unlock();
			} else {
				#ifdef FLAG
				await( *flagId );
				#else
				await( first == id );
				#endif // FLAG
			} // if
			*valId = N;
			#ifdef FLAG
			*flagId = false;
			#endif // FLAG

			randomThreadChecksum += CriticalSection( id );

			// loop goes from child of root to leaf and inspects siblings
			for ( int j = dep - 1; j >= 0; j -= 1 ) {	// must be "signed"
				VTYPE k = vals[(n >> j) ^ 1], *wid = &(vals[N + k]);
				if ( FASTPATH( *wid < N ) ) {
					*wid = N;
					Qenqueue( &queue, k );
				} // if
			} // for

			if ( FASTPATH( QnotEmpty( &queue ) ) )
			#ifdef FLAG
				flags[Qdequeue( &queue )] = true;
			else
				*flagN = true;
			#else
				first = Qdequeue( &queue );
			else
				first = N;
			#endif // FLAG

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			valId = &vals[N + id].val;					// must reset
			#ifdef FLAG
			flagId = &flags[id].flag;
			flagN = &flags[N].flag;
			#endif // FLAG
			#endif // FAST
		} // for

		__sync_fetch_and_add( &sumOfThreadChecksums, randomThreadChecksum );

		#ifdef FAST
		id = oid;

		valId = &vals[N + id].val;						// must reset
		#ifdef FLAG
		flagId = &flags[id].flag;
		flagN = &flags[N].flag;
		#endif // FLAG
		#endif // FAST
		entries[r][id] = entry;
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for

	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	Qctor( &queue );
	#ifdef FLAG
	flags = Allocator( (N + 1) * sizeof(typeof(flags[0])) );
	for ( typeof(N) id = 0; id <= N; id += 1 ) {		// initialize shared data
		flags[id] = false;
	} // for
	#endif // FLAG

	#ifdef FLAG
	flags[N] = true;
	#else
	first = N;
	#endif // FLAG

	vals = Allocator( (2 * N + 1) * sizeof(typeof(vals[0])) );
	for ( typeof(N) id = 0; id <= 2 * N; id += 1 ) {
		vals[id] = N;
	} // for

	#ifndef CAS
	b = Allocator( N * sizeof(typeof(b[0])) );
	for ( typeof(N) id = 0; id < N; id += 1 ) {			// initialize shared data
		b[id] = false;
	} // for
	y = N;
	#endif // CAS
	unlock();
} // ctor

void __attribute__((noinline)) dtor() {
	#ifndef CAS
	free( (void *)b );
	#endif // ! CAS
	free( (void *)vals );
	#ifdef FLAG
	free( (void *)flags );
	#endif // FLAG
	Qdtor( &queue );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=ElevatorTree -DWCasLF Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
