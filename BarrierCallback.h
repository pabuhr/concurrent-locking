#pragma once

#ifdef CALLBACK
	#define CB( text... ) text
	#ifdef CBCHECK
		#define CBCHK( text... ) text
	#else
		#define CBCHK( text... )
	#endif // CALLBACK
	#define CBSTART() \
		CBCHK( Fai( b->barcnt, 1 ) )
	#define CBEND() \
		if ( UNLIKELY( b->callback ) ) { \
			b->callback();			/* call action callback before triggering barrier */ \
			Fence();				/* ensure callback data visible for next cycle */ \
		} /* if */ \
		CBCHK( \
			if ( b->barcnt != b->group ) { \
				printf( "***ERROR*** barrier callback failure: barcnt %zu != %zu\n", b->barcnt, b->group ); \
				abort(); \
			} /* if */ \
			b->barcnt = 0; \
			WO( Fence() ) \
		)
	#define CBDECL() \
		void (*callback)( void )	/* closure for callback before triggering barrier */ \
		CBCHK(; VTYPE barcnt )		/* check all threads in barrier for callback */
	#define CBINIT() \
		CB(, .callback = NULL ) CBCHK(, .barcnt = 0 )
#else
#define CB( text... )
#define CBCHK( text... )
#define CBDECL( text... )
#define CBINIT( text... )
#define CBSTART( text... )
#define CBEND( text... )
#endif // CALLBACK
