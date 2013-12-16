#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Nicolaas Govert de Bruijn. Additional Comments on a Problem in Concurrent Programming Control, CACM, Mar 1967,
// 10(3):137. Letter to the Editor.

struct DeBruijn : rl::test_suite<DeBruijn, N> {
	enum Intent { DontWantIn, WantIn, EnterCS };
	std::atomic<int> control[N], turn;

	rl::var<int> data;

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
		data($) = id + 1;								// critical section
		// cycle through threads
		if ( control[turn($)]($) == DontWantIn || turn($) == id ) // exit protocol
			turn($) = cycleDown( turn($), N );
		control[id]($) = DontWantIn;
	} // thread
}; // DeBruijn

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<DeBruijn>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 DeBruijn.cc" //
// End: //
