#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

struct Taubenfeld : rl::test_suite<Taubenfeld, N> {
	std::atomic<unsigned int> intents[N][N];			// triangular matrix of intents
	std::atomic<unsigned int> turns[N][N];				// triangular matrix of turns
	int depth;

	rl::var<int> CS;									// shared resource for critical section

	void before() {
		depth = Clog2( N );								// maximal depth of binary tree
		int width = 1 << depth;							// maximal width of binary tree
		for ( int r = 0; r < depth; r += 1 ) {			// allocate matrix rows
			int size = width >> r;						// maximal row size
			for ( int c = 0; c < size; c += 1 ) {		// initial all intents to dont-want-in
				intents[r][c]($) = 0;
			} // for
		} // for
	} // before

	void thread( int id ) {
		unsigned int node = id;
		for ( int lv = 0; lv < depth; lv += 1 ) {		// entry protocol
			unsigned int lr = node & 1;					// round id for intent
			node >>= 1;									// round id for turn
			intents[lv][2 * node + lr]($) = 1;			// declare intent
			turns[lv][node]($) = lr;					// RACE
			while ( intents[lv][2 * node + (1 - lr)]($) == 1 && turns[lv][node]($) == lr ) Pause();
		} // for
		CS($) = id + 1;									// critical section
		for ( int lv = depth - 1; lv >= 0; lv -= 1 ) {	// exit protocol
			intents[lv][id / (1 << lv)]($) = 0;			// retract all intents in reverse order
		} // for
	} // thread
}; // Taubenfeld

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Taubenfeld>(p);
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Taubenfeld.cc" //
// End: //
