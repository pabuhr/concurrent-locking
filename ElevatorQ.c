// Wim H. Hesselink, P. Buhr, D, Dice, version 25 June 2014

#include <stdbool.h>

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

typedef struct CALIGN {									// performance gain when fields juxtaposed
	TYPE apply;
#ifdef SIG
	TYPE sig;
#endif // SIG
} Tstate;

static volatile Tstate *tstate CALIGN;
//static volatile TYPE *apply CALIGN;
static volatile TYPE *val CALIGN;

#ifndef CAS
static volatile TYPE *b CALIGN, x CALIGN, y CALIGN, occupied CALIGN;
#endif // ! CAS

#ifdef SIG
//static volatile TYPE *sig CALIGN;
#else
static volatile TYPE lock CALIGN;
#endif // SIG

#ifdef QNE
static volatile TYPE qne CALIGN;						// invariant: implies queue nonempty
#endif // QNE

#define await( E ) while ( ! (E) ) Pause()

#ifdef CAS
static volatile TYPE mu CALIGN;
#define muentry(x) __sync_bool_compare_and_swap( &mu, N, (x) )

#else // ! CAS

static inline bool muentry( TYPE id ) {
	if ( FASTPATH( y != N ) ) return false;
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
		for ( int j = 0; j < N; j += 1 )
			await( y != id || ! b[j] );
		if ( FASTPATH( y != id ) ) return false;
	} // if
	bool leader;
	if ( ! occupied ) occupied = leader = true;			// leader selected
	else leader = false;
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
#ifdef SIG
	volatile typeof(tstate[0].sig) *tSigId = &tstate[id].sig;
	volatile typeof(tstate[0].sig) *tSigN = &tstate[N].sig;
#endif // SIG

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

#ifdef QNE
#ifdef CAS
			Fence();									// force store before more loads
#endif // CAS

			if ( FASTPATH( qne ) ) {
#ifdef SIG
				await( *tSigId );
#else
				await( lock == id );
#endif // SIG

			} else
#endif // QNE

				if ( FASTPATH( muentry( id ) ) ) {
#ifndef CAS
					Fence();							// force store before more loads
#endif // ! CAS

#ifdef SIG
					await( *tSigN || *tSigId );
					*tSigN = false;
#else
					await( lock == N || lock == id );
					lock = id;
#endif // SIG

#ifndef CAS
					occupied = false;
#else
					mu = N;
#endif // ! CAS

				} else {
#ifdef SIG
					await( *tSigId );
#else
					await( lock == id );
#endif // SIG
				} // if
#ifdef SIG
			*tSigId = false;
#endif // SIG
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
#ifdef QNE
				TYPE thread = Qdequeue( &queue );
				qne = QnotEmpty( &queue );

#ifdef SIG
				tstate[thread].sig = true;
#else
				lock = thread;
#endif // SIG

#else // ! QNE

#ifdef SIG
				tstate[Qdequeue( &queue )].sig = true;
#else
				lock = Qdequeue( &queue );
#endif // SIG
#endif // QNE
			} else
#ifdef SIG
				*tSigN = true;
#else
				lock = N;
#endif // SIG

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
	for ( TYPE id = 0; id <= N; id += 1 ) {		// initialize shared data
		tstate[id].apply = false;
#ifdef SIG
		tstate[id].sig = false;
#endif // SIG
	} // for

#ifdef SIG
	tstate[N].sig = true;
#else
	lock = N;
#endif // SIG

	val = Allocator( 2 * N * sizeof(typeof(val[0])) );
	for ( TYPE id = 0; id < N; id += 1 ) {		// initialize shared data
		val[id] = N;
		val[N + id] = id;
	} // for

#ifdef CAS
	mu = N;
#else
	b = Allocator( N * sizeof(typeof(b[0])) );
	for ( TYPE id = 0; id < N; id += 1 ) {		// initialize shared data
		b[id] = false;
	} // for
	y = N;
	occupied = false;
#endif // CAS

#ifdef QNE
	qne = false;
#endif // QNE
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
