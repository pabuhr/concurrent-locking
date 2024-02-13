// Alex A. Aravind, Simple, Space-Efficient, and Fairness Improved FCFS Mutual Exclusion Algorithms, J. Parallel
// Distrib. Comput. 73 (2013), Fig. 4

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
	static VTYPE * FCFS_D CALIGN; \
	static ATOM2 FCFS_T CALIGN;
#define FCFSLocal() TYPE FCFSb CALIGN = 0; Tuple FCFS_S[N] CALIGN
#define FCFSEnter() \
	FCFSTestIntroTop(); \
	FCFS_D[id] = true; \
	Fence(); \
	for ( typeof(N) j = 0; j < N; j += 1 ) FCFS_S[j] = FCFS_T[j]; \
	FCFS_T[id].bits[FCFSb] = true; \
	FCFS_D[id] = false; \
	Fence(); \
	FCFSTestIntroMid(); \
	for ( typeof(N) j = 0; j < N; j += 1 ) \
		if ( FCFS_S[j].atom != 0 ) \
			await( FCFS_S[j].atom != FCFS_T[j].atom )
#define FCFSExitAcq() \
	FCFSTestIntroBot(); \
	FCFS_T[id].bits[FCFSb] = false; \
	Fence(); \
	for ( typeof(N) j = 0; j < N; j += 1 ) \
		await( FCFS_D[j] == false )
#define FCFSExitRel() \
	FCFSb = ! FCFSb
#define FCFSCtor() \
  	FCFS_D = Allocator( sizeof(typeof(FCFS_D[0])) * N ); \
  	FCFS_T = Allocator( sizeof(typeof(FCFS_T[0])) * N ); \
	for ( typeof(N) i = 0; i < N; i += 1 ) { \
		FCFS_D[i] = 0; \
		FCFS_T[i].atom = 0; \
	} \
	FCFSTestCtor()
#define FCFSDtor() \
	FCFSTestDtor(); \
	free( (void *)FCFS_D ); \
	free( (void *)FCFS_T )

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
