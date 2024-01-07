// Wim H. Hesselink, Mutual exclusion by four shared bits with not more than quadratic complexity, Science of Computer
// Programming, 102 (2015), Fig. 2

#include "FCFSTest.h"

#ifdef FCFS
#define FCFSGlobal() \
	FCFSTestGlobal(); \
	enum { FCFS_R = 3 }; \
	static VTYPE * FCFSdw CALIGN, * FCFSturn CALIGN
#define FCFSLocal() \
	typeof(N) Range CALIGN = N * FCFS_R, FCFSnx CALIGN = 0; \
	TYPE FCFScopy[2 * N * FCFS_R];
#define FCFSEnter() \
	FCFSTestIntroTop(); \
	FCFSdw[id] = true; \
	Fence(); \
	for ( typeof(Range) k = 0; k < Range; k += 1 ) FCFScopy[k] = FCFSturn[k]; \
	FCFSturn[2 * id + FCFSnx] = true; \
	FCFSdw[id] = false; \
	Fence(); \
	FCFSTestIntroMid(); \
	for ( typeof(Range) kk = 0; kk < Range; kk += 1 ) \
		if ( FCFScopy[kk] ) { \
			await( ! FCFSturn[kk] ); \
		} \
	FCFSTestIntroBot();
#define FCFSExitAcq() \
	FCFSTestExit(); \
	FCFSturn[2 * id + FCFSnx] = false; \
	FCFSnx = ! FCFSnx; \
	Fence(); \
	for ( typeof(N) thr = 0; thr < N; thr += 1 ) \
		await( ! FCFSdw[thr] )
#define FCFSExitRel()
#define FCFSCtor() \
  	FCFSdw = Allocator( sizeof(typeof(FCFSdw[0])) * N ); \
	for ( typeof(N) i = 0; i < N; i += 1 ) { \
		FCFSdw[i] = 0; \
	} \
  	FCFSturn = Allocator( sizeof(typeof(FCFSturn[0])) * 2 * N * FCFS_R ); \
	for ( typeof(N) i = 0; i < 2 * N * FCFS_R; i += 1 ) { \
		FCFSturn[i] = 0; \
	} \
	FCFSTestCtor()
#define FCFSDtor() \
	FCFSTestDtor(); \
	free( (void *)FCFSturn ); \
	free( (void *)FCFSdw )

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
