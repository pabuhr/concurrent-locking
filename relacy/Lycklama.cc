#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Edward A. Lycklama and Vassos Hadzilacos, A First-Come-First-Served Mutual-Exclusion Algorithm with Small
// Communication Variables, TOPLAS, 13(4), 1991, Fig. 4, p. 569

struct Lycklama : rl::test_suite<Lycklama, N> {
	std::atomic<int> c[N], v[N], intents[N], turn[N][2];

	rl::var<int> data;

	void before() {
		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			c[i]($) = v[i]($) = intents[i]($) = 0;
			turn[i][0]($) = turn[i][1]($) = 0;
		} // for
	} // before

	void thread( int id ) {
		int copy[N][2];
		int j, bit = 0;

		c[id]($) = 1;										// stage 1, establish FCFS
		for ( j = 0; j < N; j += 1 ) {					// copy turn values
			copy[j][0] = turn[j][0]($);
			copy[j][1] = turn[j][1]($);
		} // for
		bit = 1 - bit;
		turn[id][bit]($) = 1 - turn[id][bit]($);			// advance my turn
		v[id]($) = 1;
		c[id]($) = 0;
		for ( j = 0; j < N; j += 1 )
			while ( c[j]($) != 0 || (v[j]($) != 0 && copy[j][0] == turn[j][0]($) && copy[j][1] == turn[j][1]($))) Pause();
	  L: intents[id]($) = 1;							// B-L
		for ( j = 0; j < id; j += 1 )				// stage 2, high priority search
			if ( intents[j]($) != 0 ) {
				intents[id]($) = 0;
				while ( intents[j]($) != 0 ) Pause();
				goto L;
			} // if
		for ( j = id + 1; j < N; j += 1 )			// stage 3, low priority search
			while ( intents[j]($) != 0 ) Pause();
		data($) = id + 1;								// critical section
		v[id]($) = intents[id]($) = 0;					// exit protocol
	} // thread
}; // Lycklama

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Lycklama>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Lycklama.cc" //
// End: //
