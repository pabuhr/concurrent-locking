#include "relacy/relacy_std.hpp"
#include "Common.h"

enum { N = 8 };

// Joep L. W. Kessels, Arbitration Without Common Modifiable Variables, Acta Informatica, 17(2), 1982, pp. 140-141

struct Kessels : rl::test_suite<Kessels, N> {
	struct Token {
		std::atomic<int> Q[2], R[2];
	};
	Token t[N];

	rl::var<int> CS;									// shared resource for critical section

	void before() {
		for ( unsigned int id = 0; id < N; id += 1 ) {
			t[id].Q[0]($) = t[id].Q[1]($) = 0;
			t[id].R[0]($) = t[id].R[1]($) = 0;			// unnecessary
		} // for
	} // before

#   define inv( c ) (1 - c)
#   define plus( a, b ) ((a + b) & 1)

	void binary_prologue( int c, Token *t ) {
		int other = inv( c );
		t->Q[c]($) = 1;
		t->R[c]($) = plus( t->R[other]($), c );
		while ( t->Q[other]($) && t->R[c]($) == plus( t->R[other]($), c ) ) Pause(); // busy wait
	} // binary_prologue

	void binary_epilogue( int c, Token *t ) {
		t->Q[c]($) = 0;
	} // binary_epilogue

	void thread( int id ) {
		unsigned int n, e[N];

		n = N + id;
		while ( n > 1 ) {								// entry protocol
#if 1
//			int lr = n % 2;
			int lr = n & 1;
//			n = n / 2;
			n >>= 1;
			binary_prologue( lr, &t[n] );
			e[n] = lr;
#else
			binary_prologue( n % 2, &t[n / 2] );
			e[n / 2] = n % 2;
			n = n / 2;
#endif
		} // while
		CS($) = id + 1;									// critical section
		for ( n = 1; n < N; n = n + n + e[n] ) {		// exit protocol
//			n = n + n + e[n];
//			binary_epilogue( n & 1, &t[n >> 1] );
			binary_epilogue( e[n], &t[n] );
		} // for
	} // thread
}; // Kessels

int main() {
    rl::test_params p;
	SetParms( p );
	rl::simulate<Kessels>(p);
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "g++ -Wall -O3 -DNDEBUG Kessels.cc" //
// End: //
