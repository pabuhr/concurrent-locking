// Macros to bracket a non-FCFS lock acquire to make it FCFS, e.g.:
//      FCFSintro(); acquire(m); FCFSexit();

#include "FCFSTest.h"

#define odd( v ) (v & 1)
#ifdef FCFS
#define FCFSGlobal() \
	FCFSTestGlobal(); \
	static VTYPE * FCFSturn CALIGN;
#define FCFSLocal() TYPE FCFSnx CALIGN, FCFScopy[N] CALIGN
#define FCFSEnter() \
	FCFSTestIntroTop(); \
	FCFSnx = FCFSturn[id] + 1; \
	FCFSturn[id] = -1; \
	Fence(); \
	for ( typeof(N) k = 0; k < N; k += 1 ) FCFScopy[k] = FCFSturn[k]; \
	FCFSturn[id] = FCFSnx; \
	FCFSTestIntroMid(); \
	for ( typeof(N) k = 0; k < N; k += 1 ) \
		if ( odd( FCFScopy[k] ) ) \
			await( FCFSturn[k] != FCFScopy[k] )
#define FCFSExitAcq() \
	FCFSTestIntroBot(); \
	/* FCFSturn[id] = ((FCFSturn[id] + 1) % 6); */ \
	/* FCFSturn[id] = (7 & (FCFSturn[id] + 1)); */ \
	TYPE FCFStemp = FCFSturn[id]; \
	FCFSturn[id] = (FCFStemp == 5 ? 0 : FCFStemp + 1 )
#define FCFSExitRel()
#define FCFSCtor() \
  	FCFSturn = Allocator( sizeof(typeof(FCFSturn[0])) * N ); \
	for ( typeof(N) i = 0; i < N; i += 1 ) { \
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
#define FCFSExitAcq() do {} while(0)
#define FCFSExitRel() do {} while(0)
#define FCFSCtor() do {} while(0)
#define FCFSDtor() do {} while(0)
#endif // FCFS

// Local Variables: //
// tab-width: 4 //
// End: //
