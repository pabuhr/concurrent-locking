#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Leslie Lamport, A Fast Mutual Exclusion Algorithm, ACM Transactions on Computer Systems, 5(1), 1987, Fig. 2, p. 5

struct LamportFast : rl::test_suite<LamportFast, N> {
#   define await( E ) while ( ! (E) ) Pause()

	std::atomic<int> b[N+1], x, y;

	rl::var<int> data;

	void before() {
		for ( int i = 0; i <= N; i += 1 ) {				// initialize shared data
			b[i]($) = 0;
		} // for
		y($) = 0;
	} // before

	void thread( int id ) {
		id += 1;
	  start: b[id]($) = 1;								// entry protocol
		x($) = id;
		if ( y($) != 0 ) {
			b[id]($) = 0;
			await( y($) == 0 );
			Pause();
			goto start;
		}
		y($) = id;
		if ( x($) != id ) {
			b[id]($) = 0;
			for ( int j = 1; j <= N; j += 1 )
				await( ! b[j]($) );
			if ( y($) != id ) {
				await( y($) == 0 );
				Pause();
				goto start;
			}
		}
		data($) = id + 1;								// critical section
		y($) = 0;										// exit protocol
		b[id]($) = 0;
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
