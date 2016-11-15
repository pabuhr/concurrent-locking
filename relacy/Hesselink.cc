#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Wim H. Hesselink, Verifying a Simplification of Mutual Exclusion by Lycklama-Hadzilacos, Acta Informatica, 2013,
// 50(3), Fig.4, p. 11

struct Hesselink : rl::test_suite<Hesselink, N> {
	static const int R = 3;
	std::atomic<int> intents[N], turn[N * R];

	rl::var<int> CS;									// shared resource for critical section

	void before() {
		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			intents[i]($) = 0;
		} // for
		for ( int i = 0; i < N * R; i += 1 ) {			// initialize shared data
			turn[i]($) = 0;
		} // for
	} // before

	void thread( int id ) {
		int Range = N * R, copy[Range];
		int j, nx = 0;

		intents[id]($) = 1;								// phase 1, FCFS
		for ( j = 0; j < Range; j += 1 )				// copy turn values
			copy[j] = turn[j]($);
		turn[id * R + nx]($) = 1;						// advance turn
		intents[id]($) = 0;
		for ( j = 0; j < Range; j += 1 )
			if ( copy[j] != 0 ) {						// want in ?
				while ( turn[j]($) != 0 ) Pause();
//				copy[j] = 0;
			} // if
	  L: intents[id]($) = 1;							// phase 2, B-L entry protocol, stage 1
		for ( j = 0; j < id; j += 1 )
			if ( intents[j]($) != 0 ) {
				intents[id]($) = 0;
				while ( intents[j]($) != 0 ) Pause();
				goto L;
			} // if
		for ( j = id + 1; j < N; j += 1 )				// B-L entry protocol, stage 2
			while ( intents[j]($) != 0 ) Pause();
		CS($) = id + 1;									// critical section
		intents[id]($) = 0;								// B-L exit protocol
		turn[id * R + nx]($) = 0;
		nx = cycleUp( nx, R );
	} // thread
}; // Hesselink

int main() {
	rl::test_params p;
	SetParms( p );
	rl::simulate<Hesselink>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Hesselink.cc" //
// End: //
