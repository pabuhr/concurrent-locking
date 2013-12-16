#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

struct TaubenfeldBuhr : rl::test_suite<TaubenfeldBuhr, N> {
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
		int ridt, ridi;

		ridi = id;
		for ( int lv = 0; lv < depth; lv += 1 ) {		// entry protocol
//			ridi = id >> lv;							// round id for intent
			ridt = ridi >> 1;							// round id for turn
			intents[lv][ridi]($) = 1;					// declare intent
			turns[lv][ridt]($) = ridi;					// RACE
			while ( intents[lv][ridi ^ 1]($) == 1 && turns[lv][ridt]($) == ridi ) Pause();
			ridi = ridi >> 1;
		} // for
		data($) = id + 1;								// critical section
		for ( int lv = depth - 1; lv >= 0; lv -= 1 ) {	// exit protocol
			intents[lv][id >> lv]($) = 0;				// retract all intents in reverse order
		} // for
	} // thread
}; // TaubenfeldBuhr

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<TaubenfeldBuhr>(p);
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 TaubenfeldBuhr.cc" //
// End: //
