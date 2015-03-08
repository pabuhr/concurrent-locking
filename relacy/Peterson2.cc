#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 2 };

// G. L. Peterson, Myths About the Mutual Exclusion Problem, Information Processing Letters, 1981, 12(3), Fig. 1, p. 115
// Separate code for each thread is unified using an array.

#define inv( c ) ((c) ^ 1)

struct Peterson2 : rl::test_suite<Peterson2, N> {
	enum Intent { DontWantIn, WantIn };
	std::atomic<int> intents[N];
	std::atomic<int> last;

	rl::var<int> data;

	void before() {
		intents[0]($) = intents[1]($) = DontWantIn;
	    last($) = 0;
	} // before

	void thread( int id ) {
		int other = inv( id );

		intents[id]($) = WantIn;						// declare intent
		last($).store(id);								// write race
		while ( intents[other]($) == WantIn && last($) == id ) Pause();
		data($) = id + 1;								// critical section
		intents[id]($) = DontWantIn;					// retract intent
	} // thread
}; // Peterson2

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Peterson2>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Peterson2.cc" //
// End: //
