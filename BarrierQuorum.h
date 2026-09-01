// Appears once in the barrier declaration.
#define QUORUM_DECL \
	VTYPE CALIGN nX;	/* number of threads seen the halt flag */ \
	VTYPE CALIGN nIn;	/* number of barrier calls */ \
	VTYPE CALIGN poison; /* indicates quorum failure */ \
	struct CALIGN { \
		VTYPE halt; \
	} * quorum;

#define QUORUM_CTOR_PRIVATE( barrier ) \
	(barrier)->nX = 0; \
	(barrier)->nIn = 0; \
	(barrier)->poison = false;

// Called at end of barrier ctor.
#define QUORUM_CTOR( barrier ) \
	QUORUM_CTOR_PRIVATE( barrier ); \
	(barrier)->quorum =  (typeof((barrier)->quorum[0])  *)(malloc( (barrier)->threads * sizeof((barrier)->quorum[0]) )); \
	for ( size_t i = 0; i < (barrier)->threads; i += 1 ) { \
		(barrier)->quorum[i].halt = 0; \
	}

// Called at start of barrier dtor.
#define QUORUM_DTOR( barrier ) free( (void *)(barrier)->quorum )
// Called at start of barrier block.
#define QUORUM_ENTER( barrier ) Fai( (barrier)->nIn, 1 )
// Called before barrier leader returns.
#define QUORUM_EXIT( barrier ) Fai( (barrier)->nIn, - (barrier)->group )
// Called before barrier follower returns.
#define QUORUM_CHECK( barrier ) if ( (barrier)->poison ) return QuorumFail

// Singleton declaration in the barrier done() routine, along with any specific barrier code.
#define QUORUM_DONE( barrier ) \
	TYPE ownX = Fai( (barrier)->nX, 1 ); \
	if ( ownX == 0 ) {									/* quorum leader */ \
		await( (barrier)->threads - (barrier)->nX < (barrier)->group ); \
		await( (barrier)->nIn == (barrier)->threads - (barrier)->nX ); \
		TYPE w = (barrier)->nIn > 0 ? (barrier)->group - (barrier)->nIn : 0; \
		if ( w > 0 ) (barrier)->poison = true; \
		Fence();										/* make write visible */ \
		for ( size_t q = 0; q < (barrier)->threads; q += 1 ) { \
			if ( w > 0 ) { \
				(barrier)->quorum[q].halt = 1; w -= 1; \
			} else { \
				(barrier)->quorum[q].halt = 2; \
			} /* if */ \
			Fence();									/* make write visible */ \
		} /* for */ \
	} /* if */ \
	await( (barrier)->quorum[ownX].halt != 0 ); \
	if ( (barrier)->quorum[ownX].halt == 1 ) block( (barrier) ); /* enter barrier to form a quorum */ \
	(barrier)->quorum[ownX].halt = 0;					/* reset thread variables */ \
	Fence();											/* make write visible */ \
	if ( ownX == 0 ) { \
		for ( size_t q = 0; q < (barrier)->threads; q += 1 ) { /* wait for quorum followers to exit barrier */ \
			await( (barrier)->quorum[q].halt == 0 ); \
		} /* for */ \
		QUORUM_CTOR_PRIVATE( barrier );					/* reset global variable */ \
	} /* if */
