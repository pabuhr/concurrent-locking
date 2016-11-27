#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Gary L. Peterson and Michael J. Fischer, Economical Solutions for the Critical Section Problem in a Distributed
// System (Extended Abstract), Proceedings of the Ninth Annual ACM Symposium on Theory of Computing, STOC '77, 1977, p. 93

#include <stdint.h>										// uint*

struct PetersonT : rl::test_suite<PetersonT, N> {

#if __WORDSIZE == 64
#define HALFSIZE uint32_t
#define WHOLESIZE uint64_t
#else  // SPARC
#define HALFSIZE uint16_t
#define WHOLESIZE uint32_t
#endif // __WORDSIZE == 64

	typedef union {
		struct {
			HALFSIZE level;								// current level in tournament
			HALFSIZE state;								// intent to enter critical section
		} tuple;
		WHOLESIZE atom;									// atomic assignment
	} Tuple;

#   define L(t) ((t).tuple.level)
#   define R(s) ((s).tuple.state)
#   define EQ(a, b) ((a).tuple.state == (b).tuple.state)
	int bit( int i, int k ) { return (i & (1 << (k - 1))) != 0; }
	int min( int a, int b ) { return a < b ? a : b; }

	int depth, width, mask;
	std::atomic<WHOLESIZE> Q[N];						// maximal width of binary tree

	WHOLESIZE QMAX( unsigned int id, unsigned int k ) {
		int low = ((id >> (k - 1)) ^ 1) << (k - 1);
		int high = min( low | mask >> (depth - (k - 1)), N );
		Tuple opp;
		for ( int i = low; i <= high; i += 1 ) {
			opp.atom = Q[i]($);
			if ( L(opp) >= k ) return opp.atom;
		} // for
		return (Tuple){ .tuple = { .level = 0, .state = 0 } }.atom;
	} // QMAX

	rl::var<int> CS;									// shared resource for critical section

	void before() {
		depth = Clog2( N );								// maximal depth of binary tree
		int width = 1 << depth;							// maximal width of binary tree
		mask = width - 1;								// 1 bits for masking
		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			Q[i]($) = (Tuple){ .tuple = { .level = 0, .state = 0 } }.atom;
		} // for
	} // before

	void thread( int id ) {
		Tuple opp, temp;
		for ( HALFSIZE k = 1; k <= (HALFSIZE)depth; k += 1 ) { // entry protocol, round
			opp.atom = QMAX( id, k );
			Q[id]($) = L(opp) == k ? (Tuple){ .tuple = { .level = k, .state = (HALFSIZE)(bit(id,k) ^ R(opp)) } }.atom : (Tuple){ .tuple = {k, 1} }.atom;
			opp.atom = QMAX( id, k );
			temp.atom = L(opp) == k ? (Tuple){ .tuple = { .level = k,  .state = (HALFSIZE)(bit(id,k) ^ R(opp))} }.atom : Q[id]($);
			Q[id]($) = temp.atom;
#if 0
		  wait:	opp.atom = QMAX( id, k );
			if ( (L(opp) == k && (bit(id,k) ^ EQ(opp, temp))) || L(opp) > k ) { Pause(); goto wait; }
#else
			while ( L(opp) > k || (L(opp) == k && (bit(id,k) ^ EQ(opp, temp) )) ) {
				Pause();
				opp.atom = QMAX( id, k );
			} // while
#endif // 0
		} // for
		CS($) = id + 1;									// critical section
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
