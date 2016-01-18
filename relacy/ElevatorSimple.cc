#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

struct ElevatorSimple : rl::test_suite<ElevatorSimple, N> {
	std::atomic<TYPE> fast, first;
	std::atomic<TYPE> apply[N];
	std::atomic<TYPE> b[N], x, y;

#   define await( E ) while ( ! (E) ) Pause()

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
		for ( unsigned int kk = 0; kk < id; kk += 1 ) {
			if ( b[kk]($) ) {
				b[id]($) = false;
				return false ;
			} // if
		} // for
		for ( unsigned int kk = id + 1; kk < N; kk += 1 ) {
			await( ! b[kk]($) );
		} // for
		bool leader = ((! fast($)) ? fast($) = true : false);
		b[id]($) = false;
		return leader;
	} // WCas

#else
    #error unsupported architecture
#endif // WCas

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
		apply[id]($) = false;

		data($) = id + 1;								// critical section

		typeof(id) kk = cycleUp( id, N );
		while ( kk != id && ! apply[kk]($) )
			kk = cycleUp( kk, N );
		first($) = kk == id ? N : kk;
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
