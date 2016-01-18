#include <stdbool.h>

//======================================================

#define FREE_NODE N
#define FREE_LOCK N

typedef TYPE QElem_t;

typedef struct CALIGN {
	QElem_t *elements;
	unsigned int *inQueue;
	unsigned int front, rear;
	unsigned int capacity;
} Queue;

static int toursize CALIGN;

static inline bool QnotEmpty( volatile Queue *queue ) {
	return queue->front != queue->rear;
} // QnotEmpty

static inline void Qenqueue( volatile Queue *queue, QElem_t element ) {
	if ( FASTPATH( element != FREE_NODE && queue->inQueue[element] == false ) ) {
		queue->elements[queue->rear] = element;
		queue->rear = cycleUp( queue->rear, queue->capacity );
//		queue->rear += 1;
//		if ( queue->rear == queue->capacity )
//			queue->rear=0; 		
		queue->inQueue[element] = true;
	} // if
} // Qenqueue

static inline QElem_t Qdequeue( volatile Queue *queue ) {
	QElem_t element = queue->elements[queue->front];
	queue->front = cycleUp( queue->front, queue->capacity );
//	queue->front += 1;
//	if ( queue->front == queue->capacity )
//		queue->front = 0;
	queue->inQueue[element] = false;
	return element;
} // Qdequeue

static inline Queue *Qctor( int maxElements ) {
	Queue *queue;
	queue = malloc( sizeof(typeof(*queue)) );
	queue->capacity = maxElements + 1;
	queue->front = queue->rear = 0;
	queue->elements = malloc( queue->capacity * sizeof(typeof(queue->elements[0])) );
	queue->inQueue = malloc( maxElements * sizeof(typeof(queue->inQueue[0])) );

	for ( unsigned int i = 0; i < maxElements; i += 1 ) {
		queue->inQueue[i] = false;
	} // for
	return queue;
} // Qctor

static inline void Qdtor( volatile Queue *queue ) {
	free( queue->inQueue );
	free( queue->elements );
	free( (void *)queue );
} // Qdtor

//======================================================

typedef struct CALIGN {
	unsigned int enter;
	unsigned int wait;
} Tstate;

static volatile Queue *q CALIGN;
static volatile TYPE **tournament CALIGN;
static volatile Tstate *arrState CALIGN;
static volatile TYPE first CALIGN;
static volatile TYPE exits CALIGN;
static TYPE PAD CALIGN __attribute__(( unused ));		// protect further false sharing

#define await( E ) while ( ! (E) ) Pause()

static void *Worker( void *arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	volatile typeof(arrState[0].enter) *tEnter = &arrState[id].enter;
	volatile typeof(arrState[0].wait) *tWait = &arrState[id].wait;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			*tEnter = true;

			// up hill
			typeof(id) node = id;
			for ( int j = 0; j < toursize; j += 1 ) {	// tree register
				tournament[j][node] = id;
				node >>= 1;
			} // for

			if ( FASTPATH( ! __sync_bool_compare_and_swap( &first, FREE_LOCK, id ) ) ) {
				typeof(exits) e = exits;
				await( exits - e >= 2 || first == FREE_LOCK || first == id );
				if ( FASTPATH( ! __sync_bool_compare_and_swap( &first, FREE_LOCK, id ) ) ) {
					await( *tWait );
				} // if
			} // if

			*tEnter = *tWait = false;
			exits += 1 ;
			//Fence();									// force store before more loads

			CriticalSection( id );

			// down hill
			TYPE thread = tournament[toursize - 1][0];
			if ( FASTPATH( thread != FREE_NODE && thread != id && arrState[thread].enter ) )
				Qenqueue( q, thread );

			for ( int j = toursize - 2; j >= 0; j -= 1 ) {
				node = id >> j;

				if ( node & 1 ) {
					node -= 1;
				} // if
				thread = tournament[j][node];
				if ( thread != FREE_NODE && thread != id && arrState[thread].enter ) {
					Qenqueue( q, thread );
				} // if
				thread = tournament[j][node + 1];
				if ( thread != FREE_NODE && thread != id && arrState[thread].enter ) {
					Qenqueue( q, thread );
				} // if
			} // for
			if ( FASTPATH( QnotEmpty( q ) ) ) {
				thread = Qdequeue( q );
				first = thread;
				arrState[thread].wait = true;
			} else {
				first = FREE_LOCK;
			} // if
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
	q = Qctor( N );

	arrState = malloc( N * sizeof(typeof(arrState[0])) );
	for ( int i = 0; i < N; i += 1 ) {
		arrState[i].enter = arrState[i].wait = false;
	} // for

	toursize = Clog2( N ) + 1;
	tournament = malloc( toursize * sizeof(typeof(tournament[0])) );
	int levelSize = 1 << Clog2( N );					// 2^|log N|

	for ( int i = 0; i < toursize; i += 1 ) {
		tournament[i] = malloc( levelSize * sizeof(typeof(tournament[0][0])) );
		for ( int j = 0; j < levelSize; j += 1 ) {
			tournament[i][j] = FREE_NODE;
		} // for
		levelSize /= 2;
	} // for

	first = FREE_LOCK;
	exits = 0;
} // ctor

void __attribute__((noinline)) dtor() {
	Qdtor( q );
	for ( int i = 0; i < toursize; i += 1 ) {
		free( (void *)tournament[i] );
	} // for
	free( tournament );
	free( (void *)arrState );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=RMRS Harness.c -lpthread -lm" //
// End: //
