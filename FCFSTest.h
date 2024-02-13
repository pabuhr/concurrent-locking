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
#define FCFSTestExit() \
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
		free( (void *)FCFSTestPredec[p] ); \
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
