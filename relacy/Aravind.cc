#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Aravind, J. Parallel Distrib. Comput. 73 (2013), Fig. 3, p. 1033.
// Moved turn[id] = 0; after the critical section for performance reasons.

struct Aravind : rl::test_suite<Aravind, N> {
	std::atomic<int> intents[N], turn[N];

	rl::var<int> data;

	void before() {
		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			intents[i]($) = turn[i]($) = 0;
		} // for
	} // before

	void thread( int id ) {
		int copy[N];
		int j, t = 1;

		intents[id]($) = 1;								// phase 1, FCFS
		for ( j = 0; j < N; j += 1 )					// copy turn values
			copy[j] = turn[j]($);
		turn[id]($) = t;								// advance turn
		intents[id]($) = 0;
		for ( j = 0; j < N; j += 1 )
			if ( copy[j] != 0 )							// want in ?
				while ( copy[j] == turn[j]($) ) Pause();
	  L: intents[id]($) = 1;							// phase 2, B-L entry protocol, stage 1
		for ( j = 0; j < id; j += 1 )
			if ( intents[j]($) != 0 ) {
				intents[id]($) = 0;
				while ( intents[j]($) != 0 ) Pause();
				goto L;									// restart
			} // if
		for ( j = id + 1; j < N; j += 1 )				// B-L entry protocol, stage 2
			while ( intents[j]($) != 0 ) Pause();
//		turn[id]($) = 0;								// original position
		data($) = id + 1;								// critical section
		intents[id]($) = 0;								// B-L exit protocol
		turn[id]($) = 0;
		t = t < 3 ? t + 1 : 1;							// [1..3]
	} // thread
}; // Aravind

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Aravind>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Aravind.cc" //
// End: //
