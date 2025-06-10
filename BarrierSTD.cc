#include <barrier>

static TYPE PAD1 CALIGN __attribute__(( unused ));		// protect further false sharing
static TYPE PAD2 CALIGN __attribute__(( unused ));		// protect further false sharing

#define BARRIER_DECL static std::barrier b( N );
#define BARRIER_CALL b.arrive_and_wait()

#include "BarrierWorker.c"

void __attribute__((noinline)) ctor() {
	worker_ctor();
} // ctor

void __attribute__((noinline)) dtor() {
	worker_dtor();
} // dtor

// Local Variables: //
// compile-command: "g++-11 -Wall -Wextra -Werror -Wno-volatile -std=c++20 -O3 -DNDEBUG -fno-reorder-functions -DPIN -DAlgorithm=BarrierSTD Harness.c -lpthread -lm -D`hostname` -DCFMT" //
// End: //
