#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// James E. Burns and Nancy A. Lynch, {Mutual Exclusion using Indivisible Reads and Writes, Proceedings of the 18th
// Annual Allerton Conference on Communications, Control and Computing, 1980, p. 836

struct BurnsLynchRetract : rl::test_suite<BurnsLynchRetract, N> {
	enum Intent { DontWantIn, WantIn };
	std::atomic<int> intents[N];

	rl::var<int> CS;									// shared resource for critical section

	void before() {
		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			intents[i]($) = DontWantIn;
		} // for
	} // before

	void thread( int id ) {
		int j;

	  L0: intents[id]($) = DontWantIn;					// entry protocol
		for ( j = 0; j < id; j += 1 )
			if ( intents[j]($) == WantIn ) { Pause(); goto L0; }
		intents[id]($) = WantIn;
		for ( j = 0; j < id; j += 1 )
			if ( intents[j]($) == WantIn ) goto L0;
	  L1: for ( j = id + 1; j < N; j += 1 )
			if ( intents[j]($) == WantIn ) { Pause(); goto L1; }
		CS($) = id + 1;									// critical section
		intents[id]($) = DontWantIn;					// exit protocol
	} // thread
}; // BurnsLynchRetract

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<BurnsLynchRetract>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 BurnsLynchRetract.cc" //
// End: //
