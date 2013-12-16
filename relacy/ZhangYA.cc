#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Xiaodong Zhang, Yong Yan and Robert Castaneda, Evaluating and Designing Software Mutual Exclusion Algorithms on
// Shared-Memory Multiprocessors, Parallel Distributed Technology: Systems Applications, IEEE, 1996, 4(1), Figure 5,
// p. 31

struct ZhangYA : rl::test_suite<ZhangYA, N> {
	std::atomic<int> c[N][N + 1], p[N][N + 1], t[N][N + 1];

	rl::var<int> data;

	void before() {
		for ( int i = 0; i < N; i += 1 ) {
			for ( int j = 0; j < N + 1; j += 1 ) {
				c[i][j]($) = -1;
				p[i][j]($) = 0;
			} // for
		} // for
	} // before

	void thread( int id ) {
		int rival, j, ridi, ridt, high = Clog2( N );	// maximal depth of binary tree

		for ( j = 0; j < high; j += 1 ) {
			ridi = id >> j;								// round id for intent
			ridt = ridi >> 1;							// round id for turn
			c[j][ridi]($) = id;
			t[j][ridt]($) = id;
			p[j][id]($) = 0;
			rival = c[j][ridi ^ 1]($);
			if ( rival != -1 ) {
				if ( t[j][ridt]($) == id ) {
					if ( p[j][rival]($) == 0 ) {
						p[j][rival]($) = 1;
					} // if
					while ( p[j][id]($) == 0 ) Pause();
					if ( t[j][ridt]($) == id ) {
						while ( p[j][id]($) <= 1 ) Pause();
					}
				} // if
			} // if
		} // for
		data($) = id + 1;								// critical section
		for ( j = high - 1; j >= 0; j -= 1 ) {
			c[j][id / (1 << j)]($) = -1;
			rival = t[j][id / (1 << (j + 1))]($);
			if ( rival != id ) {
				assert( rival != -1 );
				p[j][rival] ($)= 2;
			}
		} // while
	} // thread
}; // ZhangYA

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<ZhangYA>(p);
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 ZhangYA.cc" //
// End: //
