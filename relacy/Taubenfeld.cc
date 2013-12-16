#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

struct Taubenfeld : rl::test_suite<Taubenfeld, N> {
	std::atomic<int> intents[N][N];						// triangular matrix of intents
	std::atomic<int> turns[N][N];						// triangular matrix of turns
	int depth;

	rl::var<int> data;

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
		int node, lr;

		node = id;
		for ( int l = 0; l < depth; l += 1 ) {			// entry protocol
			lr = node & 1;								// round id for intent
			node = node >> 1;							// round id for turn
			intents[l][2 * node + lr]($) = 1;			// declare intent
			turns[l][node]($) = lr;						// RACE
			while ( ! ( intents[l][2 * node + (1 - lr)]($) == 0 || turns[l][node]($) == 1 - lr ) ) Pause();
		} // for
		data($) = id + 1;								// critical section
		for ( int l = depth - 1; l >= 0; l -= 1 ) {		// exit protocol
			intents[l][id / (1 << l)]($) = 0;			// retract all intents in reverse order
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
