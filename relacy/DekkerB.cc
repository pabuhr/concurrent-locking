#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 2 };

struct Dekker : rl::test_suite<Dekker, N> {
	enum Intent { DontWantIn, WantIn };
	std::atomic<int> intents[N], last;

	rl::var<int> CS;									// shared resource for critical section

	void before() {
		intents[0]($) = intents[1]($) = DontWantIn;
	    last($) = 0;
	} // before

	void thread( int id ) {
		int other = id ^ 1;

		for ( ;; ) {
			intents[id]($) = WantIn;					// declare intent
		  if ( intents[other]($) == DontWantIn ) break;
		  if ( last($) == id ) {
			  while ( intents[other]($) == WantIn ) Pause(); // low priority busy wait
				break;
			} // if
			intents[id]($) = DontWantIn;
			while ( last($) == id ) Pause();			// low priority busy wait
			intents[id]($) = WantIn;					// declare intent
		} // for
		CS($) = id + 1;									// critical section
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
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 DekkerB.cc" //
// End: //
