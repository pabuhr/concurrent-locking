#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8, Degree = 2 };

#include <math.h>

// Xiaodong Zhang, Yong Yan and Robert Castaneda, Evaluating and Designing Software Mutual Exclusion Algorithms on
// Shared-Memory Multiprocessors, Parallel Distributed Technology: Systems Applications, IEEE, 1996, 4(1), Figure 17,
// p. 39

struct Zhang2T : rl::test_suite<Zhang2T, N> {
#   define min( x, y ) (x < y ? x : y)
#   define logx( N, b ) (log(N) / log(b))

	std::atomic<int> x[N][N], c[N][N];
	int lN;

	rl::var<int> CS;									// shared resource for critical section

	void before() {
		lN = N;
		if ( N % 2 == 1 ) lN += 1;

		for ( int i = 0; i < lN; i += 1 ) {				// initialize shared data
			for ( int j = 0; j < lN; j += 1 ) {
				x[i][j]($) = -1;
				c[i][j]($) = 0;
			} // for
		} // for
	} // before

	void thread( int id ) {
		int high, j, l, rival;

		j = 0;
		l = id;
		high = Clog2( N );							// maximal depth of binary tree
		while ( j < high ) {
			if ( l % 2 == 0 ) {
				x[j][l]($) = id;
				c[j][id]($) = 1;
				rival = x[j][l + 1]($);
				if ( rival != -1 ) {
					c[j][rival]($) = 0;
					while( c[j][id]($) != 0 ) Pause();
				}
			} else {
				x[j][l]($) = id;
			  yy:
				rival = x[j][l - 1]($);
				if ( rival != -1 ) {
					c[j][rival]($) = 0;
					while ( c[j][id]($) != 0 ) Pause();
					c[j][id]($) = 1;
					goto yy;
				} // if
			} // if
			l /= 2;
			j += 1;
		}
		CS($) = id + 1;									// critical section
		j = high;
		//int pow2 = pow( Degree, high );
		int pow2 = 1 << high;
		while ( j > 0 ) {
			j -= 1;
			pow2 /= Degree;
			l = id / pow2;
			x[j][l]($) = -1;
			int temp = (l % 2 == 0) ? l + 1 : l - 1;
			rival = x[j][temp]($);
			if ( rival != -1 ) {
				c[j][rival]($) = 0;
			}
		} // while
	} // thread
}; // Zhang2T

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Zhang2T>(p);
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Zhang2T.cc" //
// End: //
