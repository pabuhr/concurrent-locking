#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Edsger W. Dijkstra, Solution of a Problem in Concurrent Programming Control, CACM, 8(9), 1965, p. 569

struct Dijkstra : rl::test_suite<Dijkstra, N> {
	std::atomic<int> b[N+1], c[N+1], turn;

	rl::var<int> data;

	void before() {
		for ( int i = 0; i <= N; i += 1 ) {				// initialize shared data
			b[i]($) = c[i]($) = true;
		} // for
		turn($) = false;
	} // before

	void thread( int id ) {
		id += 1;
		int j;

		b[id]($) = false;								// entry protocol
	  L: c[id]($) = true;
		if ( turn($) != id ) {							// maybe set and restarted
			while ( b[turn($)]($) != 1 ) Pause();		// busy wait
			turn($) = id;
		} // if
		c[id]($) = false;
		for ( j = 1; j <= N; j += 1 )
			if ( j != id && c[j]($) == 0 ) goto L;
		data($) = id + 1;								// critical section
		c[id]($) = true;								// exit protocol
		b[id]($) = true;
		turn($) = false;
	} // thread
}; // Dijkstra

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Dijkstra>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Dijkstra.cc" //
// End: //
