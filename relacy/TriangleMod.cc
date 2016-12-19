#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

struct TriangleMod : rl::test_suite<TriangleMod, N> {
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

	Token B;

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

		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			b[i]($) = false;
		} // for
		y($) = N;

		B.Q[0]($) = B.Q[1]($) = false;
	} // before


	void entrySlow( int level, Tuple *state ) {
		for ( int s = 0; s <= level; s += 1 ) {			// entry protocol
			binary_prologue( state[s].es, state[s].ns );
		} // for
	} // entrySlow

	void exitSlow( int level, Tuple *state ) {
		for ( int s = level; s >= 0; s -= 1 ) {			// exit protocol, reverse order
			binary_epilogue( state[s].es, state[s].ns );
		} // for
	} // exitSlow

//=========================================================================

	bool entryFast( int id ) {
		if ( y($) != N ) return false;
		b[id]($) = true;								// entry protocol
		x($) = id;
		if ( y($) != N ) {
			b[id]($) = false;
			return false;
		} // if
		y($) = id;
		if ( x($) != id ) {
			b[id]($) = false;
			for ( int k = 0; y($) == id && k < N; k += 1 )
				await( y($) != id || ! b[k]($) );
			if ( y($) != id ) return false;
		} // if
		return true;
	} // entryFast

	void exitFast( unsigned int id ) {
		y($) = N;										// exit protocol
		b[id]($) = false;
	} // exitFast


	bool entryComb( unsigned int id, int level, Tuple *state ) {
		bool fa = entryFast( id );
		if ( ! fa ) {
			entrySlow( level, state );
		} // if
		//entryBinary( fa );
		binary_prologue( fa, &B );
		return fa;
	} // entryComb

	void exitComb( unsigned int id, bool fa, int level, Tuple *state ) {
		//exitBinary( fa );
		binary_epilogue( fa, &B );
		if ( fa )
			exitFast( id );
		else
			exitSlow( level, state );
	} // exitComb


	void thread( int id ) {
		int level = levels[id];
		Tuple *state = states[id];

		bool b = entryComb( id, level, state );
		CS($) = id + 1;									// critical section
		exitComb( id, b, level, state );
	} // thread
}; // TriangleMod

TriangleMod::Tuple TriangleMod::states[64][6];
int TriangleMod::levels[64] = { -1 };

int main() {
	rl::test_params p;
	SetParms( p );
	rl::simulate<TriangleMod>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 TriangleMod.cc" //
// End: //
