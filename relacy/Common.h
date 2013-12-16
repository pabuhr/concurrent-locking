#define Pause() rl::yield(1, $)

// polymorphic integer log2, using clz, which returns the number of leading 0-bits,
// starting at the most significant bit (single instruction on x86)
#define Log2( n ) ( sizeof(n) * __CHAR_BIT__ - 1 - (					\
						( sizeof(n) ==  4 ) ? __builtin_clz( n ) :		\
						( sizeof(n) ==  8 ) ? __builtin_clzl( n ) :		\
						( sizeof(n) == 16 ) ? __builtin_clzll( n ) :	\
						-1 ) )

static inline int Clog2( int n ) {						// integer ceil( log2( n ) )
	if ( n <= 0 ) return -1;
	int ln = Log2( n );
	return ln + ( (n - (1 << ln)) != 0 );				// check for any 1 bits to the right of the most significant bit
}

static inline int cycleUp( int v, int n ) { return ( ((v) >= (n - 1)) ? 0 : (v + 1) ); }
static inline int cycleDown( int v, int n ) { return ( ((v) <= 0) ? (n - 1) : (v - 1) ); }

static void SetParms( rl::test_params &p ) {
    //p.search_type = rl::fair_context_bound_scheduler_type;
    //p.search_type = rl::sched_bound;
    //p.search_type = rl::sched_full;
    //p.execution_depth_limit = 100;
    //p.initial_state = "280572";
	p.search_type = rl::sched_random;
    p.iteration_count = 5000000;
    p.context_bound = 3;
} // SetParms

// Local Variables: //
// tab-width: 4 //
// compile-mode: "c++-mode" //
// End: //
