// http://pdos.csail.mit.edu/papers/linux:lock.pdf

typedef struct {
	// 32 bit sufficient and can reduce 1 clock latency in some cases.
	volatile uint32_t ticket CALIGN;
	volatile uint32_t serving CALIGN;
} Lock;

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static Lock lock;										// fields cache aligned in node
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#ifdef PASS
TYPE
#else
void
#endif // PASS
spin_lock( Lock * lock ) {
	TYPE myticket = __sync_fetch_and_add( &lock->ticket, 1 );
	while ( myticket != lock->serving ) Pause();
#ifdef PASS
	return myticket;
#endif // PASS
} // spin_lock

void spin_unlock(
#ifdef PASS
		const TYPE ticket,
#endif // PASS
		Lock * lock ) {
#ifdef PASS
	lock->serving = ticket + 1;
#else
//	__sync_fetch_and_add( &lock->serving, 1 );
	lock->serving += 1;
#endif // PASS
} // spin_unlock

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;

	#ifdef FAST
	unsigned int cnt = 0, oid = id;
	#endif // FAST

	for ( int r = 0; r < RUNS; r += 1 ) {
		uint32_t randomThreadChecksum = 0;

		for ( entry = 0; stop == 0; entry += 1 ) {
			#ifdef PASS
			TYPE ticket =
			#endif // PASS
			spin_lock( &lock );

			randomThreadChecksum += CriticalSection( id );

			spin_unlock(
				#ifdef PASS
				ticket,
				#endif // PASS
				&lock );

			#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
			#endif // FAST
		} // for

		__sync_fetch_and_add( &sumOfThreadChecksums, randomThreadChecksum );

		#ifdef FAST
		id = oid;
		#endif // FAST
		entries[r][id] = entry;
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for

	return NULL;
} // Worker

void __attribute__((noinline)) ctor() {
	lock.ticket = lock.serving = 0;
} // ctor

void __attribute__((noinline)) dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -Wextra -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=SpinLockTicket Harness.c -lpthread -lm -D`hostname` -DCFMT -DCNT=0" //
// End: //
