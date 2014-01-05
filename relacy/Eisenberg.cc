#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Murray A. Eisenberg and Michael R. McGuire}, Further Comments on Dijkstra's Concurrent Programming Control Problem,
// CACM, 1972, 15(11), p. 999

struct Eisenberg : rl::test_suite<Eisenberg, N> {
	enum Intent { DontWantIn, WantIn, EnterCS };
	std::atomic<int> control[N], HIGH;

	rl::var<int> data;

	void before() {
		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			control[i]($) = DontWantIn;
		} // for
		HIGH($) = 0;
	} // before

	void thread( int id ) {
		int j;

	  L0: control[id]($) = WantIn;						// entry protocol
		// step 1, wait for threads with higher priority
	  L1: for ( j = HIGH($); j != id; j = cycleUp( j, N ) )
			if ( control[j]($) != DontWantIn ) { Pause(); goto L1; } // restart search
		control[id]($) = EnterCS;
		// step 2, check for any other thread finished step 1
		for ( j = 0; j < N; j += 1 )
			if ( j != id && control[j]($) == EnterCS ) goto L0;
		if ( control[HIGH($)]($) != DontWantIn && HIGH($) != id ) goto L0;
		HIGH($) = id;									// its now ok to enter
		data($) = id + 1;								// critical section
		// look for any thread that wants in other than this thread
//		for ( j = cycleUp( id + 1, N );; j = cycleUp( j, N ) ) // exit protocol
		for ( j = cycleUp( HIGH($) + 1, N );; j = cycleUp( j, N ) ) // exit protocol
			if ( control[j]($) != DontWantIn ) { HIGH($) = j; break; }
		control[id]($) = DontWantIn;
	} // thread
}; // Eisenberg

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Eisenberg>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Eisenberg.cc" //
// End: //
