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

struct ElevatorTree : rl::test_suite<ElevatorTree, N> {
	Queue queue CALIGN;

	std::atomic<TYPE> fast;
#ifndef CAS
	std::atomic<TYPE> b[N], x, y;
#endif // ! CAS

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
		bool leader;
		if ( ! fast($) ) { fast($) = true; leader = true; }
		else leader = false;
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
		bool leader;
		if ( ! fast($) ) { fast($) = true; leader = true; }
		else leader = false;
		y($) = N;
		b[id]($) = false;
		return leader;
	} // WCas

#else
    #error unsupported architecture
#endif // WCas

	//======================================================

#ifdef FLAG
	typedef struct CALIGN {								// performance gain when fields juxtaposed
		std::atomic<TYPE> flag;
	} Flags;
	Flags flags[N + 1];
	std::atomic<TYPE> val[2 * N];
#else
	std::atomic<TYPE> first;
#endif // FLAG

	typedef struct CALIGN {
		std::atomic<TYPE> val;
	} Vals;
	Vals vals[2 * N + 1];

	void before() {
		Qctor( &queue );
#ifdef FLAG
		for ( typeof(N) id = 0; id <= N; id += 1 ) {			// initialize shared data
			flags[id].flag($) = false;
		} // for
#endif // FLAG

#ifdef FLAG
		flags[N].flag($) = true;
#else
		first($) = N;
#endif // FLAG

		for ( typeof(N) id = 0; id <= 2 * N; id += 1 ) { // initialize shared data
			vals[id].val($) = N;
		} // for

#ifndef CAS
		for ( typeof(N) id = 0; id < N; id += 1 ) {		// initialize shared data
			b[id]($) = false;
		} // for
		y($) = N;
#endif // CAS
		fast($) = false;
	} // before

	rl::var<int> CS;									// shared resource for critical section

	void thread( TYPE id ) {
		const unsigned int n = N + id, dep = Log2( n );
		typeof(vals[0].val) *valId = &vals[n].val;
#ifdef FLAG
		typeof(flags[0].flag) *flagId = &flags[id].flag;
		typeof(flags[0].flag) *flagN = &flags[N].flag;
#endif // FLAG

		// loop goes from leaf to child of root
		for ( unsigned int j = n; j > 1; j >>= 1 )		// entry protocol
			vals[j].val($) = id;

		if ( WCas( id ) ) {								// true => leader
#ifdef FLAG
			await( (*flagId)($) || (*flagN)($) );
			(*flagN)($) = false;
#else
			await( first($) == id || first($) == N );
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
		(*valId)($) = N;
#ifdef FLAG
		(*flagId)($) = false;
#endif // FLAG

		CS($) = id + 1;									// critical section

		// loop goes from child of root to leaf and inspects siblings
		for ( int j = dep - 1; j >= 0; j -= 1 ) {		// must be "signed"
			TYPE k = vals[(n >> j) ^ 1].val($);
			std::atomic<TYPE> *wid = &(vals[N + k].val);
			if ( (*wid)($) < N ) {
				(*wid)($) = N;
				Qenqueue( &queue, k );
			} // if
		} // for
		if ( QnotEmpty( &queue ) )
#ifdef FLAG
			flags[Qdequeue( &queue )].flag($) = true;
		else
			(*flagN)($) = true;
#else
			first($) = Qdequeue( &queue );
		else
			first($) = N;
#endif // FLAG
	} // thread
}; // ElevatorTree

int main() {
	rl::test_params p;
	SetParms( p );
	rl::simulate<ElevatorTree>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -DCAS -I/u/pabuhr/software/relacy_2_4 ElevatorTree.cc" //
// End: //
