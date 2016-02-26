#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

struct ElevatorSimple : rl::test_suite<ElevatorSimple, N> {
	std::atomic<TYPE> fast, first;
	std::atomic<TYPE> apply[N];
	std::atomic<TYPE> b[N], x, y;

#   define await( E ) while ( ! (E) ) Pause()

#ifdef CAS

	bool WCas( TYPE ) { TYPE comp = false; return fast.compare_exchange_strong( comp, true, std::memory_order_seq_cst ); }

#else // ! CAS

#if defined( WCas1 )

	bool WCas( TYPE id ) {
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

#elif defined( WCas2 )

	bool WCas( TYPE id ) {
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

#else
    #error unsupported architecture
#endif // WCas

#endif // CAS

	//======================================================

	rl::var<int> data;

	void before() {
		for ( TYPE id = 0; id < N; id += 1 ) {			// initialize shared data
			apply[id]($) = b[id]($) = false;
		} // for
		y($) = first($) = N;
		fast($) = false;
	} // before

	void thread( TYPE id ) {
		apply[id]($) = true;
		if ( WCas( id ) ) {
			await( first($) == N || first($) == id );
			first($) = id;
			fast($) = false;
		} else {
			await( first($) == id );
		} // if

		data($) = id + 1;								// critical section

		typeof(id) thr;
		for ( thr = cycleUp( id, N ); ! apply[thr]($); thr = cycleUp( thr, N ) );
		apply[id]($) = false;							// must appear before setting first
		first($) = thr == id ? N : thr;
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
