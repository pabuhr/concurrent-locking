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
static volatile TYPE lock CALIGN;						// no reason to make ATOMIC(TYPE)
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#ifndef NOEXPBACK
enum { SPIN_START = 64, SPIN_END = 64 * 1024, };		// performance better with large SPIN_END
#endif // ! NOEXPBACK

void spin_lock( volatile TYPE * lock ) {				// no reason to make ATOMIC(TYPE)
	#ifndef NOEXPBACK
	TYPE spin = SPIN_START;
	#endif // ! NOEXPBACK

	for ( ;; ) {
		// For whatever reason, Casw often out performances Tas
	  // if ( *lock == 0 && Tas( lock ) == 0 ) break;		// Fence
	  if ( *lock == 0 && Casw( lock, (typeof(lock))0, (typeof(lock))1 ) ) break; // Fence

		#ifndef NOEXPBACK
		for ( TYPE s = 0; s < spin; s += 1 ) Pause();	// exponential spin
		spin += spin;									// powers of 2
		// if ( i % 64 == 0 ) spin += spin;				// slowly increase by powers of 2
		// if ( spin > SPIN_END ) spin = SPIN_END;			// cap spinning length
		if ( spin > SPIN_END ) spin = SPIN_START;		// restart (randomize) spinning length
		#else

		#ifndef MPAUSE
		Pause();
		#else
		MPause( lock, == 0 );							// busy wait on my spin variable
		#endif // MPAUSE
		#endif // ! NOEXPBACK
	} // for
} // spin_lock

void spin_unlock( VTYPE * lock ) {
	Clr( lock );										// Fence
} // spin_unlock

// Local Variables: //
// tab-width: 4 //
// End: //
