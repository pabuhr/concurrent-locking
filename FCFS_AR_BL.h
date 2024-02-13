// Alex A. Aravind, Simple, Space-Efficient, and Fairness Improved FCFS Mutual Exclusion Algorithms, J. Parallel
// Distrib. Comput. 73 (2013), Fig. 3

// Set to work with BurnsLynch.

// This algorithm does not generalize because the BurnsLynch lock uses intents, which must also be embedded in
// the entry protocol of the FCFS algorithm.

#include "FCFSTest.h"

#ifdef FCFS
#define FCFSGlobal() \
	FCFSTestGlobal(); \
	static VTYPE * FCFS_T CALIGN
#define FCFSLocal() VTYPE FCFS_t CALIGN = 1, FCFS_S[N] CALIGN 
#define FCFSEnter() \
	FCFSTestIntroTop(); \
	intents[id] = WantIn; /* from BL */ \
	Fence(); \
	for ( typeof(N) j = 0; j < N; j += 1 ) FCFS_S[j] = FCFS_T[j]; \
	FCFS_T[id] = FCFS_t; \
	intents[id] = DontWantIn; /* from BL */ \
	Fence(); \
	FCFSTestIntroMid(); \
	for ( typeof(N) j = 0; j < N; j += 1 ) \
		if ( FCFS_S[j] != 0 ) \
			await( FCFS_S[j] != FCFS_T[j] )
#define FCFSExitAcq() \
	FCFSTestIntroBot(); \
	FCFS_T[id] = 0
#define FCFSExitRel() \
	if ( FCFS_t < 3 ) FCFS_t += 1; else FCFS_t = 1
#define FCFSCtor() \
  	FCFS_T = Allocator( sizeof(typeof(FCFS_T[0])) * N ); \
	for ( typeof(N) i = 0; i < N; i += 1 ) { \
		FCFS_T[i] = 0; \
	} \
	FCFSTestCtor()
#define FCFSDtor() \
	FCFSTestDtor(); \
	free( (void *)FCFS_T ); \

#else

#define FCFSGlobal() int __attribute__(( unused )) FCFSGlobal
#define FCFSLocal() int __attribute__(( unused )) FCFSLocal
#define FCFSEnter() do {} while(0)
#define FCFSExitAcq() do {} while(0)
#define FCFSExitRel() do {} while(0)
#define FCFSCtor() do {} while(0)
#define FCFSDtor() do {} while(0)
#endif // FCFS

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BurnsLynch -lpthread -lm -D`hostname` -DCFMT -DFCFS=FCFS_AR_BL -DFCFSTest" //
// End: //
