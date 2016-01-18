#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

struct ElevatorBurns : rl::test_suite<ElevatorBurns, N> {
	std::atomic<TYPE> first;
	std::atomic<TYPE> apply[N];
	std::atomic<TYPE> b[N];

#   define await( E ) while ( ! (E) ) Pause()

	void mutexB( TYPE id ) {
		b[id]($) = true;
		for ( TYPE kk = 0; kk < id; kk += 1 ) {
			if ( b[kk]($) || first($) == id ) {
				b[id]($) = false;
				await( first($) == id );
				return;
			} // if
		} // for
		for ( int kk = id + 1; kk < N; kk += 1 ) {
			await( ! b[kk]($) || first($) == id );
		  if ( first($) == id ) break;
		} // for
		await( first($) == id || first($) == N );
		first($) = id;
		b[id]($) = false;
	} // mutexB

	//======================================================

	rl::var<int> data;

	void before() {
		for ( TYPE id = 0; id < N; id += 1 ) {			// initialize shared data
			apply[id]($) = b[id]($) = false;
		} // for
		first($) = N;
	} // before

	void thread( TYPE id ) {
		apply[id]($) = true;
		mutexB( id );
		apply[id]($) = false;

		data($) = id + 1;								// critical section

		typeof(id) kk = cycleUp( id, N );
		while ( kk != id && ! apply[kk]($) )
			kk = cycleUp( kk, N );
		first($) = kk == id ? N : kk;
	} // thread
}; // ElevatorBurns

int main() {
	rl::test_params p;
	SetParms( p );
	rl::simulate<ElevatorBurns>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 ElevatorBurns.cc" //
// End: //
