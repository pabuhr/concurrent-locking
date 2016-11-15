#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

//======================================================

typedef TYPE QElem_t;

typedef struct CALIGN {
	QElem_t elements[N];
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
} // Qctor

//======================================================

struct ElevatorQueue : rl::test_suite<ElevatorQueue, N> {
	Queue queue CALIGN;

	typedef struct CALIGN {								// performance gain when fields juxtaposed
		std::atomic<TYPE> apply;
#ifdef FLAG
		std::atomic<TYPE> flag;
#endif // FLAG
	} Tstate;

	std::atomic<TYPE> fast;
	Tstate tstate[N + 1];
	std::atomic<TYPE> val[2 * N];

#ifndef CAS
	std::atomic<TYPE> b[N], x, y;
#endif // ! CAS

#ifndef FLAG
	std::atomic<TYPE> first;
#endif // FLAG

#   define await( E ) while ( ! (E) ) Pause()

#ifdef CAS

	bool WCas( TYPE ) { TYPE comp = false; return fast.compare_exchange_strong( comp, true, std::memory_order_seq_cst ); }

#elif defined( WCasBL )

	bool WCas( TYPE id ) {								// based on Burns-Lamport algorithm
		b[id]($) = true;
		for ( typeof(id) thr = 0; thr < id; thr += 1 ) {
			if ( b[thr]($) ) {
				b[id]($) = false;
				return false ;
			} // if
		} // for
		for ( typeof(id) thr = id + 1; thr < N; thr += 1 ) {
			await( ! b[thr]($) );
		} // for
		bool leader = ((! fast($)) ? fast($) = true : false);
		b[id]($) = false;
		return leader;
	} // WCas

#elif defined( WCasLF )

	bool WCas( TYPE id ) {								// based on Lamport-Fast algorithm
		b[id]($) = true;
		x($) = id;
		if ( y($) != N ) {
			b[id]($) = false;
			return false;
		} // if
		y($) = id;
		if ( x($) != id ) {
			b[id]($) = false;
			for ( int j = 0; j < N; j += 1 )
				await( ! b[j]($) );
			if ( y($) != id ) return false;
		} // if
		bool leader = ((! fast($)) ? fast($) = true : false);
		y($) = N;
		b[id]($) = false;
		return leader;
	} // WCas

#else
    #error unsupported architecture
#endif // WCas

	//======================================================

	void before() {
		Qctor( &queue );
		for ( TYPE id = 0; id <= N; id += 1 ) {			// initialize shared data
			tstate[id].apply($) = false;
#ifdef FLAG
			tstate[id].flag($) = false;
#endif // FLAG
		} // for

#ifdef FLAG
		tstate[N].flag($) = true;
#else
		first($) = N;
#endif // FLAG

		for ( TYPE id = 0; id < N; id += 1 ) {			// initialize shared data
			val[id]($) = N;
			val[N + id]($) = id;
		} // for

#ifndef CAS
		for ( TYPE id = 0; id < N; id += 1 ) {			// initialize shared data
			b[id]($) = false;
		} // for
		y($) = N;
#endif // CAS
		fast($) = false;
	} // before

	rl::var<int> CS;									// shared resource for critical section

	void thread( TYPE id ) {
		const unsigned int n = N + id, dep = Log2( n );
		typeof(tstate[0].apply) *applyId = &tstate[id].apply;
#ifdef FLAG
		typeof(tstate[0].flag) *flagId = &tstate[id].flag;
		typeof(tstate[0].flag) *flagN = &tstate[N].flag;
#endif // FLAG

		// loop goes from parent of leaf to child of root
		(*applyId)($) = true;
		for ( unsigned int j = (n >> 1); j > 1; j >>= 1 )
			val[j]($) = id;

		if ( WCas( id ) ) {
#ifdef FLAG
			await( (*flagN)($) || (*flagId)($) );
			(*flagN)($) = false;
#else
			await( first($) == N || first($) == id );
			first($) = id;
#endif // FLAG
			fast($) = false;
		} else {
#ifdef FLAG
			await( (*flagId)($) );
#else
			await( first($) == id );
#endif // FLAG
		} // if
#ifdef FLAG
		(*flagId)($) = false;
#endif // FLAG
		(*applyId)($) = false;

		CS($) = id + 1;									// critical section

		// loop goes from child of root to leaf and inspects siblings
		for ( int j = dep - 1; j >= 0; j -= 1 ) {		// must be "signed"
			TYPE k = val[(n >> j) ^ 1]($);
			if ( tstate[k].apply($) ) {
				tstate[k].apply($) = false;
				Qenqueue( &queue, k );
			} // if
		}  // for
		if ( QnotEmpty( &queue ) )
#ifdef FLAG
			tstate[Qdequeue( &queue )].flag($) = true; else (*flagN)($) = true;
#else
		first($) = Qdequeue( &queue ); else first($) = N;
#endif // FLAG
	} // thread
}; // ElevatorQueue

int main() {
	rl::test_params p;
	SetParms( p );
	rl::simulate<ElevatorQueue>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -DCAS -I/u/pabuhr/software/relacy_2_4 ElevatorQueue.cc" //
// End: //
