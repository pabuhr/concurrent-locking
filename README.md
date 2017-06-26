Software solutions for mutual exclusion developed over a 30 year period,
starting with complex ad-hoc algorithms and progressing to simpler formal ones.
While it is easy to dismiss software solutions for mutual exclusion, as this
family of algorithms is antiquated and most platforms support atomic
hardware-instructions, there is still a need for these algorithms in threaded,
embedded systems running on low-cost processors lacking atomic instructions.

This repository contains C-language software-solutions for mutual exclusion
using phtreads for 2-threads and N-threads.  As well, a worst-case
high-contention performance experiment is provided to compare the algorithms
and contrast them with three common locks based on hardware atomic
instructions.  Each algorithm is compiled through the file "Harness.c", which
includes the algorithm into a single source file so the compiler can see all
the code, and hence, perform maximum optimizations.  Each algorithm has a
compile command at the end of the file, which compiles the algorithm with the
test harness.

A single experiment can be run for a particular number of threads and duration,
e.g.:

$ a.out 8 20

8 20 29533193 3691649.1 656.4 0.0%

runs an experiment with: 8 threads, for 20 seconds, with 29533193 entries into
the critical section by the 8 threads, where the average number of entries is
3691649.1 with standard deviation of 656.4, and relative standard deviation
(std/avg*100) is 0%. The shell script "run1", runs 32 experiments for 1-32
threads and 20 second experiments for a pre-compiled algorithm. The shell
script "runall" compiles all the algorithms listed in the script, and uses the
"run1" script to run each of them for 1-32 threads (can take 1-2 days to
complete).


Project authors:

Peter Buhr <pabuhr@uwaterloo.ca>, David Dice <dave.dice@oracle.com> (adviser),
and Wim H. Hesselink <w.h.hesselink@rug.nl>


Project papers:

Peter A. Buhr and David Dice and Wim H. Hesselink, High-Performance N-Thread
Software Solutions for Mutual Exclusion, Concurrency and Computation: Practice
and Experience, 27(3), pp. 651-701, March, 2015

Peter A. Buhr and David Dice and Wim H. Hesselink, Dekker's Mutual Exclusion
Algorithm Made RW-Safe, Concurrency and Computation: Practice and Experience,
28(1), pp. 144-165, January, 2016
