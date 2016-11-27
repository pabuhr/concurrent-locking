#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

struct AndersonKim : rl::test_suite<AndersonKim, N> {
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

#if __WORDSIZE == 64
#define HALFSIZE uint32_t
#define WHOLESIZE uint64_t
#else  // SPARC
#define HALFSIZE uint16_t
#define WHOLESIZE uint32_t
#endif // __WORDSIZE == 64

	typedef union {
		struct {
			HALFSIZE free;
			HALFSIZE indx;
		} tuple;
		WHOLESIZE atom;									// atomic assignment
	} Ytype;

	std::atomic<int> X;
	std::atomic<WHOLESIZE> Y, Reset;
	std::atomic<int> Name_Taken[N], Obstacle[N];
	std::atomic<int> Infast;

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
			Name_Taken[i]($) = Obstacle[i]($) = false;
		} // for
		Y($) = (Ytype){ .tuple = { .free = true, .indx = 0 } }.atom;
		Reset($) = (Ytype){ .tuple = { .free = true, .indx = 0 } }.atom;
		Infast($) = false;
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

	void SLOW1( TYPE id, int level, Tuple *state ) {
		entrySlow( level, state );
		binary_prologue( 0, &B );
		CS($) = id + 1;									// critical section
		binary_epilogue( 0, &B );
		exitSlow( level, state );
	} // SLOW1

	void SLOW2( TYPE id, Ytype y, int level, Tuple *state ) {
		entrySlow( level, state );
		binary_prologue( 0, &B );
		CS($) = id + 1;									// critical section
		Y($) = 0;
		X($) = id;
		y.atom = Reset($);
		Obstacle[id]($) = false;
		Reset($) = (Ytype){ .tuple = { .free = false, .indx = y.tuple.indx } }.atom;
		if ( ! Name_Taken[ y.tuple.indx ]($) && ! Obstacle[ y.tuple.indx ]($) ) {
			HALFSIZE temp = (HALFSIZE)(y.tuple.indx + 1 < N ? y.tuple.indx + 1 : 0);
			Reset($) = (Ytype){ .tuple = { .free = true, .indx = temp } }.atom;
			Y($) = (Ytype){ .tuple = { .free = true, .indx = temp } }.atom;
		} // if
		binary_epilogue( 0, &B );
		exitSlow( level, state );
	} // SLOW2

	void thread( int id ) {
		int level = levels[id];
		Tuple *state = states[id];

		Ytype y;

		X($) = id;
		y.atom = Y($);
		if ( ! y.tuple.free ) {
			SLOW1( id, level, state	);
		} else {
			Y($) = 0;
			Obstacle[id]($) = true;
			if ( X($) != id || Infast($) ) {
				SLOW2( id, y, level, state );
			} else {
				Name_Taken[y.tuple.indx]($) = true;
				if ( Reset($) != y.atom ) {
					Name_Taken[y.tuple.indx]($) = false;
					SLOW2( id, y, level, state );
				} else {
					Infast($) = true;
					binary_prologue( 1, &B );
					CS($) = id + 1;						// critical section
					Obstacle[id]($) = false;
					Reset($) = (Ytype){ .tuple = { .free = false, .indx = y.tuple.indx } }.atom;
					if ( ! Obstacle[ y.tuple.indx ]($) ) {
						HALFSIZE temp = (HALFSIZE)(y.tuple.indx + 1 < N ? y.tuple.indx + 1 : 0);
						Reset($) = (Ytype){ .tuple = { .free = true, .indx = temp } }.atom;
						Y($) = (Ytype){ .tuple = { .free = true, .indx = temp } }.atom;
					} // if
					Name_Taken[y.tuple.indx]($) = false;
					binary_epilogue( 1, &B );
					Infast($) = false;
				} // if
			} // if
		} // if
	} // thread
}; // AndersonKim

AndersonKim::Tuple AndersonKim::states[64][6];
int AndersonKim::levels[64] = { -1 };

int main() {
	rl::test_params p;
	SetParms( p );
	rl::simulate<AndersonKim>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 AndersonKim.cc" //
// End: //
