// John M. Mellor-Crummey and Michael L. Scott, Algorithm for Scalable Synchronization on Shared-Memory Multiprocessors,
// ACM Transactions on Computer Systems, 9(1), 1991, Fig. 6, p. 30

typedef struct mcs_node MCS_node;						// remove redundant struct qualifier
typedef struct mcs_node {
	MCS_node * volatile next;
	volatile TYPE spin;
} * MCS_lock;

void mcs_lock( MCS_lock * lock, MCS_node * node ) {
	MCS_node * pred;
	node->next = NULL;
#if defined( __sparc )
	pred = SWAP32( lock, node );						// fetch-and-store
#else
	pred = __sync_lock_test_and_set( lock, node );		// fetch-and-store
#endif
	if ( FASTPATH( pred != NULL ) ) {					// someone on list ?
		node->spin = 1;									// mark as waiting
		pred->next = node;								// add to list of waiting threads
		while ( node->spin == 1 ) Pause();				// busy wait on my spin variable
	} // if
} // mcs_lock

void mcs_unlock( MCS_lock * lock, MCS_node * node ) {
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

static MCS_lock lock CALIGN;
static TYPE PAD CALIGN __attribute__(( unused ));		// protect further false sharing

static void * Worker( void * arg ) {
	TYPE id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

	MCS_node node CALIGN;

	for ( int r = 0; r < RUNS; r += 1 ) {
		entry = 0;
		while ( stop == 0 ) {
			mcs_lock( &lock, &node );
			CriticalSection( id );
			mcs_unlock( &lock, &node );
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
	lock = NULL;
} // ctor

void dtor() {
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=MCS Harness.c -lpthread -lm" //
// End: //
