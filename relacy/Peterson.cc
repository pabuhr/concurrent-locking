#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// G. L. Peterson, Myths About the Mutual Exclusion Problem, Information Processing Letters, 1981, 12(3), Fig. 3, p. 116

struct Peterson : rl::test_suite<Peterson, N> {
	std::atomic<int> Q[N+1], turns[N];

	rl::var<int> data;

	void before() {
	    for ( int i = 0; i <= N; i += 1 ) {				// initialize shared data
			Q[i]($) = 0;
	    } // for
	} // before

	void thread( int id ) {
		id += 1;
		for ( int rd = 1; rd < N; rd += 1 ) {			// entry protocol, round
			Q[id]($) = rd;								// current round
			turns[rd]($) = id;							// MULTI-WAY RACE
		  L: for ( int k = 1; k <= N; k += 1 )			// find loser
				if ( k != id && Q[k]($) >= rd && turns[rd]($) == id ) { Pause(); goto L; }
		} // for
		data($) = id + 1;								// critical section
		Q[id]($) = 0;									// exit protocol
	} // thread
}; // Peterson

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Peterson>(p);
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Peterson.cc" //
// End: //
