#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

struct Triangle : rl::test_suite<Triangle, N> {
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

#   define inv( c ) (1 - c)

	void binary_prologue( int c, Token *t ) {
		t->Q[c]($) = 1;
		t->R($) = c;
		while ( t->Q[inv( c )]($) && t->R($) == c ) Pause(); // busy wait
	} // binary_prologue

	void binary_epilogue( int c, Token *t ) {
		t->Q[c]($) = 0;
	} // binary_epilogue

//======================================================

#   define await( E ) while ( ! (E) ) Pause()

	std::atomic<int> b[N], x, y;

//======================================================

	std::atomic<int> bintents[2], last;

//======================================================

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

		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			b[i]($) = false;
		} // for
		y($) = N;

		bintents[0]($) = bintents[1]($) = false;
	} // before

	void thread( int id ) {
		int level = levels[id];
		Tuple *state = states[id];

		if ( y($) != N ) goto aside;
		b[id]($) = true;								// entry protocol
		x($) = id;
		if ( y($) != N ) {
			b[id]($) = false;
			goto aside;
		} // if
		y($) = id;
		if ( x($) != id ) {
			b[id]($) = false;
			for ( int j = 0; j < N; j += 1 )
				await( y($) != id || ! b[j]($) );
			if ( y($) != id ) goto aside;
		} // if
		bintents[0]($) = true;
		last($) = false;
		await( ! bintents[1]($) || last($) );

		data($) = id;									// critical section

		bintents[0]($) = false;
		y($) = N;										// exit protocol
		b[id]($) = false;
		goto fini;

	  aside:
		for ( int s = 0; s <= level; s += 1 ) {			// entry protocol
			binary_prologue( state[s].es, state[s].ns );
		} // for
		bintents[1]($) = true;
		last($) = true;
		await( ! bintents[0]($) || ! last($) );

		data($) = id + 1;								// critical section

		bintents[1]($) = false;
		for ( int s = level; s >= 0; s -= 1 ) {			// exit protocol, reverse order
			binary_epilogue( state[s].es, state[s].ns );
		} // for
	  fini: ;
	} // thread
}; // Triangle

Triangle::Tuple Triangle::states[64][6];
int Triangle::levels[64] = { -1 };

int main() {
	rl::test_params p;
	SetParms( p );
	rl::simulate<Triangle>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Triangle.cc" //
// End: //
