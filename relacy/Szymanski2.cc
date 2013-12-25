#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Boleslaw K. Szymanski, Mutual Exclusion Revisited, Proceedings of the 5th Jerusalem Conference on Information
// Technology, 1990, Figure 1, Page 113.

struct Szymanski : rl::test_suite<Szymanski, N> {
	std::atomic<int> a[N], w[N], s[N];

	rl::var<int> data;

	void before() {
		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			a[i]($) = w[i]($) = s[i]($) = 0;
		} // for
	} // before

	void thread( int id ) {
		int j;

		a[id]($) = true;
		for ( j = 0; j < N; j += 1 ) while( s[j]($) );
		w[id]($) = true;
		a[id]($) = false;
		while ( ! s[id]($) ) {
			for ( j = 0; j < N && ! a[j]($); j += 1 );
			if ( j == N ) {
				s[id]($) = true;
				for ( j = 0; j < N && ! a[j]($); j += 1 );
				if ( j < N ) {
					s[id]($) = false;
				} else {
					w[id]($) = false;
					for ( j = 0; j < N; j += 1 ) while( w[j]($) );
				} // if
			} // if
			if ( j < N ) for ( j = 0; j < N && (w[j]($) || ! s[j]($)); j += 1 );
			if ( j != id && j < N ) {
				s[id]($) = true;
				w[id]($) = false;
			} // if
		} // while
		for ( j = 0; j < id; j += 1 ) while( w[j]($) || s[j]($) );
		data($) = id + 1;								// critical section
		s[id]($) = false;
	} // thread
}; // Szymanski

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Szymanski>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Szymanski2.cc" //
// End: //
