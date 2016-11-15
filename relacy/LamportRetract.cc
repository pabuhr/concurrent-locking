#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Leslie Lamport, The Mutual Exclusion Problem: Part II - Statement and Solutions, Journal of the ACM, 33(2), 1986,
// Fig. 1, p. 337

struct LamportRetract : rl::test_suite<LamportRetract, N> {
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

	  L: intents[id]($) = WantIn;
		for ( j = 0; j < id; j += 1 ) {				// check if thread with higher id wants in
//		for ( j = id - 1; j >= 0; j -= 1 ) {
			if ( intents[j]($) == WantIn ) {
				intents[id]($) = DontWantIn;
				while ( intents[j]($) == WantIn ) Pause();
				goto L;
			} // if
		} // for
		for ( j = id + 1; j < N; j += 1 )
			while ( intents[j]($) == WantIn ) Pause();
		CS($) = id + 1;									// critical section
		intents[id]($) = DontWantIn;					// exit protocol
	} // thread
}; // LamportRetract

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<LamportRetract>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 LamportRetract.cc" //
// End: //
