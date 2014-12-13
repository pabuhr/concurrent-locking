#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Boleslaw K. Szymanski, Mutual Exclusion Revisited, Proceedings of the 5th Jerusalem Conference on Information
// Technology, 1990, Figure 4, Page 115.

struct Szymanski3 : rl::test_suite<Szymanski3, N> {
	std::atomic<int> a[N], w[N], s[N], p[N];

	rl::var<int> data;

	void before() {
		for ( int i = 0; i < N; i += 1 ) {				// initialize shared data
			a[i]($) = w[i]($) = s[i]($) = p[i]($) = 0;
		} // for
	} // before

	void thread( int id ) {
		int la[N], lp[N];
		int j, k = 0;

		a[id]($) = true;
		for ( j = 0; j < N; j += 1 ) {
			la[j] = w[j]($); lp[j] = p[j]($);
		} // for
		p[id]($) = ! p[id]($);
		w[id]($) = true;
		for ( j = 0; j < N; j += 1 ) while( s[j]($) );
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
					for ( j = 0; j < N; j += 1 ) while( w[j]($) && ! a[j]($) );
				} // if
			} // if
			if ( j < N ) for ( j = 0; j < N && ( w[j]($) || ! s[j]($) ); j += 1 );
			if ( j != id && j < N ) {
				s[id]($) = true;
				w[id]($) = false;
			} // if
		} // while
		while ( k < N ) {
			for ( k = 0; k < N && ( ! la[k] || p[k]($) != lp[k] || ! w[k]($) || ! s[k]($) ); k += 1 );
			if ( k < N ) {
				for ( j = 0; j < N; j += 1 )
					if ( j == id ) a[id]($) = true;
					else while ( ! a[j]($) && s[j]($) );
				for ( j = 0; j < N; j += 1 )
					if ( j == id ) a[id]($) = false;
					else while( a[j]($) && s[j]($) );
			} // if
		} // while
		for ( j = 0; j < id; j += 1 ) while ( ! a[j]($) && ( w[j]($) || s[j]($) ) );
		data($) = id + 1;								// critical section
		s[id]($) = false;
	} // thread
}; // Szymanski3

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Szymanski3>( p );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG -I/u/pabuhr/software/relacy_2_4 Szymanski3.cc" //
// End: //
