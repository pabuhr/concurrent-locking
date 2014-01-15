#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 2 };

// Peter A. Buhr and Ashif S. Harji, Concurrent Urban Legends, Concurrency and Computation: Practice and Experience,
// 2005, 17(9), Figure 3, p. 1151

struct Dekker : rl::test_suite<Dekker, N> {
	enum Intent { DontWantIn, WantIn };
	std::atomic<int> intents[N], last;

	rl::var<int> data;

	void before() {
		intents[0]($) = intents[1]($) = DontWantIn;
	    last($) = 0;
	} // before

	void thread( int id ) {
		int other = 1 - id;

		intents[id]($) = WantIn;						// declare intent
		while ( intents[other]($) == WantIn ) {
			if ( last($) == id ) {
				intents[id]($) = DontWantIn;
				while ( last($) == id ) Pause();		// low priority busy wait
				intents[id]($) = WantIn;				// declare intent
			} else
				Pause();
		}
		data($) = id + 1;								// critical section
		last($) = id;
		intents[id]($) = DontWantIn;					// retract intent
	} // thread
}; // Dekker

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Dekker>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Dekker.cc" //
// End: //
