// On Intel, spinlock performance is constant independent of contention. The reason is that the hardware is running the
// threads almost sequentially. The issue is spinlock unfairness, which is usually attributed to cache line arbitration
// and the fact that the current owner has a better chance of reacquiring when the non-critical section is short.  On
// NUMA systems there are also some performance effects visible related to the placement of the lock (it's home node) vs
// the node of threads trying to get the lock.  In some similar experiments, both long-term and show-term fairness for
// test-and-test-and-set spin locks occurs.  There are modes where one, or perhaps two, threads are dominating ownership
// for non-trivial periods that can span 1000s of acquisitions.  Put another way, the long-term fairness as reported via
// relative std deviation is bad, but the short-term is likely far worse.  There is not much in the literature regarding
// measures of shorter term fairness for locks.  One approach is to have the critical section record the owner thread ID
// into an append only log, but there is a large probe effect here.  Post-processing the log and reporting the "median
// time to reacquire", where time is given in lock acquisition counts instead of normal time units, should give
// interesting information.

#pragma once

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
#ifndef ATOMIC
static VTYPE lock CALIGN;
#else
_Atomic(TYPE) lock CALIGN;
#endif // ! ATOMIC
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

void spin_lock( VTYPE * lock ) {
	#ifndef NOEXPBACK
	enum { SPIN_START = 4, SPIN_END = 64 * 1024, };
	unsigned int spin = SPIN_START;
	#endif // ! NOEXPBACK

	for ( unsigned int i = 1;; i += 1 ) {
	  if ( *lock == 0 && Tas( lock ) == 0 ) break;		// Fence
		#ifndef NOEXPBACK
		for ( TYPE s = 0; s < spin; s += 1 ) Pause();	// exponential spin
		spin += spin;									// powers of 2
		// if ( i % 64 == 0 ) spin += spin;				// slowly increase by powers of 2
		if ( spin > SPIN_END ) spin = SPIN_END;			// cap spinning length
		#else
		Pause();
		#endif // ! NOEXPBACK
	} // for
	WO( Fence(); );
} // spin_lock

void spin_unlock( VTYPE * lock ) {
	WO( Fence(); );
	Clr( lock );										// Fence
} // spin_unlock

// Local Variables: //
// tab-width: 4 //
// End: //
