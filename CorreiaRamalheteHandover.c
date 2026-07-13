// Correia and Ramalhete CRHandover, Mutual Exclusion - Two linear wait software solutions
// https://github.com/pramalhe/ConcurrencyFreaks/tree/master/papers/cralgorithm-2015.pdf
//
// Shared words      = 2N+1
// Number of states  = 3 + 2
// Starvation-Free   = yes, with N
// Minimum SC stores = 1 + 1
// Minimum SC loads  = 2 + 2

enum Intent { UNLOCKED, WAITING, LOCKED };
enum Handover { NOTMINE, MYTURN };

static volatile TYPE *states CALIGN;                    // shared
static volatile TYPE *handover CALIGN;                  // shared
static volatile TYPE hoEnabled CALIGN;                  // shared

inline static const int validate_left(const int id) {
    for (int i = 0; i < id; i++) {
        if (states[i] != UNLOCKED) return 0;
    }
    return 1;
}

inline static const int validate_right(const int id) {
    for (int i = id + 1; i < N; i++) {
        if (states[i] == LOCKED) return 0;
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
            int i, isFirstTime = 1;
            while (1) {
                while (hoEnabled) {
                    if (isFirstTime) { states[id] = WAITING; Fence(); isFirstTime = 0; }
                    if (handover[id] == MYTURN) {
                        handover[id] = NOTMINE;
                        states[id] = LOCKED;
                        goto LCS; // fast-path under high contention
                    }
                    Pause();
                }
                isFirstTime = 0;
                states[id] = LOCKED;
                Fence();
                for (i = 0; i < id; i++) {
                    if (states[i] != UNLOCKED) break;
                }
                if (i != id) {
                    states[id] = WAITING;
                    Fence();
                    while (!hoEnabled) {
                        for (i = 0; i < id; i++) {
                            if (states[i] != UNLOCKED) break;
                        }
                        if (i == id) break;
                        Pause();
                    }
                    continue;
                }
                while (!hoEnabled) {
                    for (i = id + 1; i < N; i++) {
                        if (states[i] == LOCKED) break;
                    }
                    if (i == N && !hoEnabled) goto LCS;
                }
                states[id] = WAITING;
                Fence();
            }
          LCS: CriticalSection( id );                      // critical section
            for (int i = id + 1; i < N; i++) {
                if (states[i] == WAITING) {
                    if (!hoEnabled) hoEnabled = 1;
                    handover[i] = MYTURN;
                    states[id] = UNLOCKED;
                    goto LEND;
                }
            }
            for (int i = 0; i < id; i++) {
                if (states[i] == WAITING) {
                    if (!hoEnabled) hoEnabled = 1;
                    handover[i] = MYTURN;
                    states[id] = UNLOCKED;
                    goto LEND;
                }
            }
            // There are no successors at all
            if (hoEnabled) hoEnabled = 0;
            states[id] = UNLOCKED;
          LEND:
#ifdef FAST
            id = startpoint( cnt );                     // different starting point each experiment
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
    handover = Allocator( sizeof(typeof(handover[0])) * N );
    for ( int i = 0; i < N; i += 1 ) {                  // initialize shared data
        states[i] = UNLOCKED;
        handover[i] = NOTMINE;
    } // for
    hoEnabled = 0; // false
} // ctor

void dtor() {
    free( (void *)states );
    free( (void *)handover );
} // dtor

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=CorreiaRamalheteHandover Harness.c -lpthread -lm" //
// End: //
