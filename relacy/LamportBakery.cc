#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Leslie Lamport, A New Solution of Dijkstra's Concurrent Programming Problem, CACM, 1974, 17(8), p. 454

struct LamportBakery : rl::test_suite<LamportBakery, N> {
	std::atomic<TYPE> choosing[N], ticket[N];

	rl::var<int> data;

	void before() {
		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			choosing[i]($) = ticket[i]($) = 0;
		} // for
	} // before

	void thread( TYPE id ) {
		TYPE max, v;
		TYPE j;

		choosing[id]($) = 1;							// entry protocol
		max = 0;										// O(N) search for largest ticket
		for ( j = 0; j < N; j += 1 ) {
			v = ticket[j]($);							// could change so copy
			if ( max < v ) max = v;
		} // for
		max += 1;										// advance ticket
		ticket[id]($) = max;
		choosing[id]($) = 0;
		// step 2, wait for ticket to be selected
		for ( j = 0; j < N; j += 1 ) {					// check other tickets
			while ( choosing[j]($) == 1 ) Pause();		// busy wait if thread selecting ticket
			while ( ticket[j]($) != 0 &&				// busy wait if choosing or
					( ticket[j]($) < max ||				//  greater ticket value or lower priority
					  ( ticket[j]($) == max && j < id ) ) ) Pause();
		} // for
		data($) = id + 1;								// critical section
		ticket[id]($) = 0;								// exit protocol
	} // thread
}; // LamportBakery

int main() {
	rl::test_params p;
	SetParms( p );
	rl::simulate<LamportBakery>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 LamportBakery.cc" //
// End: //
