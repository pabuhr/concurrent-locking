// Wim H. Hesselink, Verifying a simplification of mutual exclusion by Lycklama-Hadzilacos, Acta Informatica (2013)
// 50:199-228, Fig. 2

// Set to work with BurnsLynch.

// This algorithm does not generalize because the BurnsLynch lock uses intents, which must also be embedded in
// the entry protocol of the FCFS algorithm.

#include "FCFSTest.h"

#ifdef FCFS
#define FCFSGlobal() \
	FCFSTestGlobal(); \
	enum { FCFS_R = 3 }; \
	static VTYPE * FCFSturn CALIGN
#define FCFSLocal() \
	typeof(N) Range CALIGN = N * FCFS_R, FCFSnx CALIGN = 0; \
	TYPE FCFScopy[Range];
#define FCFSEnter() \
	FCFSTestIntroTop(); \
	intents[id] = WantIn; /* from BL */ \
	Fence(); \
	for ( typeof(Range) k = 0; k < Range; k += 1 ) FCFScopy[k] = FCFSturn[k]; \
	FCFSturn[id * FCFS_R + FCFSnx] = true; \
	intents[id] = DontWantIn; /* from BL */ \
	Fence(); \
	FCFSTestIntroMid(); \
	for ( typeof(Range) kk = 0; kk < Range; kk += 1 ) \
		if ( FCFScopy[kk] != 0 ) { \
			await( ! FCFSturn[kk] ); \
			FCFScopy[kk] = false; \
		} \
	FCFSTestIntroBot()
#define FCFSExitAcq() \
	FCFSTestExit()
#define FCFSExitRel() \
	FCFSturn[id * FCFS_R + FCFSnx] = false; \
	FCFSnx = cycleUp( FCFSnx, FCFS_R )
#define FCFSCtor() \
  	FCFSturn = Allocator( sizeof(typeof(FCFSturn[0])) * N * FCFS_R ); \
	for ( typeof(N) i = 0; i < N * FCFS_R; i += 1 ) { \
		FCFSturn[i] = 0; \
	} \
	FCFSTestCtor()
#define FCFSDtor() \
	FCFSTestDtor(); \
	free( (void *)FCFSturn )

#else

#define FCFSGlobal() int __attribute__(( unused )) FCFSGlobal
#define FCFSLocal() int __attribute__(( unused )) FCFSLocal
#define FCFSEnter() do {} while(0)
#define FCFSExit() do {} while(0)
#define FCFSCtor() do {} while(0)
#define FCFSDtor() do {} while(0)
#endif // FCFS

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BurnsLynch Harness.c -lpthread -lm -D`hostname` -DCFMT -DFCFS=FCFS_WH13_BL -DFCFSTest" //
// End: //
