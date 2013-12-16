#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8, Degree = 3 };

#include <math.h>

// Xiaodong Zhang, Yong Yan and Robert Castaneda, Evaluating and Designing Software Mutual Exclusion Algorithms on
// Shared-Memory Multiprocessors, Parallel Distributed Technology: Systems Applications, IEEE, 1996, 4(1), Figure 14,
// p. 37

struct ZhangdT : rl::test_suite<ZhangdT, N> {
#   define min( x, y ) (x < y ? x : y)
#   define logx( N, b ) (log(N) / log(b))

	std::atomic<int> x[N][N];

	rl::var<int> data;

	void before() {
		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			for ( int j = 0; j < N; j += 1 ) {
				x[i][j]($) = 0;
			} // for
		} // for
	} // before

	void thread( int id ) {
		int high, k, i, j, l, len;

		high = ceil( logx( N, Degree ) );				// maximal depth of binary tree
		j = 0;
		l = id;

		k = id / Degree;
		len = N;
		while ( j < high ) {
		  yy: x[j][l]($) = 1;
			for ( i = k * Degree; i < l; i += 1 ) {
				if ( x[j][i]($) ) {
					x[j][l]($) = 0;
					while ( x[j][i]($) != 0 ) Pause();
					goto yy;
				} // if
			} // for
			for ( i = l + 1; i < min((k + 1) * Degree, len); i += 1 )
				while ( x[j][i]($) ) Pause();
			l = l / Degree;
			k = k / Degree;
			j += 1;
			len = (len % Degree == 0) ? len / Degree : len / Degree + 1;
		} // for
		data($) = id + 1;								// critical section
		//j = high;
		int pow2 = pow( Degree, high );
		while ( j > 0 ) {
			j -= 1;
			pow2 /= Degree;
			l = id / pow2;
			x[j][l]($) = 0;
		} // while
	} // thread
}; // ZhangdT

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<ZhangdT>(p);
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 ZhangdT.cc" //
// End: //
