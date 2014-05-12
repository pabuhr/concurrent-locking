// The test harness creates N pthread worker-threads, and then blocks for a fixed period of time, T, after which a
// global stop-flag is set to indicate an experiment is over.  The N threads repeatedly attempt entry into a
// self-checking critical-section until the stop flag is set.  During the T seconds, each thread counts the number of
// times it enters the critical section. The higher the aggregate count, the better an algorithm, as it is able to
// process more requests for the critical section per unit time.  When the stop flag is set, a worker thread stops
// entering the critical section, and atomically adds it subtotal entry-counter to a global total entry-counter. When
// the driver unblocks after T seconds, it busy waits until all threads have noticed the stop flag and added their
// subtotal to the global counter, which is then stored.  Five identical experiments are performed, each lasting T
// seconds. The median value of the five results is printed.

#define _GNU_SOURCE										// See feature_test_macros(7)
#include <sched.h>
#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>										// abort, exit, atoi, ran, qsort
#include <stdint.h>										// intptr_t, INTPTR_MAX
#include <sys/time.h>
#include <poll.h>										// poll
#include <malloc.h>										// memalign
#include <unistd.h>										// getpid
#include <math.h>										// sqrt
#include <assert.h>

#if defined( __i386 ) || defined( __x86_64 )
#ifdef FAST
// unlikely
#define FASTPATH(x) __builtin_expect(!!(x), 0)
#else
// likely
#define FASTPATH(x) __builtin_expect(!!(x), 1)
#endif // FASTPATH

#if BRANCH == 'L'
#define likely(x) __builtin_expect(!!(x), 1)
#elif BRANCH == 'U'
#define likely(x) __builtin_expect(!!(x), 0)
#elif BRANCH == 'N'
#define likely(x) (x)
#else // default is neutral
#define likely(x) (x)
#endif // BRANCH

#else
#define FASTPATH(x) (x)
#endif // __i386 || __x86_64

typedef intptr_t TYPE;									// atomically addressable word-size

#define CACHE_ALIGN 128									// Intel recommendation
#define CALIGN __attribute__(( aligned (CACHE_ALIGN) ))

enum { RUNS = 5 };
static volatile int stop = 0;

// pause to prevent excess processor bus usage
#if defined( __sparc )
#define Pause() __asm__ volatile( "rd %ccr,%g0" )
#elif defined( __i386 ) || defined( __x86_64 )
#define Pause() __asm__ volatile( "pause" : : : )
#else
#error unsupported architecture
#endif

// Architectural ST-LD barrier -- memory fences
// In theory these should be obviated by the C++11 std::atomic_thread_fence() primitives.  Another option is the gcc
// _sync* primitives, but those are deprecated in favor of the new C++11 operators.  Unfortunately the current
// implementations of both __sync and C++11 atomic operators is sub-optimal.  On x86 these facilities use MFENCE, which
// is a poor choice.  On SPARC these facilities conservatively assume RMO instead of TSO.  Furthermore atomic
// instructions such as CAS/LDSTUB/SWAP are _not assumed to have fence semantics.  In principle that's prudent as,
// strictly speaking, the architecture manuals clearly state that atomics may not have barrier semantics.  But in
// practice because of "impl-dep" specifications, atomics have full bidirectional fence semantics on all SPARC
// processors.  Solaris, the JVM, etc., all assume TSO where atomics have full fence semantics.

#if defined(__sparc)
#define Fence() __asm__ __volatile__ ("membar #StoreLoad;")
// On x86, use either MFENCE; LOCK:ADDN 0,[SP]; or XCHGN to implement Fence().  See
// https://blogs.oracle.com/dave/entry/instruction_selection_for_volatile_fences We could be slightly more clever and
// merge the ST and FENCE into a single STFenced(Location,Value) primitive which is implemented via XCHG.  On SPARC
// STFenced() is implemented via ST;MEMBAR.
#elif defined(__x86_64) 
//#define Fence() __asm__ __volatile__ ("mfence")
#define Fence() __asm__ __volatile__ ("lock; addq $0,(%%rsp);" ::: "cc")
#elif defined(__i386)
#define Fence() __asm__ __volatile__ ("lock; addl $0,(%%esp);" ::: "cc")
#else
#error unsupported architecture
#endif

// memory allocator to align or not align storage
#if defined( __sparc )
//#define Allocator( size ) malloc( (size) )
#define Allocator( size ) memalign( CACHE_ALIGN, (size) )
#elif defined( __i386 ) || defined( __x86_64 )
#define Allocator( size ) memalign( CACHE_ALIGN, (size) )
#else
#error unsupported architecture
#endif

#if defined( __GNUC__ )									// GNU gcc compiler ?
// polymorphic integer log2, using clz, which returns the number of leading 0-bits,
// starting at the most significant bit (single instruction on x86)
#define Log2( n ) ( sizeof(n) * __CHAR_BIT__ - 1 - (					\
						( sizeof(n) ==  4 ) ? __builtin_clz( n ) :		\
						( sizeof(n) ==  8 ) ? __builtin_clzl( n ) :		\
						( sizeof(n) == 16 ) ? __builtin_clzll( n ) :	\
						-1 ) )
#else
static int Log2( int n ) {								// fallback integer log2( n )
	return n > 1 ? 1 + Log2( n / 2 ) : n == 1 ? 0 : -1;
}
#endif // __GNUC__

static inline int Clog2( int n ) {						// integer ceil( log2( n ) )
	if ( n <= 0 ) return -1;
	int ln = Log2( n );
	return ln + ( (n - (1 << ln)) != 0 );				// check for any 1 bits to the right of the most significant bit
}

static inline void CriticalSection( const unsigned int id ) {
	static volatile unsigned int CurrTid;				// shared, current thread id in critical section

	CurrTid = id;
	Fence();
	for ( int i = 1; i <= 100; i += 1 ) {				// delay
		if ( CurrTid != id ) {							// mutual exclusion violation ?
			printf( "Interference Id:%d\n", id );
			abort();
		} // if
	} // for
} // CriticalSection

static inline TYPE cycleUp( TYPE v, TYPE n ) { return ( ((v) >= (n - 1)) ? 0 : (v + 1) ); }
static inline TYPE cycleDown( TYPE v, TYPE n ) { return ( ((v) <= 0) ? (n - 1) : (v - 1) ); }

static volatile int Arrived = 0;
static int N, Threads, Time, Degree = -1;

#ifdef FAST
enum { MaxStartPoints = 64 };
static unsigned int NoStartPoints;
static unsigned int *Startpoints;

// To ensure the single thread exercises all aspects of an algorithm, it is assigned different start-points on each
// access to the critical section by randomly changing its thread id.  The randomness is accomplished using
// approximately 64 pseudo-random thread-ids, where 64 is divided by N to get R repetitions, e.g., for N = 5, R = 64 / 5
// = 12.  Each of the 12 repetition is filled with 5 random value in the range, 0..N-1, without replacement, e.g., 0 3 4
// 1 2.  There are no consecutive thread-ids within a repetition but there may be between repetition.  The thread cycles
// through this array of ids during an experiment.

void __attribute__((noinline)) startpoints() {
	Startpoints[0] = N;
	for ( unsigned int i = 0; i < NoStartPoints; i += N ) {
		for ( unsigned int j = i; j < i + N; j += 1 ) {
			unsigned int v;
		  L: v = rand() % N;
			unsigned int k;
			for ( k = i; k < j; k += 1 ) {
				if ( Startpoints[k] == v ) goto L;
			} // for
// Unknown performance drop caused by following assert, use -DNDEBUG for experiments
			assert( k < NoStartPoints );
			Startpoints[k] = v;
		} // for
	} // for
#if 0
    for ( unsigned int i = 0; i < NoStartPoints; i += 1 ) {
		printf( "%d ", Startpoints[i] );
    } // for
    printf( "\n" );
#endif // 0
} // startpoints

static inline unsigned int startpoint( unsigned int pos ) {
	return Startpoints[pos];
//	return rand() % N;
} // startpoint
#endif // FAST

// Vary concurrency level to help detect exclusion failure and progress-liveness bugs in lock algorithms and
// implementations.  In many cases lock bugs do not ever manifest in steady-state, so varying the concurrency level
// randomly every 10 msecs is usually able to perturb the system to "shake out" more lock bugs.
//
// All threads are explicitly and intentionally quiesced while changing concurrency levels to increase the frequency at
// which the lock shifts between contended and uncontended states.  Specifically, concurrency shifts from M to 0 to N
// instead of from M to N.
// 
// We expect "Threads" to remain stable - Should be a stationary field.  When the barrier is operating BVO VStress != 0,
// Threads serves as a bound on concurrency.  The actual concurrency level at any given time will be in [1,Threads].
// Arrive() respects the Halt flag.

#ifdef STRESSINTERVAL
static int StressInterval =	STRESSINTERVAL;				// 500 is good
static volatile int BarHalt = 0; 

static int __attribute__((noinline)) PollBarrier() {
  if ( BarHalt == 0 ) return 0; 
	// singleton barrier instance state
	static volatile int Ticket = 0;  
	static volatile int Grant  = 0; 
	static volatile int Gate   = 0; 
	static volatile int nrun   = 0; 
	static const int Verbose   = 1; 

	static int ConcurrencyLevel = 0; 

	// We have distinct non-overlapping arrival and draining/departure phases
	// Lead threads waits inside CS for quorum
	// Follower threads wait at entry to CS on ticket lock
	// XXX ASSERT (Threads > 0); 
	int t = __sync_fetch_and_add (&Ticket, 1); 
	while ( Grant != t ) Pause(); 

	if ( Gate == 0 ) { 
		// Wait for full quorum
		while ( (Ticket - t) != Threads ) Pause(); 
		// Compute new/next concurrency level - cohort
		// Consider biasing PRNG to favor 1 to more frequently alternate contended
		// and uncontended modes.  
		// Release a subset of the captured threads
		// We expect the formation of the subsets to be effectively random,
		// but if that's not the case we can use per-thread flags and explicitly
		// select a random subset for the next epoch. 
		if ( (rand() % 10) == 0 ) { 
			Gate = 1; 
		} else { 
			Gate = (rand() % Threads) + 1; 
		}
		ConcurrencyLevel = Gate; 
		if ( Verbose ) printf ("L%d", Gate); 
		// XXX ASSERT (Gate > 0); 
		// XXX ASSERT (BarHalt != 0); 
		BarHalt = 0; 
		nrun = 0; 
	} // if

	// Consider : shift Verbose printing to after incrementing Grant
	if ( Verbose ) { 
		int k = __sync_fetch_and_add( &nrun, 1 );
		if ( k == (ConcurrencyLevel-1) ) printf( "; " );
		if ( k >= ConcurrencyLevel ) printf( "?" );
	} // if

	Gate -= 1; 
	// Need ST-ST barrier here
	// Release ticket lock
	Grant += 1; 

	// Consider a randomized delay here ...
	return 0; 
} // PollBarrier
#endif // STRESSINTERVAL

static uint64_t **entries;								// holds CS entry results for each threads for all runs

#define xstr(s) str(s)
#define str(s) #s
#include xstr(Algorithm.c)

static void shuffle( unsigned int set[], const int size ) {
	unsigned int p1, p2, temp;

	for ( int i = 0; i < 200; i +=1 ) {					// shuffle array S times
		p1 = rand() % size;
		p2 = rand() % size;
		temp = set[p1];
		set[p1] = set[p2];
		set[p2] = temp;
	} // for
} // shuffle

#define median(a) ((RUNS & 1) == 0 ? (a[RUNS/2-1] + a[RUNS/2]) / 2 : a[RUNS/2] )
static int compare( const void *p1, const void *p2 ) {
	size_t i = *((size_t *)p1);
	size_t j = *((size_t *)p2);
	return i > j ? 1 : i < j ? -1 : 0;
}

int main( int argc, char *argv[] ) {
	N = 8;												// defaults
	Time = 10;											// seconds

	switch ( argc ) {
	  case 4:
		Degree = atoi( argv[3] );
		if ( Degree < 2 ) goto usage;
	  case 3:
		Time = atoi( argv[2] );
		N = atoi( argv[1] );
		if ( Time < 1 || N < 1 ) goto usage;
		break;
	  usage:
	  default:
		printf( "Usage: %s %d (number of threads) %d (time in seconds threads spend entering critical section) %d (Zhang D-ary)\n",
				argv[0], N, Time, Degree );
		exit( EXIT_FAILURE );
	} // switch

	printf( "%d %d ", N, Time );

#ifdef FAST
	assert( N <= MaxStartPoints );
	Threads = 1;										// fast test, Threads=1, N=1..32
	NoStartPoints = MaxStartPoints / N * N;				// floor( MaxStartPoints / N )
	Startpoints = Allocator( sizeof(unsigned int) * NoStartPoints );
	startpoints( N );
#else
	Threads = N;										// allow testing of T < N
	//N = 32;
#endif // FAST
	entries = (uint64_t **)malloc( RUNS * sizeof(uint64_t *) );
	for ( int r = 0; r < RUNS; r += 1 ) {
		entries[r] = Allocator( Threads * sizeof(uint64_t) );
	} // for

	unsigned int set[Threads];
	for ( int i = 0; i < Threads; i += 1 ) set[ i ] = i;
	//srand( getpid() );
	shuffle( set, Threads );

	ctor();												// global algorithm constructor

	pthread_t workers[Threads];
#if defined( __linux ) && defined( PIN )
	cpu_set_t mask;
#endif // linux && PIN

	for ( int id = 0; id < Threads; id += 1 ) {			// start workers
		if ( pthread_create( &workers[id], NULL, Worker, (void *)(size_t)set[id] ) != 0 ) abort();
#if defined( __linux ) && defined( PIN )
		CPU_ZERO( &mask );
#ifdef FAST
		enum { offset = 35 };							// random CPU in upper range
#else
		enum { offset = 32 };							// upper range of cores away from core 0
#endif // FAST
		int cpu;
#if 0
		// 4x8x2 : 4 sockets, 8 cores per socket, 2 hyperthreads per core
		cpu = (id & 0x30) | ((id & 1) << 3) | ((id & 0xE) >> 1) + 32;
#endif // 0
#if 0
		static int cpus[] = { 0, 2, 4, 6, 8, 10, 12, 14,
							  1, 3, 5, 7, 9, 11, 13, 15,
							  16, 18, 20, 22, 24, 26, 28, 30,
							  17, 19, 21, 23, 25, 27, 29, 31,
		}; // cpus
		cpu = cpus[id] + 32;
#endif // 0
#if 1
		cpu = id + offset;
#endif // 0
		//printf( "%d\n", cpu );
		CPU_SET( cpu, &mask );
		if ( pthread_setaffinity_np( workers[id], sizeof(cpu_set_t), &mask ) != 0 ) {
			perror( "setaffinity" );
			abort();
		} // if
#endif // linux && PIN
	} // for

#ifdef STRESSINTERVAL
	// Periodically change concurrency level The time argument is ignored, although we could make it serve as a bound on
	// the run duration.  Detection of progress failures requires the human eye, specifically noticing that we stop
	// reporting in PollBarrier().  It's relatively easy to automate that aspect by waiting with timeouts.
	printf( "Stress mode : Threads=%d Interval=%d\n"
			"The program runs indefinitely in this mode.\n"
			"Performance data has no meaning, and should not be reported or collected!\n",
			Threads, StressInterval );
	for ( ;; ) { 
		poll( NULL, 0, StressInterval ); 
		BarHalt = 1; 
	} // for
#else
	for ( int r = 0; r < RUNS; r += 1 ) {
		poll( NULL, 0, Time * 1000 );
		stop = 1;										// reset
		while ( Arrived != Threads ) Pause();
		stop = 0;
		while ( Arrived != 0 ) Pause();
	} // for
#endif // STRESSINTERVAL

	for ( int i = 0; i < Threads; i += 1 ) {			// terminate workers
		if ( pthread_join( workers[i], NULL ) != 0 ) abort();
	} // for

	dtor();												// global algorithm destructor

	uint64_t totals[RUNS], sort[RUNS];

#ifdef DEBUG
	printf( "\n" );
#endif // DEBUG
	for ( int r = 0; r < RUNS; r += 1 ) {
		totals[r] = 0;
		for ( int id = 0; id < Threads; id += 1 ) {
			totals[r] += entries[r][id];
#ifdef DEBUG
			printf( "%ju ", entries[r][id] );
#endif // DEBUG
		} // for
#ifdef DEBUG
		printf( "\n" );
#endif // DEBUG
		sort[r] = totals[r];
	} // for
	qsort( sort, RUNS, sizeof(uint64_t), compare );
	uint64_t med = median( sort );
	printf( "%ju", med );								// median round

	unsigned int posn;
	for ( posn = 0; posn < RUNS && totals[posn] != med; posn += 1 ); // assumes RUNS is odd
#ifdef DEBUG
	printf( "\ntotals: " );
	for ( int i = 0; i < RUNS; i += 1 ) {				// print values
		printf( "%ju ", totals[i] );
	} // for
	printf( "\nsorted: " );
	for ( int i = 0; i < RUNS; i += 1 ) {				// print values
		printf( "%ju ", sort[i] );
	} // for
	printf( "\nmedian posn:%d\n", posn );
#endif // DEBUG
	double avg = (double)totals[posn] / Threads;		// average
	double sum = 0.0;
	for ( int id = 0; id < Threads; id += 1 ) {			// sum squared differences from average
		double diff = entries[posn][id] - avg;
		sum += diff * diff;
	} // for
	double std = sqrt( sum / Threads );
	printf( " %.1f %.1f %.1f%%", avg, std, std / avg * 100 );

	free( entries );

	printf( "\n" );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu99 -O3 Harness.c -lpthread -lm" //
// End: //
