// Waiman Long <waiman.long@hp.com>, qspinlock: Introducing a 4-byte queue spinlock implementation, https://lwn.net/Articles/561775/

// The following does not encode the flag field.

#include "MCS.h"

inline void acquire( QSLock * lock ) {
	MCS_node node;
	mcs_lock( &lock->mcs_lock, &node );
	await( lock->flag == 0 );
	lock->flag = 1;
	mcs_unlock( &lock->mcs_lock, &node );
} // acquire
	
inline void release( QSLock * lock ) {
	lock->flag = 0;
} // release

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static QSLock qslock CALIGN;
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		RTYPE randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			acquire( &qslock );

			randomThreadChecksum += CriticalSection( id );

			release( &qslock );

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		Fai( &sumOfThreadChecksums, randomThreadChecksum );

		#ifdef FAST
		id = oid;
		#endif // FAST
		entries[r][id] = entry;
		Fai( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		Fai( &Arrived, -1 );
	} // for

	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	qslock = (QSLock){ NULL, 0 };
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=SpinLockQ Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
