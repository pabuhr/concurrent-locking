#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

struct PetersonBuhr : rl::test_suite<PetersonBuhr, N> {
	struct Token {
		std::atomic<int> Q[2], R;
	};
	Token t[N];

	typedef struct {
		int es;											// left/right opponent
		Token *ns;										// pointer to path node from leaf to root
	} Tuple;
	static Tuple states[64][6];							// handle 64 threads with maximal tree depth of 6 nodes (lg 64)
	static int levels[64];								// minimal level for binary tree

	rl::var<int> data;

	void before() {
		for ( unsigned int id = 0; id < N; id += 1 ) {
			t[id].Q[0]($) = t[id].Q[1]($) = 0;
			unsigned int start = N + id, level = Log2( start );
			levels[id] = level - 1;
			for ( unsigned int s = 0; start > 1; start >>= 1, s += 1 ) {
				states[id][s].es = start & 1;
				states[id][s].ns = &t[start >> 1];
			} // for
		} // for
	} // before

#   define inv( c ) (1 - c)

	void binary_prologue( int c, Token *t ) {
		t->Q[c]($) = 1;
		t->R($) = c;
		while ( t->Q[inv( c )]($) && t->R($) == c ) Pause(); // busy wait
	} // binary_prologue

	void binary_epilogue( int c, Token *t ) {
		t->Q[c]($) = 0;
	} // binary_epilogue

	void thread( int id ) {
		int s, level = levels[id];
		Tuple *state = states[id];

		for ( s = 0; s <= level; s += 1 ) {				// entry protocol
			binary_prologue( state[s].es, state[s].ns );
		} // for
		data($) = id + 1;								// critical section
		for ( s = level; s >= 0; s -= 1 ) {				// exit protocol, reverse order
			binary_epilogue( state[s].es, state[s].ns );
		} // for
	} // thead
}; // PetersonBuhr

PetersonBuhr::Tuple PetersonBuhr::states[64][6];
int PetersonBuhr::levels[64] = { -1 };

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<PetersonBuhr>(p);
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 PetersonBuhr.cc" //
// End: //
