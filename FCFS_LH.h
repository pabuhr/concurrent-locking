// Edward A. Lycklama and Vassos Hadzilacos, A First-Come-First-Served Mutual-Exclusion Algorithm with Small
// Communication Variables, ACM Transactions on Programming Lnaguages and Systems, 13(4), Oct. 1991, Fig. 4.

#include "FCFSTest.h"

#ifdef FCFS
#ifndef ATOMIC
#define ATOM1 volatile WHOLESIZE
#define ATOM2 volatile Tuple *
#else
#define ATOM1 _Atomic(WHOLESIZE)
#define ATOM2 _Atomic(Tuple *)
#endif // ! ATOMIC

#define FCFSGlobal() \
	FCFSTestGlobal(); \
	typedef union CALIGN { \
		struct { \
			HALFSIZE bits[2]; \
		}; \
		ATOM1 atom; \
	} Tuple; \
	static VTYPE * FCFSc CALIGN, * FCFSv CALIGN; \
	static ATOM2 FCFSturn CALIGN;
#define FCFSLocal() TYPE FCFSbit CALIGN = 0; Tuple FCFS_S[N] CALIGN
#define FCFSEnter() \
	FCFSTestIntroTop(); \
	FCFSc[id] = true; \
	Fence(); \
	for ( typeof(N) j = 0; j < N; j += 1 ) FCFS_S[j] = FCFSturn[j]; \
	FCFSbit = ! FCFSbit; \
	FCFSturn[id].bits[FCFSbit] = ! FCFSturn[id].bits[FCFSbit]; \
	FCFSv[id] = true; \
	FCFSc[id] = false; \
	Fence(); \
	FCFSTestIntroMid(); \
	for ( typeof(N) j = 0; j < N; j += 1 ) \
		await( ! ( FCFSc[j] || ( FCFSv[j] && FCFS_S[j].atom == FCFSturn[j].atom ) ) );
#define FCFSExitAcq() \
	FCFSTestIntroBot()
#define FCFSExitRel() \
	FCFSv[id] = false
#define FCFSCtor() \
  	FCFSc = Allocator( sizeof(typeof(FCFSc[0])) * N ); \
  	FCFSv = Allocator( sizeof(typeof(FCFSv[0])) * N ); \
  	FCFSturn = Allocator( sizeof(typeof(FCFSturn[0])) * N ); \
	for ( typeof(N) i = 0; i < N; i += 1 ) { \
		FCFSc[i] = FCFSv[i] = 0; \
		FCFSturn[i].atom = 0; \
	} \
	FCFSTestCtor()
#define FCFSDtor() \
	FCFSTestDtor(); \
	free( (void *)FCFSturn ); \
	free( (void *)FCFSv ); \
	free( (void *)FCFSc )

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
