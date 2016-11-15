#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Eric C. R. Hehner and R. K. Shyamasundar, An Implementation of P and V, Information Processing Letters, 1981, 12(4),
// pp. 196-197

struct Hehner : rl::test_suite<Hehner, N> {
	std::atomic<int> ticket[N];

	rl::var<int> CS;									// shared resource for critical section

	void before() {
		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			ticket[i]($) = 0;
		} // for
	} // before

	void thread( int id ) {
		int max, v;
		int j;

		// step 1, select a ticket
		ticket[id]($) = 0;								// set highest priority
		max = 0;										// O(N) search for largest ticket
		for ( j = 0; j < N; j += 1 ) {
			v = ticket[j]($);							// could change so copy
			if ( v != INT_MAX && max < v ) max = v;
		} // for
		max += 1;										// advance ticket
		ticket[id]($) = max;
		// step 2, wait for ticket to be selected
		for ( j = 0; j < N; j += 1 )					// check other tickets
			while ( ticket[j]($) < max ||				// busy wait if choosing or
					( ticket[j]($) == max && j < id ) ) Pause(); //  greater ticket value or lower priority
		CS($) = id + 1;									// critical section
		ticket[id]($) = INT_MAX;						// exit protocol
	} // thread
}; // Hehner

int main() {
	rl::test_params p;
	SetParms( p );
	rl::simulate<Hehner>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Hehner.cc" //
// End: //
