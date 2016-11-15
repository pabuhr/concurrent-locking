#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Nancy A. Lynch, Distributed Algorithms, Morgan Kaufmann, 1996, Section 10.5.3
// Significant parts of this algorithm are written in prose, and therefore, left to our interpretation with respect to implementation.

static inline int min( int a, int b ) { return a < b ? a : b; }

struct Lynch : rl::test_suite<Lynch, N> {
	int depth, width, mask;
	std::atomic<int> intents[N], turns[N];				// maximal width of binary tree

	rl::var<int> CS;									// shared resource for critical section

	void before() {
		depth = Log2( N );
		width = 1 << depth;
		mask = width - 1;
		for ( int i = 0; i < N; i += 1 ) {			// initialize shared data
			intents[i]($) = 0;
		} // for
	} // before

	void thread( int id ) {
		int lid, comp, role, low, high;

		for ( int km1 = 0, k = 1; k <= depth; km1 += 1, k += 1 ) { // entry protocol
			lid = id >> km1;							// local id
			comp = (lid >> 1) + (width >> k);			// unique position in the tree
			role = lid & 1;								// left or right descendent
			intents[id]($) = k;							// declare intent, current round
			turns[comp]($) = role;						// RACE
			low = ((lid) ^ 1) << km1;					// lower competition
			high = min( low | mask >> (depth - km1), N - 1 ); // higher competition
			for ( int i = low; i <= high; i += 1 )	// busy wait
				while ( intents[i]($) >= k && turns[comp]($) == role ) Pause();
		} // for
		CS($) = id + 1;									// critical section
		intents[id]($) = 0;								// exit protocol
	} // thread
}; // Lynch

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Lynch>(p);
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Lynch.cc" //
// End: //
