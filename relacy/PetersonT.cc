#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Gary L. Peterson and Michael J. Fischer, Economical Solutions for the Critical Section Problem in a Distributed
// System (Extended Abstract), Proceedings of the Ninth Annual ACM Symposium on Theory of Computing, STOC '77, 1977, p. 93

#include <stdint.h>										// uint*

struct PetersonT : rl::test_suite<PetersonT, N> {
	union Tuple {
		struct {
			uint16_t level;								// current level in tournament
			uint16_t state;								// intent to enter critical section
		} tuple;
		uint32_t atom;									// ensure atomic assignment
	}; // Tuple

#   define L(t) ((t).tuple.level)
#   define R(s) ((s).tuple.state)
#   define EQ(a, b) ((a).tuple.state == (b).tuple.state)
	int bit( int i, int k ) { return (i & (1 << (k - 1))) != 0; }
	int min( int a, int b ) { return a < b ? a : b; }

	int depth, width, mask;
	std::atomic<uint32_t> Q[N];							// maximal width of binary tree

	uint32_t QMAX( int w, unsigned int id, int k ) {
		int low = ((id >> (k - 1)) ^ 1) << (k - 1);
		int high = min( low | mask >> (depth - (k - 1)), N );
		Tuple opp;
		for ( int i = low; i <= high; i += 1 ) {
			opp.atom = Q[i]($);
			if ( L(opp) >= k ) return opp.atom;
		} // for
		return (Tuple){ {0, 0} }.atom;
	} // QMAX

	rl::var<int> data;

	void before() {
		depth = Log2( N );
		int width = 1 << depth;
		mask = width - 1;
	    for ( int i = 0; i < width; i += 1 ) {			// initialize shared data
			Q[i]($) = 0;
	    } // for
	} // before

	void thread( int id ) {
		Tuple opp, temp;
		for ( int k = 1; k <= depth; k += 1 ) {			// entry protocol, round
			opp.atom = QMAX( 1, id, k );
			temp.atom = L(opp) == k ? (Tuple){ {k, bit(id,k) ^ R(opp)} }.atom : (Tuple){ {k, 1} }.atom;
			Q[id]($) = temp.atom;
			opp.atom = QMAX( 2, id, k );
			temp.atom = L(opp) == k ? (Tuple){ {k, bit(id,k) ^ R(opp)} }.atom : Q[id]($);
			Q[id]($) = temp.atom;
//		  wait:	opp.atom = QMAX( id, k );
//			if ( (L(opp) == k && (bit(id,k) ^ EQ(opp, temp))) || L(opp) > k ) { Pause(); goto wait; }
//			int cnt = 0, max = -1;
			while ( L(opp) > k || (L(opp) == k && (bit(id,k) ^ EQ(opp, temp))) ) {
				Pause();
				opp.atom = QMAX( 3, id, k );
			} // while
		} // for
		data($) = id + 1;								// critical section
		Q[id]($) = 0;									// exit protocol
	} // thread
}; // PetersonT

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<PetersonT>(p);
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 PetersonT.cc" //
// End: //
