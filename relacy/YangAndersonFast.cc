// Jae-Heon Yang, James H. Anderson, A Fast, Scalable Mutual Exclusion Algorithm, Distributed Computing, 9(1), 1995,
// Fig. 3, p. 57

#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

struct YangAndersonFast : rl::test_suite<YangAndersonFast, N> {
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

	Token B;

//======================================================

	rl::var<int> CS;									// shared resource for critical section

	std::atomic<int> Bx[N], Z;
	std::atomic<int> X CALIGN, Y CALIGN;				// signed numbers

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

		B.Q[0]($) = B.Q[1]($) = false;

		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			Bx[i]($) = false;
		} // for
		Y($) = -1;
		Z($) = false;
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

	void thread( int id ) {
		int level = levels[id];
		Tuple *state = states[id];

		TYPE flag;

		X($) = id;
		if ( Y($) != -1 ) goto SLOW;
		Y($) = id;
		if ( X($) != id ) goto SLOW;
		Bx[id]($) = true;
		if ( Z($) ) goto SLOW;
		if ( Y($) != id ) goto SLOW;

		binary_prologue( 1, &B );
		CS($) = id + 1;									// critical section
		binary_epilogue( 1, &B );

		Y($) = -1;
		Bx[id]($) = false;
		goto FINI;

	  SLOW: ;
		entrySlow( level, state );
		binary_prologue( 0, &B );
		CS($) = id + 1;									// critical section

		Bx[id]($) = false;
		if ( X($) == id ) {
			Z($) = true;
			flag = true;
			for ( int n = 0; n < N; n += 1 ) {
				if ( Bx[n]($) ) flag = false;
			} // for
			if ( flag ) Y($) = -1;
			Z($) = false;
		} // if

		binary_epilogue( 0, &B );
		exitSlow(
#ifdef TB
			id
#else
			level, state
#endif // TB
			);
	  FINI: ;
	} // thread
}; // YangAndersonFast

YangAndersonFast::Tuple YangAndersonFast::states[64][6];
int YangAndersonFast::levels[64] = { -1 };

int main() {
	rl::test_params p;
	SetParms( p );
	rl::simulate<YangAndersonFast>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 YangAndersonFast.cc" //
// End: //
