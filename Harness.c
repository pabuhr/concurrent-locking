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
#define CALIGN __attribute__(( aligned(CACHE_ALIGN) ))

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

#define max( x, y ) (x > y ? x : y)
#define min( x, y ) (x < y ? x : y)

#define xstr(s) str(s)
#define str(s) #s

//------------------------------------------------------------------------------

typedef uintptr_t TYPE;									// addressable word-size
typedef volatile TYPE VTYPE;							// volatile addressable word-size
typedef uint32_t RTYPE;									// unsigned 32-bit integer

#if __WORDSIZE == 64
typedef uint32_t HALFSIZE;
typedef volatile uint32_t VHALFSIZE;
typedef uint64_t WHOLESIZE;
typedef volatile uint64_t VWHOLESIZE;
#else
typedef uint16_t HALFSIZE;
typedef volatile uint16_t VHALFSIZE;
typedef uint32_t WHOLESIZE;
typedef volatile uint32_t VWHOLESIZE;
#endif // __WORDSIZE == 64

#ifdef ATOMIC
#define VTYPE _Atomic TYPE
#define VWHOLESIZE _Atomic WHOLESIZE
#endif // ATOMIC

//------------------------------------------------------------------------------

// Architectural ST-LD memory fences: In theory explicit memory fences should be obviated by the C11 automated atomic
// declarations, which is provided by specifying -DATOMIC.  Unfortunately, the current implementations of C11 atomic
// operators is still sub-optimal compared to hand fencing. Manually annotating every load/store with atomic macros is
// more error prone because there is more to get wrong than manually fencing.

#ifdef ATOMIC
	#if defined( LPAUSE ) || defined( MPAUSE )
		#error Compilation options LPAUSE/MPAUSE are incompatible with ATOMIC.
	#endif
	#define Fence()
#else
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
#endif // ATOMIC

// pause to prevent excess processor bus usage
#if defined( LPAUSE ) && defined( MPAUSE )
	#error Compilation options LPAUSE and MPAUSE cannot be used together.
#endif

#if defined( __i386 ) || defined( __x86_64 )

#if defined( LPAUSE )
	#define Pause() __asm__ __volatile__ ( "lfence" )
#elif defined( MPAUSE )
	inline TYPE MonitorLD( VTYPE * A ) {
		TYPE rv = 0;
		__asm__ __volatile__ (
			"xorq %%rcx,%%rcx; xorq %%rdx,%%rdx; monitorx; mov (%[RA]), %[RV];  "
			: [RV] "=r" (rv)
			: [RA] "a" (A)
			: "rcx", "rdx", "memory") ;
		return rv ;
	}
	inline void MWait( int timo __attribute__(( unused )) ) { // newer mwait takes an argument
		__asm__ __volatile__ ( "xorq %%rcx,%%rcx; xorq %%rax,%%rax; mwaitx; " ::: "rax", "rcx" );
	}
	#define MPause( E, C ) { while ( (typeof(E))MonitorLD( (VTYPE *)(&(E)) ) C ) { MWait( 0 ); } }
	#define MPauseS( S, E, C ) { while ( ( S (typeof(E))MonitorLD( (VTYPE *)(&(E)) ) ) C ) { MWait( 0 ); } }
	#define Pause() __asm__ __volatile__ ( "pause" ::: )
#else
	#define Pause() __asm__ __volatile__ ( "pause" ::: )
#endif // LPAUSE

#elif defined(__ARM_ARCH)

#if defined( LPAUSE )
	#define Pause() __asm__ __volatile__ ( "DMB ISH" ::: )
#elif defined( MPAUSE )
	inline TYPE MonitorLD( VTYPE * A ) {
		TYPE v = 0;
		// Polymorphic based on size of operand => automagically selects w or x register for %0.
		__asm__ __volatile__ ( "wfe; ldaxr %0,%1; " : "=r" (v) : "Q" (*A) );
		return v;
	} // MonitorLD

	#define sevl() __asm__ __volatile__ ( "sevl" )
	// Polymorphic on operand using type erasure => use uintptr_t so both values and pointers work.
	#define MPause( E, C ) { sevl(); while ( (typeof(E))MonitorLD( (VTYPE *)(&(E)) ) C ) {} }
	#define MPauseS( S, E, C ) { sevl(); while ( ( S (typeof(E))MonitorLD( (VTYPE *)(&(E)) ) ) C ) {} }
	#define Pause() __asm__ __volatile__ ( "YIELD" ::: )
#else
	#define Pause() __asm__ __volatile__ ( "YIELD" ::: )
#endif // LPAUSE

#else
	#error unsupported architecture
#endif

//------------------------------------------------------------------------------

#define Clr( lock ) __atomic_clear( lock, __ATOMIC_RELEASE )
#define Clrm( lock, memorder ) __atomic_clear( lock, memorder )
#define Tas( lock ) __atomic_test_and_set( (lock), __ATOMIC_ACQUIRE )
#define Tasm( lock, memorder ) __atomic_test_and_set( (lock), memorder )
#define Cas( change, comp, assn ) ({typeof(comp) __temp = (comp); __atomic_compare_exchange_n( (change), &(__temp), (assn), false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST ); })
#define Casm( change, comp, assn, memorder... ) ({typeof(comp) * __temp = &(comp); __atomic_compare_exchange_n( (change), &(__temp), (assn), false, memorder ); })
#define Casv( change, comp, assn ) __atomic_compare_exchange_n( (change), (comp), (assn), false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST )
#define Casvm( change, comp, assn, memorder... ) __atomic_compare_exchange_n( (change), (comp), (assn), false, memorder )
#define Fas( change, assn ) __atomic_exchange_n( (change), (assn), __ATOMIC_SEQ_CST )
#define Fasm( change, assn, memorder ) __atomic_exchange_n( (change), (assn), memorder )
#define Fai( change, Inc ) __atomic_fetch_add( (change), (Inc), __ATOMIC_SEQ_CST )
#define Faim( change, Inc, memorder ) __atomic_fetch_add( (change), (Inc), memorder )

#define await( E ) while ( ! (E) ) Pause()

//------------------------------------------------------------------------------

static inline long long int rdtscl( void ) {
	#if defined( __i386 ) || defined( __x86_64 )
	unsigned int lo, hi;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );

	#elif defined( __aarch64__ ) || defined( __arm__ )
	// https://github.com/google/benchmark/blob/v1.1.0/src/cycleclock.h#L116
	long long int virtual_timer_value;
	asm volatile("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));
	return virtual_timer_value;
	#else
		#error unsupported hardware architecture
	#endif
}

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

static __thread RTYPE R;
static __attribute__(( unused, noinline )) RTYPE PRNG() { // must be called
	uint32_t ret = R;
	R ^= R << 6;
	R ^= R >> 21;
	R ^= R << 7;
	return ret;
} // PRNG

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

// Do not use VTYPE because -DATOMIC changes it.
static _Atomic(TYPE) stop CALIGN = 0;
static _Atomic(TYPE) Arrived CALIGN = 0;
static uintptr_t N CALIGN, Threads CALIGN, Time CALIGN;
static intptr_t Degree CALIGN = -1;

enum { RUNS = 5 };
volatile int Run CALIGN = 0;

//------------------------------------------------------------------------------

// memory allocator to align or not align storage
#define Allocator( size ) memalign( CACHE_ALIGN, (size) )

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

static inline unsigned int startpoint( unsigned int pos ) {
	assert( pos < NoStartPoints );
	return Startpoints[pos];
//	return rand() % N;
} // startpoint
#endif // FAST

//------------------------------------------------------------------------------

#ifndef NCS_DELAY
#define NCS_DELAY 0
#endif // NCS_DELAY

#ifndef CSTIMES
#define CSTIMES 20
#endif // CSTIMES

enum {
	NCSTimes = NCS_DELAY,								// time delay before attempting entry to CS
	CSTimes  = CSTIMES									// time spent in CS + random-number call
};

static TYPE HPAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static volatile RTYPE randomChecksum CALIGN = 0;
static volatile RTYPE sumOfThreadChecksums CALIGN = 0;
// Do not use VTYPE because -DATOMIC changes it.
static volatile TYPE CurrTid CALIGN = 0;				// shared, current thread id in critical section
static TYPE HPAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#if NCS_DELAY != 0
	#define NCS_DECL
//	#define NCS if ( UNLIKELY( id == 0 && N > 1 ) ) NonCriticalSection( id )
	#define NCS if ( UNLIKELY( N > 1 ) ) NonCriticalSection( id )
	static inline void NonCriticalSection() {
//		volatile RTYPE randomNumber = PRNG() % NCSTimes; // match with CS
//		for ( VTYPE delay = 0; delay < randomNumber; delay += 1 ) {} // short random delay
		for ( VTYPE delay = 0; delay < NCSTimes; delay += 1 ) {} // short fixed delay
	} // NonCriticalSection
#else
	#define NCS_DECL
	#define NCS
#endif // NCS_DELAY != 0

#ifdef CONVOY
static TYPE convoy[16][128][128];						// [RUNS][THREADS][THREADS]
#endif // CONVOY

//static int Identity(int v) { __asm__ __volatile__ (";" : "+r" (v)); return v; } 
#if CSTIMES != 0
static inline RTYPE CS( const TYPE tid __attribute__(( unused )) ) { // parameter unused for CSTIME == 0
	#ifdef CONVOY
	convoy[Run][tid][CurrTid] += 1;						// can be off by 1 for thread 0
	#endif // CONVOY

	CurrTid = tid;										// CurrTid is global

	// If the critical section is violated, the additions are corrupted because of the load/store race unless there is
	// perfect interleaving. Note, the load, delay, store to increase the chance of detecting a violation.
	RTYPE randomNumber = PRNG();						// belt and
	volatile RTYPE copy = randomChecksum;
	for ( volatile int delay = 0; delay < CSTimes; delay += 1 ) {} // short delay
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
} // CS
#else
static inline RTYPE CS( const TYPE tid __attribute__(( unused )) ) { // parameter unused for CSTIME == 0
	#ifdef CONVOY
	convoy[Run][tid][CurrTid] += 1;						// can be off by 1 for thread 0
	#endif // CONVOY

	CurrTid = tid;										// CurrTid is global
	return 0;
} // CS
#endif // CSTIMES != 0

//------------------------------------------------------------------------------

#if defined( FAST ) || defined( NCS_DELAY )
void randPoints( uint64_t points[], unsigned int numPoints, unsigned int N ) {
	//printf( "randPoints points %p, numPoints %d, N %d\n", points, numPoints, N );
	points[0] = N;
 	for ( unsigned int i = 0; i < numPoints; i += N ) {
		for ( unsigned int j = i; j < i + N; j += 1 ) {
			unsigned int v;
		  L: v = rand() % N;
			size_t k;
			for ( k = i; k < j; k += 1 ) {
				if ( points[k] == v ) goto L;
			} // for
			// Unknown performance drop caused by following assert, use -DNDEBUG for experiments
			assert( k < numPoints );
			points[k] = v;
		} // for
	} // for
#if 0
	printf( "\nN %d numPoints %d\n", N, numPoints );
	for ( size_t i = 0; i < numPoints; i += 1 ) {
		printf( "%2jd ", points[i] );
		if ( (i+1) % N == 0 ) printf( "\n" );
	} // for
	printf( "\n" );
#endif // 0
} // randPoints
#endif // FAST || NCS_DELAY

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
	int t = Fai(&Ticket, 1);
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
		int k = Fai( &nrun, 1 );
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
#if 1
#if defined( nasus )
	#if ! defined( HYPERAFF ) && ! defined( LINEARAFF )		// default affinity
	#define HYPERAFF
	#endif // HYPERAFF

	enum { OFFSETSOCK = 1 /* 0 origin */, SOCKETS = 2, CORES = 64, HYPER = 1 };
	#if defined( LINEARAFF )
	int cpu = tid + ((tid < CORES) ? OFFSETSOCK * CORES : HYPER < 2 ? OFFSETSOCK * CORES : CORES * SOCKETS);
	#endif // LINEARAFF
	#if defined( HYPERAFF )
	int cpu = OFFSETSOCK * CORES + (tid / 2) + ((tid % 2 == 0) ? 0 : CORES * SOCKETS);
	#endif // HYPERAFF
#elif defined( pyke )
	#if ! defined( HYPERAFF ) && ! defined( LINEARAFF )		// default affinity
	#define HYPERAFF
	#endif // HYPERAFF

	enum { OFFSETSOCK = 0 /* 0 origin */, SOCKETS = 2, CORES = 24, HYPER = 1 /* wrap on socket */ };
	#if defined( LINEARAFF )
	int cpu = tid + ((tid < CORES) ? OFFSETSOCK * CORES : HYPER < 2 ? OFFSETSOCK * CORES : CORES * SOCKETS);
	#endif // LINEARAFF
	#if defined( HYPERAFF )
	int cpu = OFFSETSOCK * CORES + (tid / 2) + ((tid % 2 == 0) ? 0 : CORES * SOCKETS );
	#endif // HYPERAFF
#else
#define LINEARAFF
#if defined( algol )
	enum { OFFSETSOCK = 1 /* 0 origin */, SOCKETS = 2, CORES = 48, HYPER = 1 };
#elif defined( prolog )
	enum { OFFSETSOCK = 1 /* 0 origin */, SOCKETS = 2, CORES = 64, HYPER = 1 }; // pretend 2 sockets
#elif defined( jax )
	enum { OFFSETSOCK = 1 /* 0 origin */, SOCKETS = 4, CORES = 24, HYPER = 2 /* wrap on socket */ };
#elif defined( cfapi1 )
	enum { OFFSETSOCK = 0 /* 0 origin */, SOCKETS = 1, CORES = 4, HYPER = 1 };
#else // default
	enum { OFFSETSOCK = 2 /* 0 origin */, SOCKETS = 4, CORES = 16, HYPER = 1 };
#endif // HOSTS
	int cpu = tid + ((tid < CORES) ? OFFSETSOCK * CORES : HYPER < 2 ? OFFSETSOCK * CORES : CORES * SOCKETS);
#endif // 0
#endif // computer

#if 0
	// 4x8x2 : 4 sockets, 8 cores per socket, 2 hyperthreads per core
	int cpu = (tid & 0x30) | ((tid & 1) << 3) | ((tid & 0xE) >> 1) + 32;
#endif // 0
	//printf( "%d\n", cpu );

	cpu_set_t mask;
	CPU_ZERO( &mask );
	CPU_SET( cpu, &mask );
	int rc = pthread_setaffinity_np( pthreadid, sizeof(cpu_set_t), &mask );
	if ( rc != 0 ) {
		errno = rc;
		char buf[64];
		snprintf( buf, 64, "***ERROR*** setaffinity failure for CPU %d", cpu );
		perror( buf );
		abort();
	} // if
#endif // linux && PIN
} // affinity

//------------------------------------------------------------------------------

static uint64_t ** entries CALIGN;						// holds CS entry results for each threads for all runs

#ifdef __cplusplus
#include xstr(Algorithm.cc)								// include software algorithm for testing
#else
#include xstr(Algorithm.c)								// include software algorithm for testing
#endif // __cplusplus

//------------------------------------------------------------------------------

static __attribute__(( unused )) void shuffle( unsigned int set[], const int size ) {
	unsigned int p1, p2, temp;

	for ( size_t i = 0; i < 200; i += 1 ) {				// shuffle array S times
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

static void statistics( size_t N, uint64_t values[N], double * avg, double * std, double * rstd ) {
	double sum = 0.;
	for ( size_t r = 0; r < N; r += 1 ) {
		sum += values[r];
	} // for
	*avg = sum / N;										// average
	sum = 0.;
	for ( size_t r = 0; r < N; r += 1 ) {				// sum squared differences from average
		double diff = values[r] - *avg;
		sum += diff * diff;
	} // for
	*std = sqrt( sum / N );
	*rstd = *avg == 0.0 ? 0.0 : *std / *avg * 100;
} // statisitics

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
		printf( "Usage: %s [ threads (> 0) %ju ] [ experiment duration (> 0, seconds) %ju ] [ Zhang D-ary (> 1) %jd ]\n",
				argv[0], N, Time, Degree );
		exit( EXIT_FAILURE );
	} // switch

	R = rdtscl();										// seed PRNG

#ifdef CFMT
	#define QUOTE "'13"
	setlocale( LC_NUMERIC, "en_US.UTF-8" );

	if ( N == 1 ) {										// title
		printf( "%s"
	#ifdef __cplusplus
				".cc,"
	#else
				".c,"
	#endif // __cplusplus
	#if defined( LINEARAFF )
				" LINEAR AFFINITY,"
	#endif // LINEARAFF
	#if defined( HYPERAFF )
				" HYPER AFFINITY,"
	#endif // HYPERAFF
	#ifdef FAST
				" FAST,"
	#endif // FAST
	#ifdef ATOMIC
				" ATOMIC,"
	#endif // ATOMIC
	#ifdef THREADLOCAL
				" THREADLOCAL,"
	#endif // THREADLOCAL
	#if defined( NOEXPBACK ) && ! defined( MPAUSE )
				" NOEXPBACK,"
	#endif // NOEXPBACK
	#ifdef LPAUSE
				" LFENCE pause,"
	#endif // LPAUSE
	#ifdef MPAUSE
				" MONITOR pause,"
	#endif // MPAUSE
	#ifdef FCFS
				" FCFS,"
	#endif // FCFS
	#ifdef FCFSTest
				" FCFSTest,"
	#endif // FCFSTest
				" %d NCS spins,"
				" %d CS spins,"
				" %d runs median"
				, xstr(Algorithm), NCSTimes, CSTimes, RUNS );
		if ( Degree != -1 ) printf( " %jd-ary", Degree ); // Zhang only
		printf( "\n  N   T    CS Entries           AVG           STD   RSTD"
	#ifdef CNT
				"   CAVG"
	#endif // CNT
			"\n");
	} // if
#else
	#define QUOTE ""
#endif // CFMT

	printf( "%3ju %3jd ", N, Time );

	#ifdef FAST
	assert( N <= MaxStartPoints );
	Threads = 1;										// fast test, Threads=1, N=1..32
	NoStartPoints = MaxStartPoints / N * N;				// floor( MaxStartPoints / N )
	Startpoints = Allocator( sizeof(typeof(Startpoints[0])) * NoStartPoints );
	randPoints( Startpoints, NoStartPoints, N );
	#else
	Threads = N;										// allow testing of T < N
	#endif // FAST

	entries = (typeof(entries[0]) *)malloc( sizeof(typeof(entries[0])) * RUNS );
	#ifdef CNT
	counters = (typeof(counters[0]) *)malloc( sizeof(typeof(counters[0])) * RUNS );
	#endif // CNT
	for ( size_t r = 0; r < RUNS; r += 1 ) {
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
	for ( size_t r = 0; r < RUNS; r += 1 ) {
		for ( size_t id = 0; id < N; id += 1 ) {
			for ( size_t i = 0; i < CNT + 1; i += 1 ) { // reset for each run
				counters[r][id].cnts[i] = 0;
			} // for
		} // for
	} // for
//#endif // FAST
#endif // CNT

	#if defined( CONVOY )
	assert( RUNS <= 16 && N <= 128 );
	for ( size_t r = 0; r < RUNS; r += 1 ) {
		for ( size_t tid1 = 0; tid1 < N; tid1 += 1 ) {
			for ( size_t tid2 = 0; tid2 < N; tid2 += 1 ) {
				convoy[r][tid1][tid2] = 0;
			} // for
		} // for
	} // for
	#endif // CONVOY

	unsigned int set[Threads];
	for ( size_t i = 0; i < Threads; i += 1 ) set[ i ] = i;
	// srand( getpid() );
	// shuffle( set, Threads );							// randomize thread ids
	#ifdef DEBUG
	printf( "\nthread set: " );
	for ( size_t i = 0; i < Threads; i += 1 ) printf( "%u ", set[ i ] );
	printf( "\n" );
	#endif // DEBUG

	ctor();												// global algorithm constructor

	pthread_t workers[Threads];

	for ( size_t tid = 0; tid < Threads; tid += 1 ) {	// start workers
		int rc = pthread_create( &workers[tid], NULL, Worker, (void *)(size_t)set[tid] );
		if ( rc != 0 ) {
			errno = rc;
			perror( "***ERROR*** pthread create" );
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

	for ( size_t tid = 0; tid < Threads; tid += 1 ) { // terminate workers
		int rc = pthread_join( workers[tid], NULL );
		if ( rc != 0 ) {
			errno = rc;
			perror( "***ERROR*** pthread join" );
			abort();
		} // if
	} // for

	dtor();												// global algorithm destructor

	double avg = 0.0, std = 0.0, rstd;

	#ifdef DEBUG
	printf( "\nthreads:\n" );
	#endif // DEBUG
	for ( size_t r = 0; r < RUNS; r += 1 ) {
		#ifdef DEBUG
		for ( size_t tid = 0; tid < Threads; tid += 1 ) {
			printf( "%" QUOTE "ju ", entries[r][tid] );
		} // for
		#endif // DEBUG
		statistics( Threads, entries[r], &avg, &std, &rstd );
		#ifdef DEBUG
		printf( ": avg %'1.f  std %'1.f  rstd %'2.f%%\n", avg, std, rstd );
		#endif // DEBUG
	} // for

	uint64_t totalCols[RUNS];

	#ifdef DEBUG
	printf( "\nruns:\n" );
	for ( size_t tid = 0; tid < Threads; tid += 1 ) {
		for ( size_t r = 0; r < RUNS; r += 1 ) {
			totalCols[r] = entries[r][tid];				// must copy, row major order
			printf( "%" QUOTE "ju ", entries[r][tid] );
		} // for
		statistics( RUNS, totalCols, &avg, &std, &rstd );
		printf( ": avg %'1.f  std %'1.f  rstd %'2.f%%\n", avg, std, rstd );
	} // for
	#endif // DEBUG

	uint64_t sort[RUNS];

	for ( size_t r = 0; r < RUNS; r += 1 ) {
		totalCols[r] = 0;
		for ( size_t tid = 0; tid < Threads; tid += 1 ) {
			totalCols[r] += entries[r][tid];
		} // for
		sort[r] = totalCols[r];
	} // for
	statistics( RUNS, totalCols, &avg, &std, &rstd );
	const double percent = 15.0;
	if ( rstd > percent ) printf( "Warning relative standard deviation %.1f%% greater than %.0f%% over %d runs.\n", rstd, percent, RUNS );

	qsort( sort, RUNS, sizeof(typeof(sort[0])), compare );
	uint64_t med = median( sort );
	size_t posn;										// run with median result
	for ( posn = 0; posn < RUNS && totalCols[posn] != med; posn += 1 ); // assumes RUNS is odd

	#ifdef DEBUG
	printf( "\ntotals: " );
	for ( size_t i = 0; i < RUNS; i += 1 ) {			// print values
		printf( "%" QUOTE "ju ", totalCols[i] );
	} // for
	printf( "\nsorted: " );
	for ( size_t i = 0; i < RUNS; i += 1 ) {			// print values
		printf( "%" QUOTE "ju ", sort[i] );
	} // for
	printf( "\nmedian posn:%jd\n\n", posn );
	#endif // DEBUG

	printf( "%" QUOTE "ju", med );						// median round
	statistics( Threads, entries[posn], &avg, &std, &rstd ); // median thread
	printf( " %" QUOTE ".1f %" QUOTE ".1f %5.1f%%", avg, std, rstd );

	#ifdef CNT
	// posn is the run containing the median result. Other runs are ignored.
	uint64_t cntsum;
	for ( size_t i = 0; i < CNT + 1; i += 1 ) {
		cntsum = 0;
		#ifdef FAST
		for ( size_t tid = 0; tid < N; tid += 1 ) {
		#else
		for ( size_t tid = 0; tid < Threads; tid += 1 ) {
		#endif // FAST
			cntsum += counters[posn][tid].cnts[i];
		} // for
		printf( " %5.1f%%", (double)cntsum / (double)totalCols[posn] * 100.0 );
	} // for
	#endif // CNT

	#ifdef CONVOY
	printf( "\n\n" );
	for ( size_t r = 0; r < RUNS; r += 1 ) {
		for ( size_t tid1 = 0; tid1 < N; tid1 += 1 ) {
			for ( size_t tid2 = 0; tid2 < N; tid2 += 1 ) {
				#ifdef CFMT
				printf( "%" QUOTE "ju ", convoy[r][tid1][tid2] ); // posn for median
				#else
				printf( "%13ju ", convoy[r][tid1][tid2] ); // posn for median
				#endif // CFMT
			} // for
			printf( "\n" );
		} // for
		printf( "\n" );
	} // for
	#endif // CONVOY

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
