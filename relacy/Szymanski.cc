#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Boleslaw K. Szymanski. A simple solution to Lamport's concurrent programming problem with linear wait.
// Proceedings of the 2nd International Conference on Supercomputing, 1988, Figure 2, Page 624.
// Waiting after CS can be moved before it.

struct Szymanski : rl::test_suite<Szymanski, N> {
	std::atomic<int> flag[N];

	rl::var<int> CS;									// shared resource for critical section

#   define await( E ) while ( ! (E) ) Pause()

	void before() {
		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			flag[i]($) = 0;
		} // for
	} // before

	void thread( int id ) {
		int j;

		flag[id]($) = 1;
		for ( j = 0; j < N; j += 1 )					// wait until doors open
			await( flag[j]($) < 3 );
		flag[id]($) = 3;								// close door 1
		for ( j = 0; j < N; j += 1 )					// check for 
			if ( flag[j]($) == 1 ) {					//   others in group ?
				flag[id]($) = 2;						// enter waiting room
			  L: for ( int k = 0; k < N; k += 1 )		// wait for
					if ( flag[k]($) == 4 ) goto fini;	//   door 2 to open
				goto L;
			  fini: ;
			} // if
		flag[id]($) = 4;								// open door 2
//		for ( j = 0; j < N; j += 1 )					// wait for all threads in waiting room
//			await( flag[j]($) < 2 || flag[j]($) > 3 );	//    to pass through door 2
		for ( j = 0; j < id; j += 1 )					// service threads in priority order
			await( flag[j]($) < 2 );
		CS($) = id + 1;									// critical section
		for ( j = id + 1; j < N; j += 1 )				// wait for all threads in waiting room
			await( flag[j]($) < 2 || flag[j]($) > 3 );	//    to pass through door 2
		flag[id]($) = 0;
	} // thread
}; // Szymanski

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Szymanski>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Szymanski.cc" //
// End: //
