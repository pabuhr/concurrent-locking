// Correia and Ramalhete CRTurn, Mutual Exclusion - Two linear wait software solutions
// https://github.com/pramalhe/ConcurrencyFreaks/tree/master/papers/cralgorithm-2015.pdf
//
// Shared words      = N+1
// Number of states  = 3
// Starvation-Free   = yes, with N
// Minimum SC stores = 1 + 1
// Minimum SC loads  = N+2

enum Intent { UNLOCKED, WAITING, LOCKED };

static volatile TYPE *states CALIGN;					// shared
static volatile TYPE turn CALIGN;                       // shared

inline static const int validate_left(const int id, const int lturn) {
    int i;
    if (lturn > id) {
        for (i = lturn; i < N; i++) {
            if (states[i] != UNLOCKED) return 0;
        }
        for (i = 0; i < id; i++) {
            if (states[i] != UNLOCKED) return 0;
        }
    } else {
        for (i = lturn; i < id; i++) {
            if (states[i] != UNLOCKED) return 0;
        }
    }
    return 1;
}

inline static const int validate_right(const int id, const int lturn) {
    int i;
    if (lturn <= id) {
        for (i = id + 1; i < N; i++) {
            if (states[i] == LOCKED) return 0;
        }
        for (i = 0; i < lturn; i++) {
            if (states[i] == LOCKED) return 0;
        }
    } else {
        for (i = id + 1; i < lturn; i++) {
            if (states[i] == LOCKED) return 0;
        }
    }
    return 1;
}

static void *Worker( void *arg ) {
    TYPE id = (size_t)arg;
	uint64_t entry;
#ifdef FAST
	unsigned int cnt = 0, oid = id;
#endif // FAST

    for ( int r = 0; r < RUNS; r += 1 ) {
        entry = 0;
        while ( stop == 0 ) {
            states[id] = LOCKED;
            Fence();
            while (1) {
                TYPE lturn = turn;
                if (!validate_left(id, lturn)) {
                    states[id] = WAITING;
                    Fence();
                    while (1) {
                        if (validate_left(id, lturn) && lturn == turn) break;
                        Pause();
                        lturn = turn;
                    }
                    states[id] = LOCKED;
                    Fence();
                    continue;
                }
                while (lturn == turn) {
                    if (validate_right(id, lturn)) break;
                    Pause();
                }
                if (lturn == turn) break;
            }
			CriticalSection( id );						// critical section
			turn = (turn+1) % N;
			states[id] = UNLOCKED;					// exit protocol
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
	states = Allocator( sizeof(typeof(states[0])) * N );
	for ( int i = 0; i < N; i += 1 ) {					// initialize shared data
		states[i] = UNLOCKED;
	} // for
	turn = 0;
} // ctor

void dtor() {
	free( (void *)states );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=CorreiaRamalheteTurn Harness.c -lpthread -lm" //
// End: //
