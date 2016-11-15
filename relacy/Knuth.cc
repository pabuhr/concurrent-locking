#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Donald E. Knuth, Additional Comments on a Problem in Concurrent Programming Control, CACM, 1966, 9(5), p. 321,
// Letter to the Editor

struct Knuth : rl::test_suite<Knuth, N> {
	enum Intent { DontWantIn, WantIn, EnterCS };
	std::atomic<int> control[N], turn;

	rl::var<int> CS;									// shared resource for critical section

	void before() {
		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			control[i]($) = DontWantIn;
		} // for
		turn($) = 0;
	} // before

	void thread( int id ) {
		int j;

	  L0: control[id]($) = WantIn;						// entry protocol
	  L1: for ( j = turn($); j != id; j = cycleDown( j, N ) )
			if ( control[j]($) != DontWantIn ) { Pause(); goto L1; } // restart search
		control[id]($) = EnterCS;
		for ( j = N - 1; j >= 0; j -= 1 )
			if ( j != id && control[j]($) == EnterCS ) { Pause(); goto L0; }
//		turn($) = id;
		CS($) = id + 1;									// critical section
		// cycle through threads
		turn($) = cycleDown( id, N );					// exit protocol
		control[id]($) = DontWantIn;
	} // thread
}; // Knuth

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Knuth>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Knuth.cc" //
// End: //
