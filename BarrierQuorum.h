// Appears once in the barrier declaration.
#define QUORUM_DECL \
	VTYPE CALIGN nIn;		/* threads in barrier */ \
	VTYPE CALIGN nExit;		/* threads exiting barrier loop (stopping) */ \
	VSTYPE CALIGN nRescue;	/* threads rescuing block thread (not including threads to enter barrier) */ \
	TYPE CALIGN nTerm;		/* threads terminating done() */

// Called at end of barrier ctor.
#define QUORUM_CTOR( barrier ) \
	(barrier)->nIn = (barrier)->nExit = (barrier)->nTerm = 0; \
	(barrier)->nRescue = -1;
// Called at start of barrier dtor (reverse order from allocations).
#define QUORUM_DTOR( barrier )

// Called at start of barrier block.
#define QUORUM_ENTER( barrier ) Fai( (barrier)->nIn, 1 )
// Called before barrier leader returns.
#define QUORUM_EXIT( barrier ) Fai( (barrier)->nIn, - (barrier)->group )
// Called before barrier follower returns.
#define QUORUM_CHECK( barrier ) if ( (barrier)->nRescue > 0 ) return QuorumFail

// Singleton declaration in the barrier done() routine, along with any specific barrier code.
#define QUORUM_DONE( barrier ) \
	TYPE ticket = Fai( (barrier)->nExit, 1 ); \
	if ( ticket == 0 ) {								/* quorum leader */ \
		await( (barrier)->threads - (barrier)->nExit < (barrier)->group ); \
		await( (barrier)->nIn == (barrier)->threads - (barrier)->nExit ); \
		(barrier)->nRescue = ((barrier)->nIn > 0) ? (barrier)->group - (barrier)->nIn : 0; \
		Fence();										/* make write visible */ \
	} /* if */ \
	await( (barrier)->nRescue >= 0 ); \
	if ( ticket < (VTYPE)((barrier)->nRescue) ) block( (barrier) ); /* enter barrier to form a quorum */ \
	ticket = Fai( (barrier)->nTerm, 1 ); \
	if ( ticket == (barrier)->threads - 1 ) { \
		QUORUM_CTOR( barrier );							/* reset global variable */ \
	} /* if */
