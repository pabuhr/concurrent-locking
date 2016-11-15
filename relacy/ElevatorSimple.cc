#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

struct ElevatorSimple : rl::test_suite<ElevatorSimple, N> {
	typedef struct CALIGN {								// performance gain when fields juxtaposed
		std::atomic<TYPE> apply;
#ifdef FLAG
		std::atomic<TYPE> flag;
#endif // FLAG
	} Tstate;

	std::atomic<TYPE> fast;
	Tstate tstate[N + 1];

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
		for ( TYPE id = 0; id <= N; id += 1 ) {				// initialize shared data
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

#ifndef CAS
		for ( TYPE id = 0; id < N; id += 1 ) {				// initialize shared data
			b[id]($) = false;
		} // for
		y($) = N;
#endif // CAS
		fast($) = false;
	} // before

	rl::var<int> CS;									// shared resource for critical section

	void thread( TYPE id ) {
		typeof(tstate[0].apply) *applyId = &tstate[id].apply;
#ifdef FLAG
		typeof(tstate[0].flag) *flagId = &tstate[id].flag;
		typeof(tstate[0].flag) *flagN = &tstate[N].flag;
#endif // FLAG

		(*applyId)($) = true;
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

		CS($) = id + 1;									// critical section

		typeof(id) thr;
		for ( thr = cycleUp( id, N ); ! tstate[thr].apply($); thr = cycleUp( thr, N ) );
		(*applyId)($) = false;							// must appear before setting first
		if ( thr != id )
#ifdef FLAG
			tstate[thr].flag($) = true; else (*flagN)($) = true;
#else
			first($) = thr; else first($) = N;
#endif // FLAG
	} // thread
}; // ElevatorSimple

int main() {
	rl::test_params p;
	SetParms( p );
	rl::simulate<ElevatorSimple>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 ElevatorSimple.cc" //
// End: //
