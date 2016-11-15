#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Edward A. Lycklama and Vassos Hadzilacos, A First-Come-First-Served Mutual-Exclusion Algorithm with Small
// Communication Variables, TOPLAS, 13(4), 1991, Fig. 4, p. 569
// Replaced pairs of bits by the values 0, 1, 2, respectively, and cycle through these values using modulo arithmetic.

struct LycklamaBuhr : rl::test_suite<LycklamaBuhr, N> {
	std::atomic<int> c[N], v[N], intents[N], turn[N];

	rl::var<int> CS;									// shared resource for critical section

	void before() {
		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			c[i]($) = v[i]($) = intents[i]($) = turn[i]($) = 0;
		} // for
	} // before

	void thread( int id ) {
		int copy[N];
		int j;

		c[id]($) = 1;									// stage 1, establish FCFS
		for ( j = 0; j < N; j += 1 )					// copy turn values
			copy[j] = turn[j]($);
		turn[id]($) = cycleUp( turn[id]($), 3 );		// advance my turn
		v[id]($) = 1;
		c[id]($) = 0;
		for ( j = 0; j < N; j += 1 )
			while ( c[j]($) != 0 || (v[j]($) != 0 && copy[j] == turn[j]($)) ) Pause();
	  L: intents[id]($) = 1;							// B-L
		for ( j = 0; j < id; j += 1 )					// stage 2, high priority search
			if ( intents[j]($) != 0 ) {
				intents[id]($) = 0;
				while ( intents[j]($) != 0 ) Pause();
				goto L;
			} // if
		for ( j = id + 1; j < N; j += 1 )			// stage 3, low priority search
			while ( intents[j]($) != 0 ) Pause();
		CS($) = id + 1;									// critical section
		v[id]($) = intents[id]($) = 0;					// exit protocol
	} // thread
}; // LycklamaBuhr

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<LycklamaBuhr>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 LycklamaBuhr.cc" //
// End: //
