//#define inv( c ) (1 - c)
//#define inv( c ) (! (c))
//#define inv( c ) ((c) ^ 1)
#define await( E ) while ( ! (E) ) Pause()

typedef struct CALIGN {
	TYPE Q[2];
#if defined( __sparc ) && defined( TB )
	int /* gcc on SPARC has an issue with unsigned affecting performance */
#else
	TYPE
#endif // TB
	R;
} Token;

static inline void binary_prologue( TYPE c, volatile Token *t ) {
	TYPE other = inv( c );
#if defined( DEKKER )
	t->Q[c] = 1;
	Fence();											// force store before more loads
	while ( t->Q[other] ) {
		if ( t->R == c ) {
			t->Q[c] = 0;
			Fence();									// force store before more loads
			while ( t->R == c ) Pause();				// low priority busy wait
			t->Q[c] = 1;
			Fence();									// force store before more loads
//		} else {
//			Pause();
		} // if
	} // while
#elif defined( DEKKERWHH )
	for ( ;; ) {
		t->Q[c] = 1;
		Fence();										// force store before more loads
	  if ( ! t->Q[other] ) break;
	  if ( t->R == c ) {
			await( ! t->Q[other] );
			break;
		} // if
		t->Q[c] = 0;
		Fence();
		await( t->R == c || ! t->Q[other] );
	} // for
#elif defined( TSAY )
	t->Q[c] = 1;
	t->R = c;											// RACE
	Fence();											// force store before more loads
	if ( t->Q[other] ) while ( t->R == c ) Pause();		// busy wait
#else // Peterson (default)
	t->Q[c] = 1;
	t->R = c;											// RACE
	Fence();											// force store before more loads
	while ( t->Q[other] && t->R == c ) Pause();			// busy wait
#endif
} // binary_prologue

static inline void binary_epilogue( TYPE c, volatile Token *t ) {
#if defined( DEKKER )
	t->R = c;
	t->Q[c] = 0;
#elif defined( DEKKERWHH )
	t->R = c;
	if ( t->R == c ) {
		t->R = inv( c );
	} // if
	t->Q[c] = 0;
#elif defined( TSAY )
	t->Q[c] = 0;
	t->R = c;
#else // Peterson (default)
	t->Q[c] = 0;
#endif
} // binary_epilogue

// Local Variables: //
// tab-width: 4 //
// End: //
