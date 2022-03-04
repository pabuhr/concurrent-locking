// Waiman Long <waiman.long@hp.com>, qspinlock: Introducing a 4-byte queue spinlock implementation,
// https://lwn.net/Articles/561775/
//
// This qspinlock is kernel specific. First, the MCS nodes are not actually on-stack, but reside in a static global 2D
// array indexed by CPUID and trap level.  There are 4 trap levels : normal; device interrupt; panic and NMI.  The
// latter 2 are almost never used.  Ignoring the trap levels and assuming a flat 1D array of MCS nodes indexed by the
// CPUID, the only time a thread needs to use its MCS node is for waiting.  A thread might hold lots of spin locks but
// it can only wait on at most one at a time, so having one MCS node per CPU suffices.  Relatedly, to acquire a
// qspinlock preemption must be disabled - scheduler+timer interrupts are disabled.  And code in a qspinlock critical
// section is not allowed to perform a voluntary context switch.  Those are all hard and fixed rules.  So while a thread
// is spinning or holds a qspinlock, the kernel can not switch to another thread, and the thread:CPU relation is stable
// for that period.  Thus, having one MCS node per CPU, while seemingly very odd, is OK in this particular use case.
//
// Also, for historical and legacy reasons (32-bit era, with migration from test-and-set and ticket locks, both of which
// are small) the qspinlock_t size needed to be 32-bits, even on a 64-bit system.  There are lots of structures that are
// tightly packed and cache aligned and would double in size if qspinlock_t was switched from 32-bits to 64-bits.  The
// page_t structure, for instance, consumes one cache line for each 4K page on an x86 system.  It is a non-trivial "tax"
// on physical memory to keep the page_t structures.  On x86, the lose is 1.5% of all _physical memory to the page_t
// array.  The page_t structure is already filled with strange encodings and bit packing tracks to keep the size one
// cache line.  And it contains 3 qspinlock_t fields.  There are many other examples, but the page_t one is compelling.
// Anyway, to keep qspinlock_t at 32-bits, instead of containing a pointer to a tail element, it contains the CPUID
// index of the tail element in the MCS node array.

// From Dave Dice and Alex Kogan "TWA - Ticket Locks Augmented with a Waiting Array", page 9,
// https://arxiv.org/pdf/1810.01573.pdf.
//
// The Linux qspinlock construct [11, 12, 31] is a compact 32-bit lock, even on 64-bit architectures. The low-order bits
// of the lock word constitute a simple test-and-set lock while the upper bits encode the tail of an MCS chain. The
// result is a hybrid of MCS and test-and-set. In order to fit into a 32-bit work - a critical requirement - the chain
// is formed by logical CPU identifiers instead of traditional MCS queue node pointers. Arriving threads attempt to
// acquire the test-and-set lock embedded in the low order bits of the lock word. This attempt fails if the test-and-set
// lock is held or of the MCS chain is populated. If successful, they enter the critical section, otherwise they join
// the MCS chain embedded in the upper bits of the lock word. When a thread becomes an owner of the MCS lock, it can
// wait for the test-and-set lock to become clear, at which point it claims the test-and-set lock, releases the MCS
// lock, and then enters the critical section. The MCS aspect of qspinlock is used only when there is contention. The
// unlock operator simply clears the test-and-set lock. The MCS lock is never held over the critical section, but only
// during contended acquisition. Only the owner of the MCS lock spins on the test-and-set lock, reducing coherence
// traffic. Qspinlock is strictly FIFO. While the technique employs local spinning on the MCS chain, unlike traditional
// MCS, arriving and departing threads will both update the common lock word, increasing coherence traffic and degrading
// performance relative to classic MCS. Qspinlock incorporates an additional optimization where the first contending
// thread spins on the test-and-set lock instead of using the MCS path. Traditional MCS does not fit well in the Linux
// kernel as (a) the constraint that a low-level spin lock instance be only 32-bits is a firm requirement, and (b) the
// lock-unlock API does not provide a convenient way to pass the owner's MCS queue node address from lock to unlock. We
// note that qspinlocks replaced classic ticket locks as the kernel's primary low-level spin lock mechanism in 2014, and
// ticket locks replaced test-and-set locks, which are unfair and allow unbounded bypass, in 2008 [13].

// The following is general and does not encode the flag field.

#include "MCS.h"

typedef struct {
	MCS_lock mcs_lock;
	volatile TYPE flag CALIGN;
} QSLock;

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
