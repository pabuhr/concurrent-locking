// Macros to bracket a non-FCFS lock acquire to make it FCFS, e.g.:
//      FCFSintro(); acquire(m); FCFSexit();

#ifdef FCFSTest
#include "SpinLock.h"
#define FCFSTestGlobal() \
	static VTYPE ** FCFSTestPredec CALIGN, * FCFSTestInside; \
	static VTYPE FCFSLock CALIGN
#define FCFSTestIntroTop() \
	spin_lock( &FCFSLock ); \
	for ( typeof(N) k = 0; k < N; k += 1 ) FCFSTestPredec[id][k] = FCFSTestInside[k]; \
	spin_unlock( &FCFSLock )
#define FCFSTestIntroMid() \
	spin_lock( &FCFSLock ); \
	FCFSTestInside[id] = 1; \
	spin_unlock( &FCFSLock )
#define FCFSTestIntroBot()	\
	spin_lock( &FCFSLock ); \
	for ( typeof(N) k = 0; k < N; k += 1 ) if ( FCFSTestPredec[id][k] ) { printf( "FCFS failure:%zu\n", id ); exit( EXIT_FAILURE ); } \
	spin_unlock( &FCFSLock )
#define FCFSTestExit()		\
	spin_lock( &FCFSLock ); \
	FCFSTestInside[id] = 0; \
	for ( typeof(N) k = 0; k < N; k += 1 ) FCFSTestPredec[k][id] = 0; \
	spin_unlock( &FCFSLock )
#define FCFSTestCtor() \
  	FCFSTestPredec = Allocator( sizeof(typeof(FCFSTestPredec[0])) * N ); \
  	FCFSTestInside = Allocator( sizeof(typeof(FCFSTestInside[0])) * N ); \
	for ( typeof(N) p = 0; p < N; p += 1 ) { \
		FCFSTestInside[p] = 0; \
		FCFSTestPredec[p] = Allocator( sizeof(typeof(FCFSTestPredec[0][0])) * N ); \
		for ( typeof(N) k = 0; k < N; k += 1 ) { \
			FCFSTestPredec[p][k] = 0; \
		} \
	}
#define FCFSTestDtor() \
  	free( (void *)FCFSTestInside ); \
	for ( typeof(N) p = 0; p < N; p += 1 ) { \
		free( (void *)FCFSTestPredec[p] );	 \
	} \
  	free( (void *)FCFSTestPredec );

#else

#define FCFSTestGlobal() int __attribute__(( unused )) FCFSTestGlobal
#define FCFSTestIntroTop() do {} while(0)
#define FCFSTestIntroMid() do {} while(0)
#define FCFSTestIntroBot() do {} while(0)
#define FCFSTestExit() do {} while(0)
#define FCFSTestCtor() do {} while(0)
#define FCFSTestDtor() do {} while(0)
#endif // FCFSTest


#define odd( v ) (v & 1)
#ifdef FCFS
#define FCFSGlobal() \
	static VTYPE * FCFSturn CALIGN; \
	FCFSTestGlobal();
#define FCFSLocal() TYPE FCFScopy[N], FCFSnx
#define FCFSEnter() \
	FCFSTestIntroTop(); \
	FCFSnx = FCFSturn[id] + 1; \
	FCFSturn[id] = -1; \
	Fence(); \
	for ( typeof(N) k = 0; k < N; k += 1 ) FCFScopy[k] = FCFSturn[k]; \
	WO( Fence(); ); \
	FCFSturn[id] = FCFSnx; \
	Fence(); \
	FCFSTestIntroMid(); \
	for ( typeof(N) k = 0; k < N; k += 1 ) \
		if ( odd( FCFScopy[k] ) ) await( FCFSturn[k] != FCFScopy[k] ); \
	FCFSTestIntroBot()
#define FCFSExit() \
	FCFSTestExit(); \
	WO( Fence(); ); \
	/* FCFSturn[id] = ((FCFSturn[id] + 1) % 6); */ \
	/* FCFSturn[id] = (7 & (FCFSturn[id] + 1)); */ \
	TYPE FCFStemp = FCFSturn[id]; \
	FCFSturn[id] = (FCFStemp == 5 ? 0 : FCFStemp + 1 );
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
#define FCFSExit() do {} while(0)
#define FCFSCtor() do {} while(0)
#define FCFSDtor() do {} while(0)
#endif // FCFS

// Local Variables: //
// tab-width: 4 //
// End: //
