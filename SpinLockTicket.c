// http://pdos.csail.mit.edu/papers/linux:lock.pdf

typedef struct {
	TYPE ticket
#if defined( __i386 ) || defined( __x86_64 )
		__attribute__(( aligned (128) ));				// Intel recommendation
#elif defined( __sparc )
		CALIGN;
#else
    #error unsupported architecture
#endif

	TYPE serving
#if defined( __i386 ) || defined( __x86_64 )
		__attribute__(( aligned (128) ));				// Intel recommendation
#elif defined( __sparc )
		CALIGN;
#else
    #error unsupported architecture
#endif
} Lock;

static Lock lock __attribute__(( aligned (128) ));		// Intel recommendation
static TYPE PAD CALIGN __attribute__(( unused ));		// protect further false sharing

#ifdef PASS
TYPE
#else
void
#endif // PASS
spin_lock( volatile Lock * lock ) {
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
		volatile Lock * lock ) {
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
		entry = 0;
		while ( stop == 0 ) {
#ifdef PASS
			TYPE ticket =
#endif // PASS
			spin_lock( &lock );
			CriticalSection( id );
			spin_unlock(
#ifdef PASS
				ticket,
#endif // PASS
				&lock );
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			entry += 1;
		} // while
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

void ctor() {
	lock.ticket = lock.serving = 0;
} // ctor

void dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=SpinLockTicket Harness.c -lpthread -lm" //
// End: //
