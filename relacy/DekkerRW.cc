#include "relacy/relacy_std.hpp"
#include "Common.h"

#define await( E ) while ( ! (E) ) Pause()

enum { N = 2 };

struct Dekker : rl::test_suite<Dekker, N> {
	std::atomic<int> cc[N], last;

	rl::var<int> CS;									// shared resource for critical section

	void before() {
		cc[0]($) = cc[1]($) = 0;
	    last($) = 0;
	} // before

	void thread( int id ) {
		int other = id ^ 1;

		for ( ;; ) {
			cc[id]($) = 1;
		  if ( cc[other]($) == 0 ) break;
		  if ( last($) != id ) {
				await( cc[other]($) == 0 );
				break;
			} // if
			cc[id]($) = 0;
			await( cc[other]($) == 0 || last($) != id );
		} // for

		CS($) = id + 1;									// critical section

		if ( last($) != id ) {
			last($) = id;
		} // if
		cc[id]($) = 0;
	} // thread
}; // Dekker

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Dekker>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 DekkerRW.cc" //
// End: //
