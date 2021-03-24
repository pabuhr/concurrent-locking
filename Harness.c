// The test harness creates N pthread worker-threads, and then blocks for a fixed period of time, T, after which a
// global stop-flag is set to indicate an experiment is over.  The N threads repeatedly attempt entry into a
// self-checking critical-section until the stop flag is set.  During the T seconds, each thread counts the number of
// times it enters the critical section. The higher the aggregate count, the better an algorithm, as it is able to
// process more requests for the critical section per unit time.  When the stop flag is set, a worker thread stops
// entering the critical section, and atomically adds it subtotal entry-counter to a global total entry-counter. When
// the driver unblocks after T seconds, it busy waits until all threads have noticed the stop flag and added their
// subtotal to the global counter, which is then stored.  Five identical experiments are performed, each lasting T
// seconds. The median value of the five results is printed.

#ifndef __cplusplus
#define _GNU_SOURCE										// See feature_test_macros(7)
#endif // __cplusplus
#include <stdio.h>
#include <stdlib.h>										// abort, exit, atoi, rand, qsort
#include <math.h>										// sqrt
#include <assert.h>
#include <pthread.h>
#include <errno.h>										// errno
#include <stdint.h>										// uintptr_t, UINTPTR_MAX
#include <sys/time.h>
#include <poll.h>										// poll
#include <malloc.h>										// memalign
#include <unistd.h>										// getpid
#include <limits.h>										// ULONG_MAX
#ifdef CFMT												// output comma format
#include <locale.h>
#endif // CFMT

#ifdef __ARM_ARCH
#define WO( stmt ) stmt
#else
#define WO( stmt )
#endif

#if __GNUC__ >= 7										// valid GNU compiler diagnostic ?
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"	// Mute g++
#endif // __GNUC__ >= 7

#define CACHE_ALIGN 128									// Intel recommendation
#define CALIGN __attribute__(( aligned (CACHE_ALIGN) ))

#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#ifdef FAST
	// unlikely
	#define FASTPATH(x) __builtin_expect(!!(x), 0)
	#define SLOWPATH(x) __builtin_expect(!!(x), 1)
#else
	// likely
	#define FASTPATH(x) __builtin_expect(!!(x), 1)
	#define SLOWPATH(x) __builtin_expect(!!(x), 0)
#endif // FASTPATH

// Architectural ST-LD barrier -- memory fences
// In theory these should be obviated by the C++11 std::atomic_thread_fence() primitives.  Another option is the gcc
// _sync* primitives, but those are deprecated in favor of the new C++11 operators.  Unfortunately the current
// implementations of both __sync and C++11 atomic operators is sub-optimal.  On x86 these facilities use MFENCE, which
// is a poor choice.

#if defined(__x86_64)
	//#define Fence() __asm__ __volatile__ ( "mfence" )
	#define Fence() __asm__ __volatile__ ( "lock; addq $0,(%%rsp);" ::: "cc" )
#elif defined(__i386)
	#define Fence() __asm__ __volatile__ ( "lock; addl $0,(%%esp);" ::: "cc" )
#elif defined(__ARM_ARCH)
	#define Fence() __asm__ __volatile__ ( "DMB ISH" ::: )
#else
	#error unsupported architecture
#endif

// pause to prevent excess processor bus usage
#if defined( __i386 ) || defined( __x86_64 )
	#define Pause() __asm__ __volatile__ ( "pause" : : : )
//	#define Delay() for ( int i = random() & (512 - 1); i < 1000; i += 1 ) Pause()
//	#define Delay() { for ( uint32_t r = (ThreadLocalRandom() & ( (1<<16) - 1)) + ((1<<16) - 1); r >= 8; r -= 8 ) { Pause(); Pause(); Pause(); Pause(); Pause(); Pause(); Pause(); Pause(); } }
//	#define Delay() { for ( uint32_t r = 1 << (((ThreadLocalRandom() & (1<<4)) - 1) + 3); r >= 8; r -= 8 ) { Pause(); Pause(); Pause(); Pause(); Pause(); Pause(); Pause(); Pause(); } }
#elif defined(__ARM_ARCH)
	#define Pause() __asm__ __volatile__ ( "YIELD" ::: )
#else
	#error unsupported architecture
#endif

//------------------------------------------------------------------------------

typedef uintptr_t TYPE;									// addressable word-size
typedef volatile TYPE VTYPE;							// volatile addressable word-size
typedef _Atomic TYPE ATYPE;								// atomic addressable word-size
typedef uint32_t RTYPE;									// unsigned 32-bit integer

//------------------------------------------------------------------------------

static RTYPE Mix64( uint64_t v ) {
	static const int64_t Mix64K = 0xdaba0b6eb09322e3LL;
	v = (v ^ (v >> 32)) * Mix64K;
	v = (v ^ (v >> 32)) * Mix64K;
	return v ^ (v >> 32);
} // Mix64

// Marsaglia shift-XOR PRNG with thread-local state
// Period is 4G-1
// 0 is absorbing and must be avoided
// Low-order bits are not particularly random
// Self-seeding based on address of thread-local state
// Not reproducible run-to-run because of user-mode address space randomization
// Pipelined to allow OoO overlap with reduced dependencies
// Critically, return the current value, and compute and store the next value
// Optionally, sequester R on its own cache line to avoid false sharing
// but on linux __thread "initial-exec" TLS variables are already segregated.

static __attribute__(( unused )) RTYPE ThreadLocalRandom() {
	static __thread RTYPE R = 0;
	if ( R == 0 ) R = Mix64( (uint64_t)(uintptr_t)&R ) | 1;
	RTYPE v = R;
	RTYPE n = v;
	n ^= n << 6;
	n ^= n >> 21;
	n ^= n << 7;
	R = n;
	return v;
} // ThreadLocalRandom

//------------------------------------------------------------------------------

static __attribute__(( unused )) inline TYPE cycleUp( TYPE v, TYPE n ) { return ( ((v) >= (n - 1)) ? 0 : (v + 1) ); }
static __attribute__(( unused )) inline TYPE cycleDown( TYPE v, TYPE n ) { return ( ((v) <= 0) ? (n - 1) : (v - 1) ); }

#if defined( __GNUC__ )									// GNU gcc compiler ?
// O(1) polymorphic integer log2, using clz, which returns the number of leading 0-bits, starting at the most
// significant bit (single instruction on x86)
#define Log2( n ) ( sizeof(n) * __CHAR_BIT__ - 1 - (			\
				  ( sizeof(n) ==  4 ) ? __builtin_clz( n ) :	\
				  ( sizeof(n) ==  8 ) ? __builtin_clzl( n ) :	\
				  ( sizeof(n) == 16 ) ? __builtin_clzll( n ) :	\
				  -1 ) )
#else
static __attribute__(( unused )) int Log2( int n ) {	// fallback integer log2( n )
	return n > 1 ? 1 + Log2( n / 2 ) : n == 1 ? 0 : -1;
}
#endif // __GNUC__

static __attribute__(( unused )) inline int Clog2( int n ) { // integer ceil( log2( n ) )
	if ( n <= 0 ) return -1;
	int ln = Log2( n );
	return ln + ( (n - (1 << ln)) != 0 );				// check for any 1 bits to the right of the most significant bit
}

//------------------------------------------------------------------------------

#ifdef CNT
struct CALIGN cnts {
	uint64_t cnts[CNT + 1];
};
static struct cnts ** counters CALIGN;
#endif // CNT

//------------------------------------------------------------------------------

static ATYPE stop CALIGN = 0;
static ATYPE Arrived CALIGN = 0;
static uintptr_t N CALIGN, Threads CALIGN, Time CALIGN;
static intptr_t Degree CALIGN = -1;

enum { RUNS = 5 };
volatile int Run CALIGN = 0;

//------------------------------------------------------------------------------

// memory allocator to align or not align storage
#define Allocator( size ) memalign( CACHE_ALIGN, (size) )

//------------------------------------------------------------------------------

enum { CSTimes = 50 };

#ifdef NONCS
static inline void NonCriticalSection( const TYPE id ) {
	for ( volatile int i = 0; i < 550; i += 1 );		// Spinlock: 200
	// Fence();											// optional
	// for ( int i = 1; i <= CSTimes; i += 1 ) {		// delay
	// 	if ( id == Threads ) {							// mutual exclusion violation ?
	// 		printf( "Interference Id:%zu\n", id );
	// 	} // if
	// } // for
} // NonCriticalSection
#endif // NONCS

static TYPE HPAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static volatile RTYPE randomChecksum CALIGN = 0;
static volatile RTYPE sumOfThreadChecksums CALIGN = 0;
static VTYPE CurrTid CALIGN = ULONG_MAX;				// shared, current thread id in critical section
static TYPE HPAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

//static int Identity(int v) { __asm__ __volatile__ (";" : "+r" (v)); return v; } 
static inline RTYPE CriticalSection( const TYPE tid ) {
	#ifdef CNT
	if ( UNLIKELY( CurrTid == tid ) ) counters[Run][tid].cnts[0] += 1;
	#endif // CNT
	CurrTid = tid;

	// If the critical section is violated, the additions are corrupted because of the load/store race unless there is
	// perfect interleaving. Note, the load, delay, store to increase the cchance of detecting a violation.
	RTYPE randomNumber = ThreadLocalRandom();			// belt and
	volatile RTYPE copy = randomChecksum;
	for ( volatile int delay = 0; delay < CSTimes; delay += 1 ) {} // delay
	randomChecksum = copy + randomNumber;

	// The assignment to CurrTid above can delay in a store buffer, that is, committed but not pushed into coherent
	// space. Hence, the load below fetchs the value from the store buffer via look aside instead of the coherent
	// version.  If this scenario occurs for multiple threads in the CS, these threads do not detect the violation
	// because their copy of CurrTid in the store buffer is unchanged.
	if ( CurrTid != tid ) {								// suspenders
		printf( "Interference Id:%zu\n", tid );
		abort();
	} // if

	return randomNumber;
} // CriticalSection

//------------------------------------------------------------------------------

#ifdef FAST
enum { MaxStartPoints = 64 };
static unsigned int NoStartPoints CALIGN;
static uint64_t * Startpoints CALIGN;

// To ensure the single thread exercises all aspects of an algorithm, it is assigned different start-points on each
// access to the critical section by randomly changing its thread id.  The randomness is accomplished using
// approximately 64 pseudo-random thread-ids, where 64 is divided by N to get R repetitions, e.g., for N = 5, R = 64 / 5
// = 12.  Each of the 12 repetition is filled with 5 random value in the range, 0..N-1, without replacement, e.g., 0 3 4
// 1 2.  There are no consecutive thread-ids within a repetition but there may be between repetition.  The thread cycles
// through this array of ids during an experiment.

void startpoints() {
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
	printf( "N %jd NoStartPoints %d ", N, NoStartPoints );
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

//------------------------------------------------------------------------------

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
static int StressInterval CALIGN = STRESSINTERVAL;		// 500 is good
static volatile int BarHalt CALIGN = 0;

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

//------------------------------------------------------------------------------

void affinity( pthread_t pthreadid, unsigned int tid ) {
// There are many ways to assign threads to processors: cores, chips, etc.
// On the AMD, we find starting at core 32 and sequential assignment is sufficient.
// Below are alternative approaches.
#if defined( __linux ) && defined( PIN )
	cpu_set_t mask;

	CPU_ZERO( &mask );
	int cpu;

#if 0
	// 4x8x2 : 4 sockets, 8 cores per socket, 2 hyperthreads per core
	cpu = (tid & 0x30) | ((tid & 1) << 3) | ((tid & 0xE) >> 1) + 32;
#endif // 0

#if 1
#if defined( c4arm )
	enum { OFFSETSOCK = 1 /* 0 origin */, SOCKET = 2, CORES = 48, HYPER = 1 };
#elif defined( nasus )
	enum { OFFSETSOCK = 1 /* 0 origin */, SOCKET = 2, CORES = 64, HYPER = 1 };
#elif defined( jax )
	enum { OFFSETSOCK = 1 /* 0 origin */, SOCKET = 4, CORES = 24, HYPER = 2 /*wrap */ };
#elif defined( pyke )
	enum { OFFSETSOCK = 1 /* 0 origin */, SOCKET = 2, CORES = 24, HYPER = 2 /*wrap */ };
#elif defined( cfapi1 )
	enum { OFFSETSOCK = 0 /* 0 origin */, SOCKET = 1, CORES = 4, HYPER = 1 };
#else // default
	enum { OFFSETSOCK = 2 /* 0 origin */, SOCKET = 4, CORES = 16, HYPER = 1 };
#endif // HOSTS
	cpu = tid + ((tid < CORES) ? OFFSETSOCK * CORES : HYPER < 2 ? OFFSETSOCK * CORES : CORES * SOCKET);
#endif // 0

	//printf( "%d\n", cpu );
	CPU_SET( cpu, &mask );
	int rc = pthread_setaffinity_np( pthreadid, sizeof(cpu_set_t), &mask );
	if ( rc != 0 ) {
		errno = rc;
		perror( "setaffinity" );
		abort();
	} // if
#endif // linux && PIN
} // affinity

//------------------------------------------------------------------------------

static uint64_t ** entries CALIGN;						// holds CS entry results for each threads for all runs

#define xstr(s) str(s)
#define str(s) #s
#ifdef __cplusplus
#include xstr(Algorithm.cc)								// include software algorithm for testing
#else
#include xstr(Algorithm.c)								// include software algorithm for testing
#endif // __cplusplus

//------------------------------------------------------------------------------

static __attribute__(( unused )) void shuffle( unsigned int set[], const int size ) {
	unsigned int p1, p2, temp;

	for ( int i = 0; i < 200; i +=1 ) {					// shuffle array S times
		p1 = rand() % size;
		p2 = rand() % size;
		temp = set[p1];
		set[p1] = set[p2];
		set[p2] = temp;
	} // for
} // shuffle

//------------------------------------------------------------------------------

#define median(a) ((RUNS & 1) == 0 ? (a[RUNS/2-1] + a[RUNS/2]) / 2 : a[RUNS/2] )
static int compare( const void * p1, const void * p2 ) {
	size_t i = *((size_t *)p1);
	size_t j = *((size_t *)p2);
	return i > j ? 1 : i < j ? -1 : 0;
} // compare

//------------------------------------------------------------------------------

int main( int argc, char * argv[] ) {
	N = 8;												// defaults
	Time = 10;											// seconds

	switch ( argc ) {
	  case 4:
		Degree = atoi( argv[3] );
		if ( Degree < 2 ) goto USAGE;
	  case 3:
		Time = atoi( argv[2] );
		N = atoi( argv[1] );
		if ( Time < 1 || N < 1 ) goto USAGE;
		break;
	  USAGE:
	  default:
		printf( "Usage: %s %ju (number of threads) %ju (time in seconds threads spend entering critical section) %jd (Zhang D-ary)\n",
				argv[0], N, Time, Degree );
		exit( EXIT_FAILURE );
	} // switch

#ifdef CFMT
	#define COMMA "'13"
	setlocale( LC_NUMERIC, "en_US.UTF-8" );

	if ( N == 1 ) printf( "%s"
	#ifdef __cplusplus
						  ".cc"
	#else
						  ".c"
	#endif // __cplusplus
						  "\n  N   T    CS Entries           AVG           STD   RSTD   CAVG        SMALLS\n", xstr(Algorithm) );
#else
	#define COMMA ""
#endif // CFMT

	printf( "%3ju %3jd ", N, Time );

#ifdef FAST
	assert( N <= MaxStartPoints );
	Threads = 1;										// fast test, Threads=1, N=1..32
	NoStartPoints = MaxStartPoints / N * N;				// floor( MaxStartPoints / N )
	Startpoints = Allocator( sizeof(typeof(Startpoints[0])) * NoStartPoints );
	startpoints( N );
#else
	Threads = N;										// allow testing of T < N
#endif // FAST

	entries = (typeof(entries[0]) *)malloc( sizeof(typeof(entries[0])) * RUNS );
#ifdef CNT
	counters = (typeof(counters[0]) *)malloc( sizeof(typeof(counters[0])) * RUNS );
#endif // CNT
	for ( int r = 0; r < RUNS; r += 1 ) {
		entries[r] = (typeof(entries[0][0]) *)Allocator( sizeof(typeof(entries[0][0])) * Threads );
#ifdef CNT
#ifdef FAST
		counters[r] = (typeof(counters[0][0]) *)Allocator( sizeof(typeof(counters[0][0])) * N );
#else
		counters[r] = (typeof(counters[0][0]) *)Allocator( sizeof(typeof(counters[0][0])) * Threads );
#endif // FAST
#endif // CNT
	} // for

#ifdef CNT
//#ifdef FAST
	// For FAST experiments, there is only thread but it changes its thread id to visit all the start points. Therefore,
	// all the counters for each id must be initialized and summed at the end.
	for ( int r = 0; r < RUNS; r += 1 ) {
		for ( uintptr_t id = 0; id < N; id += 1 ) {
			for ( unsigned int i = 0; i < CNT + 1; i += 1 ) { // reset for each run
				counters[r][id].cnts[i] = 0;
			} // for
		} // for
	} // for
//#endif // FAST
#endif // CNT

	unsigned int set[Threads];
	for ( typeof(Threads) i = 0; i < Threads; i += 1 ) set[ i ] = i;
	// srand( getpid() );
	// shuffle( set, Threads );							// randomize thread ids
	#ifdef DEBUG
	printf( "\nthread set: " );
	for ( uintptr_t i = 0; i < Threads; i += 1 ) printf( "%u ", set[ i ] );
	#endif // DEBUG

	ctor();												// global algorithm constructor

	pthread_t workers[Threads];

	for ( typeof(Threads) tid = 0; tid < Threads; tid += 1 ) { // start workers
		int rc = pthread_create( &workers[tid], NULL, Worker, (void *)(size_t)set[tid] );
		if ( rc != 0 ) {
			errno = rc;
			perror( "pthread create" );
			abort();
		} // if
		affinity( workers[tid], tid );
	} // for

	for ( ; Run < RUNS;  ) {							// global variable
		// threads start first experiment immediately
		sleep( Time );									// delay for experiment duration
		stop = 1;										// stop threads
		while ( Arrived != Threads ) Pause();			// all threads stopped ?

		if ( randomChecksum != sumOfThreadChecksums ) {
			printf( "Interference run %d randomChecksum %u sumOfThreadChecksums %u\n", Run, randomChecksum, sumOfThreadChecksums );
			abort();
		} // if
		randomChecksum = sumOfThreadChecksums = 0;
		CurrTid = ULONG_MAX;							// reset for next run

		Run += 1;
		stop = 0;										// start threads
		while ( Arrived != 0 ) Pause();					// all threads started ?
	} // for

	for ( typeof(Threads) tid = 0; tid < Threads; tid += 1 ) { // terminate workers
		int rc = pthread_join( workers[tid], NULL );
		if ( rc != 0 ) {
			errno = rc;
			perror( "pthread join" );
			abort();
		} // if
	} // for

	dtor();												// global algorithm destructor

	uint64_t totals[RUNS], sort[RUNS], smalls[RUNS];

	#ifdef DEBUG
	printf( "\nruns: " );
	#endif // DEBUG
	for ( int r = 0; r < RUNS; r += 1 ) {
		smalls[r] = totals[r] = 0;
		for ( typeof(Threads) tid = 0; tid < Threads; tid += 1 ) {
			if ( entries[r][tid] <= 5 ) smalls[r] += 1;	// only 5 entries in CS => small
			totals[r] += entries[r][tid];
			#ifdef DEBUG
			printf( "%" COMMA "ju ", entries[r][tid] );
			#endif // DEBUG
		} // for
		sort[r] = totals[r];
	} // for
	qsort( sort, RUNS, sizeof(typeof(sort[0])), compare );
	uint64_t med = median( sort );

	double sum = 0.0;
	for ( int r = 0; r < RUNS; r += 1 ) {
		sum += totals[r];
	} // for
	double avg = sum / RUNS;							// average
	sum = 0.0;
	for ( int r = 0; r < RUNS; r += 1 ) {				// sum squared differences from average
		double diff = totals[r] - avg;
		sum += diff * diff;
	} // for
	double std = sqrt( sum / Threads );
	avg = avg == 0 ? 0.0 : std / avg * 100;
	if ( avg > 10.0 ) printf( "\nWarning relative standard deviation %.1f%% greater than 10%% over %d runs.\n", avg, RUNS );

	unsigned int posn;									// run with median result
	for ( posn = 0; posn < RUNS && totals[posn] != med; posn += 1 ); // assumes RUNS is odd

	#ifdef DEBUG
	printf( "\ntotals: " );
	for ( int i = 0; i < RUNS; i += 1 ) {				// print values
		printf( "%" COMMA "ju ", totals[i] );
	} // for
	printf( "\nsorted: " );
	for ( int i = 0; i < RUNS; i += 1 ) {				// print values
		printf( "%" COMMA "ju ", sort[i] );
	} // for
	printf( "\nmedian posn:%d\n", posn );
	#endif // DEBUG

	avg = (double)totals[posn] / Threads;				// average
	sum = 0.0;
	for ( typeof(Threads) tid = 0; tid < Threads; tid += 1 ) { // sum squared differences from average
		double diff = entries[posn][tid] - avg;
		sum += diff * diff;
	} // for
	std = sqrt( sum / Threads );

	printf( "%" COMMA "ju", med );						// median round
	printf( " %" COMMA ".1f %" COMMA ".1f %5.1f%%", avg, std, avg == 0 ? 0.0 : std / avg * 100 );


#if 0
#ifdef CNT
	printf( "\n\n" );
	uint64_t cntsum2;
	for ( unsigned int r = 0; r < RUNS; r += 1 ) {
		cntsum2 = 0;
#ifdef FAST
		for ( uintptr_t tid = 0; tid < N; tid += 1 ) {
#else
		for ( uintptr_t tid = 0; tid < Threads; tid += 1 ) {
			cntsum2 += counters[r][tid].cnts[0];
#endif // FAST
			printf( "%" COMMA "ju ", counters[r][tid].cnts[0] );
		} // for
		printf( "= %lu\n", cntsum2 );
	} // for
	printf( "\n" );
#endif // CNT
#endif // 0


#ifdef CNT
	// posn is the run containing the median result. Other runs are ignored.
	uint64_t cntsum;
	for ( unsigned int i = 0; i < CNT + 1; i += 1 ) {
		cntsum = 0;
#ifdef FAST
		for ( uintptr_t tid = 0; tid < N; tid += 1 ) {
#else
		for ( uintptr_t tid = 0; tid < Threads; tid += 1 ) {
#endif // FAST
			cntsum += counters[posn][tid].cnts[i];
		} // for
		printf( " %5.1f%%", (double)cntsum / (double)totals[posn] * 100.0 );
	} // for
#endif // CNT

	printf( " %" COMMA "ju", smalls[posn] );

	for ( int r = 0; r < RUNS; r += 1 ) {
		free( entries[r] );
#ifdef CNT
		free( counters[r] );
#endif // CNT
	} // for
#ifdef CNT
	free( counters );
#endif // CNT

	free( entries );

#ifdef FAST
	free( Startpoints );
#endif // FAST

	printf( "\n" );
} // main

// Local Variables: //
// tab-width: 4 //
// compile-command: "gcc -Wall -std=gnu11 -O3 Harness.c -lpthread -lm" //
// End: //
