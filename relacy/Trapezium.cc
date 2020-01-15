#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

struct Trapezium : rl::test_suite<Trapezium, N> {
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

#   define inv( c ) ( (c) ^ 1 )

	void binary_prologue( int c, Token * t ) {
		t->Q[c]($) = 1;
		t->R($) = c;
		while ( t->Q[inv( c )]($) && t->R($) == c ) Pause(); // busy wait
	} // binary_prologue

	void binary_epilogue( int c, Token * t ) {
		t->Q[c]($) = 0;
	} // binary_epilogue

//======================================================

#   define await( E ) while ( ! (E) ) Pause()

	enum { K = NEST };
	struct FastPaths : rl::test_suite<Trapezium, N> {
		std::atomic<int> b[N], x, y;
		Token B;
	};
	FastPaths fastpaths[K];

//======================================================

	rl::var<int> CS;									// shared resource for critical section

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

		for ( int k = 0; k < K; k += 1 ) {
			for ( int i = 0; i < N; i += 1 ) {			// initialize shared data
				fastpaths[k].b[i]($) = false;
			} // for
			fastpaths[k].y($) = N;
			fastpaths[k].B.Q[0]($) = fastpaths[k].B.Q[1]($) = false;
		} // for
	} // before

	void thread( int id ) {
		int level = levels[id];
		Tuple * state = states[id];

		int fa;
		FastPaths * fp;

		for ( fa = 0; fa < K; fa += 1 ) {
			fp = &fastpaths[fa];						// optimization
			if ( fp->y($) != N ) continue;
			fp->b[id]($) = true;						// entry protocol
			fp->x($) = id;
			if ( fp->y($) != N ) {
				fp->b[id]($) = false;
				continue;
			} // if
			fp->y($) = id;
			if ( fp->x($) != id ) {
				fp->b[id]($) = false;
				for ( int k = 0; fp->y($) == id && k < N; k += 1 )
					await( fp->y($) != id || ! fp->b[k]($) );
				if ( fp->y($) != id ) continue;
			} // if
			goto Fast;
		} // for
		goto Slow;

	  Fast: ;
		for ( int i = fa; i >= 0; i -= 1 ) {
			binary_prologue( i < fa, &fastpaths[i].B );
		} // for

		CS($) = id + 1;									// critical section

		for ( int i = 0; i <= fa; i += 1 ) {
			binary_epilogue( i < fa, &fastpaths[i].B );
		} // for

		fp->y($) = N;									// exit fast protocol
		fp->b[id]($) = false;

		goto FINI;

	  Slow:
		for ( int s = 0; s <= level; s += 1 ) {			// entry protocol
			binary_prologue( state[s].es, state[s].ns );
		} // for

		fa -= 1;
		for ( int i = fa; i >= 0; i -= 1 ) {
			binary_prologue( 1, &fastpaths[i].B );
		} // for

		CS($) = id + 1;									// critical section

		for ( int i = 0; i <= fa; i += 1 ) {
			binary_epilogue( 1, &fastpaths[i].B );
		} // for

		for ( int s = level; s >= 0; s -= 1 ) {			// exit protocol, reverse order
			binary_epilogue( state[s].es, state[s].ns );
		} // for

	  FINI: ;
	} // thread
}; // Trapezium

Trapezium::Tuple Trapezium::states[64][6];
int Trapezium::levels[64] = { -1 };

int main() {
	rl::test_params p;
	SetParms( p );
	rl::simulate<Trapezium>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -DNEST=3 -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Trapezium.cc" //
// End: //
