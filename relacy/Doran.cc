// R. W. Doran and L. K. Thomas, Variants of the Software Solution to Mutual Exclusion, Information Processing Letters,
// July, 1980, 10(4/5), pp. 206-208, Figure 1.

#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 2 };

struct Doran : rl::test_suite<Doran, N> {
	enum Intent { DontWantIn, WantIn };
	std::atomic<int> intents[N], last;

	rl::var<int> data;

	void before() {
		intents[0]($) = intents[1]($) = DontWantIn;
	    last($) = 0;
	} // before

	void thread( int id ) {
		int other = id ^ 1;

		intents[id]($) = WantIn;						// declare intent
		if ( intents[other]($) == WantIn ) {			// other thread want in ?
			if ( last($) == id ) {						// low priority task ?
				intents[id]($) = DontWantIn;			// retract intent
				while ( last($) == id ) Pause();		// low priority busy wait
				intents[id]($) = WantIn;				// re-declare intent
			}
			while ( intents[other]($) == WantIn ) Pause(); // high priority busy wait
		}
		data($) = id + 1;								// critical section
		last($) = id;
		intents[id]($) = DontWantIn;					// retract intent
	} // thread
}; // Doran

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Doran>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Doran.cc" //
// End: //
