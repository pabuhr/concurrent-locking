// Wim H. Hesselink, P. Buhr, D, Dice, version 25 June 2014

#include <stdbool.h>

//======================================================

typedef TYPE QElem_t;

typedef struct CALIGN {
	QElem_t *elements;
	unsigned int front, rear;
} Queue;

static inline bool QnotEmpty( volatile Queue *queue ) {
	return queue->front != queue->rear;
} // QnotEmpty

static inline void Qenqueue( volatile Queue *queue, QElem_t element ) {
	queue->elements[queue->rear] = element;
	queue->rear = cycleUp( queue->rear, N );
} // Qenqueue

static inline QElem_t Qdequeue( volatile Queue *queue ) {
	QElem_t element = queue->elements[queue->front];
	queue->front = cycleUp( queue->front, N );
	return element;
} // Qdequeue

static inline void Qctor( Queue *queue ) {
	queue->front = queue->rear = 0;
	queue->elements = Allocator( N * sizeof(typeof(queue->elements[0])) );
} // Qctor

static inline void Qdtor( Queue *queue ) {
	free( queue->elements );
} // Qdtor

Queue queue CALIGN;

//======================================================

typedef struct CALIGN {									// performance gain when fields juxtaposed
	TYPE apply;
#ifdef FLAG
	TYPE fast;
#endif // FLAG
} Tstate;

static volatile TYPE fast CALIGN;
static volatile Tstate *tstate CALIGN;
//static volatile TYPE *apply CALIGN;
static volatile TYPE *val CALIGN;

#ifndef CAS
static volatile TYPE *b CALIGN, x CALIGN, y CALIGN;
#endif // ! CAS

#ifdef FLAG
//static volatile TYPE *fast CALIGN;
#else
static volatile TYPE lock CALIGN;
#endif // FLAG

#define await( E ) while ( ! (E) ) Pause()

#ifdef CAS
// Paper uses true/false versus id/N, but id/N matches with non-CAS interface.
#define muentry( x ) __sync_bool_compare_and_swap( &(fast), (N), (x) )

#else // ! CAS

static inline bool muentry( TYPE id ) {
// Only needed in Triangle algorithm
//	if ( FASTPATH( y != N ) ) return false;
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
		Fence();										// force store before more loads
		for ( int j = 0; y == id && j < N; j += 1 )
			await( ! b[j] );
		if ( FASTPATH( y != id ) ) return false;
	} // if
	bool leader = ! fast ? fast = true, true : false;
	y = N;
	b[id] = false;
	return leader;
} // muentry
#endif // CAS

static TYPE PAD CALIGN __attribute__(( unused ));		// protect further false sharing

static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	const unsigned int n = N + id, dep = Log2( n );
	volatile typeof(tstate[0].apply) *tApplyId = &tstate[id].apply;
#ifdef FLAG
	volatile typeof(tstate[0].fast) *tFastId = &tstate[id].fast;
	volatile typeof(tstate[0].fast) *tFastN = &tstate[N].fast;
#endif // FLAG

#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			*tApplyId = true;
			// loop goes from parent of leaf to child of root
			for ( unsigned int j = (n >> 1); j > 1; j >>= 1 )
				val[j] = id;

#ifndef CAS
			Fence();									// force store before more loads
#endif // ! CAS

			if ( FASTPATH( muentry( id ) ) ) {
#ifndef CAS
				Fence();								// force store before more loads
#endif // ! CAS

#ifdef FLAG
				await( *tFastN || *tFastId );
				*tFastN = false;
#else
				await( lock == N || lock == id );
				lock = id;
#endif // FLAG

#ifndef CAS
				fast = false;
#else
				fast = N;
#endif // ! CAS

			} else {
#ifdef FLAG
				await( *tFastId );
#else
				await( lock == id );
#endif // FLAG
			} // if
#ifdef FLAG
			*tFastId = false;
#endif // FLAG
			*tApplyId = false;

			CriticalSection( id );

			// loop goes from child of root to leaf and inspects siblings
			for ( int j = dep - 1; j >= 0; j -= 1 ) {	// must be "signed"
				unsigned int k = val[(n >> j) ^ 1];
				if ( FASTPATH( tstate[k].apply ) ) {
					tstate[k].apply = false;
					Qenqueue( &queue, k );
				} // if
			}  // for
			if ( FASTPATH( QnotEmpty( &queue ) ) ) {
#ifdef FLAG
				tstate[Qdequeue( &queue )].fast = true;
#else
				lock = Qdequeue( &queue );
#endif // FLAG
			} else
#ifdef FLAG
				*tFastN = true;
#else
				lock = N;
#endif // FLAG

#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			entry += 1;
		} // while
#ifdef FAST
		id = oid;
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
	tstate = Allocator( (N+1) * sizeof(typeof(tstate[0])) );
	for ( TYPE id = 0; id <= N; id += 1 ) {				// initialize shared data
		tstate[id].apply = false;
#ifdef FLAG
		tstate[id].fast = false;
#endif // FLAG
	} // for

#ifdef FLAG
	tstate[N].fast = true;
#else
	lock = N;
#endif // FLAG

	val = Allocator( 2 * N * sizeof(typeof(val[0])) );
	for ( TYPE id = 0; id < N; id += 1 ) {				// initialize shared data
		val[id] = N;
		val[N + id] = id;
	} // for

#ifdef CAS
	fast = N;
#else
	b = Allocator( N * sizeof(typeof(b[0])) );
	for ( TYPE id = 0; id < N; id += 1 ) {				// initialize shared data
		b[id] = false;
	} // for
	y = N;
	fast = false;
#endif // CAS
} // ctor

void __attribute__((noinline)) dtor() {
#ifndef CAS
	free( (void *)b );
#endif // ! CAS

	free( (void *)val );
	free( (void *)tstate );
	Qdtor( &queue );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=ElevatorQ Harness.c -lpthread -lm" //
// End: //
