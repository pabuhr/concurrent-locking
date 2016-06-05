#include "relacy/relacy_std.hpp"

#define CACHE_ALIGN 64
#define CALIGN __attribute__(( aligned (CACHE_ALIGN) ))
typedef uintptr_t TYPE;					// atomically addressable word-size
typedef volatile TYPE ATYPE;				// atomic shared data
//typedef int TYPE;

#define Pause() rl::yield(1, $)

// O(1) polymorphic integer log2, using clz, which returns the number of leading 0-bits, starting at the most
// significant bit (single instruction on x86)
#define Log2( n ) ( sizeof(n) * __CHAR_BIT__ - 1 - (			\
				  ( sizeof(n) ==  4 ) ? __builtin_clz( n ) :	\
				  ( sizeof(n) ==  8 ) ? __builtin_clzl( n ) :	\
				  ( sizeof(n) == 16 ) ? __builtin_clzll( n ) :	\
				  -1 ) )

static inline int Clog2( int n ) {			// integer ceil( log2( n ) )
	if ( n <= 0 ) return -1;
	int ln = Log2( n );
	return ln + ( (n - (1 << ln)) != 0 );		// check for any 1 bits to the right of the most significant bit
}

static inline TYPE cycleUp( TYPE v, TYPE n ) { return ( ((v) >= (n - 1)) ? 0 : (v + 1) ); }
static inline TYPE cycleDown( TYPE v, TYPE n ) { return ( ((v) <= 0) ? (n - 1) : (v - 1) ); }

static void SetParms( rl::test_params &p ) {
	//p.search_type = rl::fair_context_bound_scheduler_type;
	//p.search_type = rl::sched_full;
	//p.execution_depth_limit = 100;
	//p.initial_state = "280572";
	//p.search_type = rl::sched_bound;
	//p.context_bound = 3;
	p.search_type = rl::sched_random;
	p.iteration_count = 5000000;
} // SetParms

//#include "Common.h"

enum { N = 8 };

//======================================================

#define FREE_NODE N
#define FREE_LOCK N

typedef TYPE QElem_t;

typedef struct CALIGN {
	QElem_t elements[N + 1];
	unsigned int inQueue[N];
	unsigned int front, rear;
	unsigned int capacity;
} Queue;

static int toursize CALIGN;

static inline bool QnotEmpty( volatile Queue *queue ) {
	return queue->front != queue->rear;
} // QnotEmpty

static inline void Qenqueue( volatile Queue *queue, QElem_t element ) {
	if ( element != FREE_NODE && queue->inQueue[element] == false ) {
		queue->elements[queue->rear] = element;
		queue->rear = cycleUp( queue->rear, queue->capacity );
		queue->inQueue[element] = true;
	} // if
} // Qenqueue

static inline QElem_t Qdequeue( volatile Queue *queue ) {
	QElem_t element = queue->elements[queue->front];
	queue->front = cycleUp( queue->front, queue->capacity );
	queue->inQueue[element] = false;
	return element;
} // Qdequeue

static inline void Qctor( volatile Queue *queue, unsigned int maxElements ) {
	queue->capacity = maxElements + 1;
	queue->front = queue->rear = 0;

	for ( unsigned int i = 0; i < maxElements; i += 1 ) {
		queue->inQueue[i] = false;
	} // for
} // Qctor

//======================================================

struct RMRS : rl::test_suite<RMRS, N> {
	typedef struct CALIGN {
		std::atomic<TYPE> enter;
		std::atomic<TYPE> wait;
	} Tstate;

	Queue queue CALIGN;
	std::atomic<TYPE> tournament[N][N];
	Tstate arrState[N + 1];
	std::atomic<TYPE> first;
	std::atomic<TYPE> exits;

#	define await( E ) while ( ! (E) ) Pause()

	//======================================================

	void before() {
		Qctor( &queue, N );

		for ( int i = 0; i < N; i += 1 ) {
			arrState[i].enter($) = arrState[i].wait($) = false;
		} // for

		toursize = Clog2( N ) + 1;
		int levelSize = 1 << (toursize - 1);			// 2^|log N|

		for ( int i = 0; i < toursize; i += 1 ) {
			for ( int j = 0; j < levelSize; j += 1 ) {
				tournament[i][j]($) = FREE_NODE;
			} // for
			levelSize /= 2;
		} // for

		first($) = FREE_LOCK;
		exits($) = 0;
	} // before

	rl::var<int> data;

	void thread( TYPE id ) {
		typeof(arrState[0].enter) *enter = &arrState[id].enter;
		typeof(arrState[0].wait) *wait = &arrState[id].wait;

		//(*wait)($) = true;
		(*enter)($) = false;

		// up hill
		typeof(id) node = id;
		for ( int j = 0; j < toursize; j += 1 ) {		// tree register
			tournament[j][node]($) = id;
			node >>= 1;
		} // for

		TYPE comp = FREE_LOCK;
		if ( ! first.compare_exchange_strong( comp, id, std::memory_order_seq_cst ) ) {
			TYPE e = exits($);
			await( exits($) - e >= 2 || first($) == FREE_LOCK || first($) == id );
			comp = FREE_LOCK;
			if ( ! first.compare_exchange_strong( comp, id, std::memory_order_seq_cst ) ) {
				await( (*wait)($) );
			} // if
		} // if

		(*enter)($) = (*wait)($) = true;
		exits($) += 1 ;

		data($) = id + 1;								// critical section

		// down hill
		TYPE thread = tournament[toursize - 1][0]($);
		if ( thread != FREE_NODE && thread != id && arrState[thread].enter($) )
			Qenqueue( &queue, thread );

		for ( int j = toursize - 2; j >= 0; j -= 1 ) {
			node = id >> j;

			if ( node & 1 ) {
				node -= 1;
			} // if
			thread = tournament[j][node]($);
			if ( thread != FREE_NODE && thread != id && arrState[thread].enter($) ) {
				Qenqueue( &queue, thread );
			} // if
			thread = tournament[j][node + 1]($);
			if ( thread != FREE_NODE && thread != id && arrState[thread].enter($) ) {
				Qenqueue( &queue, thread );
			} // if
		} // for
		if ( QnotEmpty( &queue ) ) {
			thread = Qdequeue( &queue );
			first($) = thread;
			arrState[thread].wait($) = false;
		} else {
			first($) = FREE_LOCK;
		} // if
	} // thread
}; // RMRS

int main() {
	rl::test_params p;
	SetParms( p );
	rl::simulate<RMRS>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 RMRS.cc" //
// End: //
