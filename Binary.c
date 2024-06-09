//#define inv( c ) (1 - (c))
//#define inv( c ) (! (c))
//#define inv( c ) ((c) ^ 1)

typedef struct CALIGN {
	VTYPE Q[2];
	VTYPE
#if defined( KESSELS2 )
	R[2];
#else
	R;
#endif // KESSELS
} Token;

static inline void binary_prologue( TYPE c, volatile Token * t ) {
	int other = inv( c );								// int is better than TYPE
#if defined( KESSELS2 )
	t->Q[c] = 1;
	Fence();											// force store before more loads
	t->R[c] = t->R[other] ^ c ;
	Fence();											// force store before more loads
	while ( t->Q[other] == 1 && t->R[c] == (t->R[other] ^ c) ) Pause() ;
#elif defined( DEKKERORIG )
  A1: t->Q[c] = 1;
	Fence();											// force store before more loads
  L1: if ( FASTPATH( t->Q[other] ) ) {
		if ( t->R != c ) { Pause(); goto L1; }
		t->Q[c] = 0;
	  B1: if ( t->R == c ) { Pause(); goto B1; }		// low priority busy wait
		goto A1;
	} // if
#elif defined( DEKKERA )
	for ( ;; ) {
		t->Q[c] = 1;
		Fence();										// force store before more loads
	  if ( FASTPATH( ! t->Q[other] ) ) break;
		if ( t->R == c ) {
			t->Q[c] = 0;
			while ( t->R == c ) Pause();				// low priority busy wait
		} // if
	} // for
#elif defined( DEKKERB )
	for ( ;; ) {
		t->Q[c] = 1;
		Fence();										// force store before more loads
	  if ( FASTPATH( ! t->Q[other] ) ) break;
	  if ( t->R != c ) {
			while ( t->Q[other] ) Pause();				// low priority busy wait
			break;
		} // if
		t->Q[c] = 0;
		while ( t->R == c ) Pause();					// low priority busy wait
	} // for
#elif defined( DORAN )
	t->Q[c] = 1;
	Fence();											// force store before more loads
	if ( FASTPATH( t->Q[other] ) ) {
		if ( t->R == c ) {
			t->Q[c] = 0;
			while ( t->R == c ) Pause();				// low priority busy wait
			t->Q[c] = 1;
			Fence();									// force store before more loads
		} // if
		while ( t->Q[other] ) Pause();					// low priority busy wait
	} // if
#elif defined( DEKKERRW )
	for ( ;; ) {
		t->Q[c] = 1;
		Fence();										// force store before more loads
	  if ( ! t->Q[other] ) break;
	  if ( t->R != c ) {
			while ( t->Q[other] ) Pause() ;
			break;
		} // if
		t->Q[c] = 0;
		while ( t->Q[other] && t->R == c ) Pause();
	} // for
#elif defined( TSAY )
	t->Q[c] = 1;
	t->R = c;											// RACE
	Fence();											// force store before more loads
	if ( FASTPATH( t->Q[other] ) )
		while ( t->R == c ) Pause();					// busy wait
#elif defined( PETERSONU )
	t->Q[c] = 1;
	Fence();											// force store before more loads
	if ( FASTPATH( t->Q[other] ) ) {
		t->R = c;										// RACE
		Fence();										// force store before more loads
		while ( t->Q[other] && t->R == c ) Pause();		// busy wait
	} // if
#else // Peterson (default)
	t->Q[c] = 1;
	t->R = c;											// RACE
	Fence();											// force store before more loads
	while ( t->Q[other] && t->R == c ) Pause();			// busy wait
#endif
} // binary_prologue


// No fence is necessary *before* binary_epilogue for the TSO memory-model because there are only stores, which cannot
// be moved before prior reads. The exception is DEKKERRW because it has a read of "turn" (t->R) that can be moved
// before a prior write in the entry protocol.  However, turn is only written in the exit protocol. MX ensures no other
// thread can change turn except the thread in the CS. Finally, turn cannot be raised before the last line of the
// DEKKERRW entry-protocol because turn is read on the last line.  Hence, if the thread in the CS reads turn before or
// after the CS, it must be the same value and it cannot be flickering because of a write. Therefore, no fence is
// required before binary_epilogue in DEKKERRW.
	
static inline void binary_epilogue( TYPE c, volatile Token * t ) {
#if defined( KESSELS2 )
	t->Q[c] = 0;
#elif defined( DEKKERORIG ) || defined( DEKKERA ) || defined( DEKKERB ) || defined( DORAN )
	t->R = c;
	t->Q[c] = 0;
#elif defined( DEKKERRW )
	if ( t->R != c ) {
		t->R = c;
	} // if
	t->Q[c] = 0;
#elif defined( TSAY )
	t->Q[c] = 0;
	t->R = c;
#else // PETERSONU & Peterson (default)
	WO( Fence(); );										// force store before more loads
	t->Q[c] = 0;
#endif
} // binary_epilogue

// Local Variables: //
// tab-width: 4 //
// End: //
