#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Leslie Lamport, A Fast Mutual Exclusion Algorithm, ACM Transactions on Computer Systems, 5(1), 1987, Fig. 2, p. 5

struct LamportFast : rl::test_suite<LamportFast, N> {
#   define await( E ) while ( ! (E) ) Pause()

	std::atomic<int> b[N], x, y;

	rl::var<int> data;

	void before() {
		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			b[i]($) = false;
		} // for
		y($) = N;
	} // before

	void thread( int id ) {
	  start: b[id]($) = true;							// entry protocol
		x($) = id;
		if ( y($) != N ) {
			b[id]($) = false;
			await( y($) == N );
			Pause();
			goto start;
		}
		y($) = id;
		if ( x($) != id ) {
			b[id]($) = false;
			for ( int j = 0; j < N; j += 1 )
				await( ! b[j]($) );
			if ( y($) != id ) {
//				await( y($) == N );
				Pause();
				goto start;
			}
		}
		data($) = id;									// critical section
		y($) = N;										// exit protocol
		b[id]($) = false;
	} // thread
}; // LamportFast

int main() {
	rl::test_params p;
	SetParms( p );
	rl::simulate<LamportFast>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 LamportFast.cc" //
// End: //
