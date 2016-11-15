#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 2 };

// Peter A. Buhr and Ashif S. Harji, Concurrent Urban Legends, Concurrency and Computation: Practice and Experience,
// 2005, 17(9), Figure 3, p. 1151

struct DekkerOrig : rl::test_suite<DekkerOrig, N> {
	enum Intent { DontWantIn, WantIn };
	std::atomic<int> cc[N], turn;

	rl::var<int> CS;									// shared resource for critical section

	void before() {
	    cc[0]($) = cc[1]($) = DontWantIn;
	    turn($) = 0;
	} // before

	void thread( int id ) {
	    int other = id ^ 1;

	  A1: cc[id]($) = WantIn;							// declare intent
	  L1: if ( cc[other]($) == WantIn ) {
			if ( turn($) != id ) { Pause(); goto L1; }
			cc[id]($) = DontWantIn;
		  B1: if ( turn($) == id ) goto B1;
			goto A1;
		}
		CS($) = id + 1;									// critical section
		turn($) = id;
		cc[id]($) = DontWantIn;							// retract intent
	} // thread
}; // DekkerOrig

int main() {
    rl::test_params p;
    SetParms( p );
    rl::simulate<DekkerOrig>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 DekkerOrig.cc" //
// End: //
