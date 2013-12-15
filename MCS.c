// John M. Mellor-Crummey and Michael L. Scott, Algorithm for Scalable Synchronization on Shared-Memory Multiprocessors,
// ACM Transactions on Computer Systems, 9(1), 1991, Fig. 6, p. 30

#if defined( __sparc )
// gcc atomic-intrinsic inserts unnecessary MEMBAR
static inline void *SWAP32( volatile void *ptr, void *v ) {
	__asm__ __volatile ("swap [%[ADR]],%[RV]"
						: [RV] "+r" (v)
						: [ADR] "r" (ptr)
						: "memory" );
	return v;
}

#define CAS32( ptr, cmp, set )					\
	({  typeof(set) tt = (set);					\
		__asm__ __volatile__ (					\
			"cas [%2],%3,%0"					\
			: "=&r" (tt)						\
			: "0" (tt), "r" (ptr), "r" (cmp)	\
			: "memory" );						\
		tt ;									\
	})
#endif

typedef struct mcs_node MCS_node;
typedef struct mcs_node {
	MCS_node *volatile next;
	volatile TYPE spin;
} *MCS_lock;

void mcs_lock( MCS_lock *lock, MCS_node *node ) {
	MCS_node *pred;
	node->next = NULL;
#if defined( __sparc )
	pred = SWAP32( lock, node );						// fetch-and-store
#else
	pred = __sync_lock_test_and_set( lock, node );		// fetch-and-store
#endif
	if ( pred != NULL ) {								// someone on list ?
		node->spin = 1;									// mark as waiting
		pred->next = node;								// add to list of waiting threads
		while ( node->spin == 1 ) Pause();				// busy wait on my spin variable
	} // if
} // mcs_lock

void mcs_unlock( MCS_lock *lock, MCS_node *node ) {
#if defined( __sparc )
	__asm__ __volatile (";" ::: "memory");				// gcc problem with code movement on SPARC
#endif
	if ( node->next == NULL ) {							// no one waiting ?
#if defined( __sparc )
  if ( (void *)CAS32( lock, node, NULL ) == node ) return; // not changed since last looked ?
#else
  if ( __sync_bool_compare_and_swap( lock, node, NULL ) ) return; // not changed since last looked ?
#endif
		while ( node->next == NULL ) Pause();			// busy wait until my node is modified
	} // if
	node->next->spin = 0;								// stop their busy wait
} // mcs_unlock

MCS_lock lock CALIGN;

static void *Worker( void *arg ) {
	unsigned int id = (size_t)arg;
	MCS_node node CALIGN;
#ifdef FAST
	unsigned int cnt = 0;
#endif // FAST
	size_t entries[RUNS];

	for ( int r = 0; r < RUNS; r += 1 ) {
		entries[r] = 0;
		while ( stop == 0 ) {
#ifdef FAST
			id = startpoint( cnt );						// different starting point each experiment
			cnt = cycleUp( cnt, NoStartPoints );
#endif // FAST
			mcs_lock( &lock, &node );
			CriticalSection( id );
			mcs_unlock( &lock, &node );
			entries[r] += 1;
		} // while
		__sync_fetch_and_add( &Arrived, 1 );
		while ( stop != 0 ) Pause();
		__sync_fetch_and_add( &Arrived, -1 );
	} // for
	qsort( entries, RUNS, sizeof(size_t), compare );
	return (void *)median(entries);
} // Worker

void ctor() {
	lock = NULL;
} // ctor

void dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DAlgorithm=MCS Harness.c -lpthread -lm" //
// End: //
