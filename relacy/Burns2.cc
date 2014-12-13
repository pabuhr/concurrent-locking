#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// James E. Burns, Symmetry in Systems of Asynchronous Processes. 22nd Annual
// Symposium on Foundations of Computer Science, 1981, Figure 2, p 170.

struct Burns2 : rl::test_suite<Burns2, N> {
	std::atomic<int> flag[N], turn;

	rl::var<int> data;

	void before() {
		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			flag[i]($) = false;
		} // for
		turn($) = 0;
	} // before

	void thread( int id ) {
		int j;

	  L0: flag[id]($) = true;							// entry protocol
		turn($) = id;									// RACE
	  L1: if ( turn($) != id ) {
			flag[id]($) = false;
		  L11: for ( j = 0; j < N; j += 1 )
				if ( j != id && flag[j]($) != 0 ) { Pause(); goto L11; }
			goto L0;
		} else {
//			flag[id]($) = true;
		  L2: if ( turn($) != id ) goto L1;
			for ( j = 0; j < N; j += 1 )
				if ( j != id && flag[j]($) != 0 ) { Pause(); goto L2; }
		} // if
		data($) = id + 1;								// critical section
		flag[id]($) = false;							// exit protocol
	} // thread
}; // Burns2

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Burns2>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Burns2.cc" //
// End: //
