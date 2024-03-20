/*********************************************************************
 * This code solves the 'IBM Ponder this' challenge from March 2024
 * See: research.ibm.com/haifa/ponderthis/challenges/March2024.html
 *
 * The algorithm uses a prime generator library.
 * It can be found at github.com/kimwalisch/primesieve/tree/master
 * and has to be installed before compiling.
 *
 * Usage: Usage: IBM_ponder_2024-03_2_MT [-v] [-t numThreads] [-m memSize] n
 *  Options:
 *   -v
 *		verbose mode. Print information during the search.
 *
 *   -t numThreads
 *      Uses numThreads threads to compute the results (default is 1)
 *
 *   -m memSize
 *		The size of the allocated array to rule out integers will be
 *      memSize bytes. Default is ten millions.
 *
 ********************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>

#define MAX_THREADS 64

#include <primesieve.h>

/* A bunch of global variables accessible by all threads on a read-only basis */
int verbose = 0;
char *primeArray = NULL;         /* Array of primes */
int_fast64_t n ;             /* Which X_n do we want? */
int_fast64_t memSize;        /* Size of the integers window */
int_fast64_t upperBoundDiff; /* Difference between a_0 and a_n, ie: n(n-1)/2 */
int_fast64_t globalOffset;   /* Integers window offset, ie: index 0 represent true integer 'globalOffset' */

int numThreads = 1;

/* a global variable to hold the best starting value found by a thread.
 * It will be modified by the threads when they find a possible starting value
 * for the sequence, so it must be declared volatile (so that each thread gets
 * the updated value) and protected by a mutual exclusion lock.
 */
volatile int_fast64_t bestValue = 0; 
pthread_mutex_t mutex;

/* The iterator used to generate all primes. See the primesieve library */
primesieve_iterator it;

/* Function prototypes */
void fillArrayOfPrimes(int_fast64_t memSize);
int test(int_fast64_t value);
void *greedyLoop(void *ptr);
int_fast64_t CheckSequence(int_fast64_t initialValue, int_fast64_t n, int *iterationNbr);

/*********************************************************************/

/* This function allocates (if not already done) an array of primes. The array
 *  represents integers in the range [globalOffset - globalOffset+memSize].
 *  Each prime integer is marked with a 1 in the array.
 * The array is in fact a bit larger than memSize because to be able
 *  to test integers up to globalOffset+memsize, we need to check primes
 *  up to globalOffset+memSize + upperBoundDiff
 */
void fillArrayOfPrimes(int_fast64_t memSize) {
	int_fast64_t lastPrime, pIndex;
	/* We have to allocate a bit more. */
	int_fast64_t primeSize = memSize + upperBoundDiff;
	if (!primeArray) {
		primeArray = malloc(sizeof(char) * primeSize);
		if (!primeArray) {
			printf("ERROR: cannot allocate enough memory for numbers array.\n");
			exit(1);
		}
	}
	if (verbose)
		printf("Initializing numbers array from %" PRIdFAST64 "\n", globalOffset);
	for (int_fast64_t i = 0; i < primeSize; i++)
		primeArray[i] = 0;
	if (verbose)
		printf("Allocation done !\n");

	// Start from the first prime after the offset and mark 1 for each prime
	primesieve_jump_to(&it, globalOffset, globalOffset + primeSize);
	lastPrime = primesieve_next_prime(&it);
	while ((pIndex = lastPrime - globalOffset) < primeSize) {
		primeArray[pIndex] = 1;
		lastPrime = primesieve_next_prime(&it);
	}
	if (verbose)
		printf("Primes marked !\n");
}

/* Test a value to see if it can be a starting one for the sequence.
 * It computes each value of the sequence a_i = a_i-1 + i and checks
 * whether it is a prime or not. 'globalOffset' is the initial offset
 *  of the prime array and is given with a global variable.
 */
int isCorrectValue(int_fast64_t value) {
	int_fast64_t i = 0;
	int_fast64_t valueOffset = value - globalOffset;
	while (i < n) {
		if (primeArray[(valueOffset += (i++))])
			return 0;
	}
	return 1;
}

/* This is the main loop executed by each thread.
 * The parameter is the initial starting value to check. It is equal to 
 *  the global offset plus the thread ID (from 0 to the number of threads
 *  [stored in the numThreads global variable]). 
 * The function then checks each number in the integer range and proceed
 *  with step 'numThreads'.
 * The function stops on three cases:
 *  - it has tested all integers in the range without success. The function
 *    will return -1 and the thread exits.
 *  - another thread has already found a correct starting value
 *    ['bestValue' global variable] lower than our current tested value.
 *    the thread will exit and return its current value.
 *  - the thread has found a correct value and it is lower than the current
 *    best value (or no correct value has yet been found).
 *    The thread will update the best value global variable
 *    (protected by a mutual exclusion lock) and return it.
 */
void *mainLoop(void *ptr) {
	int_fast64_t initialOffset = *(int_fast64_t *) ptr;
	int_fast64_t threadID = initialOffset - globalOffset;
    int_fast64_t *startValue = malloc(sizeof(int_fast64_t));
    int res = 0;

	*startValue = initialOffset;
    while (*startValue < memSize + globalOffset) {
    	res = isCorrectValue(*startValue);
    	if (verbose && !(*startValue & 0x7FFFFFF))
    		// print tested value once in a while
    		printf("Testing %" PRIdFAST64 "\n", *startValue);
    	if (res || (bestValue && bestValue < *startValue))
    		break;
    	*startValue += numThreads;
    }
    if (*startValue >= memSize + globalOffset) {
    	if (verbose)
	    	printf("Thread %" PRIdFAST64 " out of memory.\n", threadID);
    	*startValue = -1;
    	pthread_exit(startValue);
    }
    pthread_mutex_lock(&mutex);
    if (!bestValue || *startValue < bestValue) {
    	if (verbose)
	       	printf("Thread %" PRIdFAST64 " updates best value.\n", threadID);
	    bestValue = *startValue;
	} else {
		if (verbose)
		   	printf("Thread %" PRIdFAST64 " stops.\n", threadID);
	}
    pthread_mutex_unlock(&mutex);
    return startValue;
}

/* The main function will set up an integer range and launch several threads
 *  to test it. If no thread find a correct value, we start again with the
 *  next integer range (increasing globalOffset by memSize).
 */
int main(int argc, char **argv) {
    pthread_t ID[MAX_THREADS];
    int_fast64_t tab[MAX_THREADS];
    void *exitPtr[MAX_THREADS];
 	int i;

	memSize = 100000000L; // default memory size of 100 millions
	int c;

	while ((c = getopt (argc, argv, "vm:t:")) != -1) {
		switch (c) {
			case 'v':
				verbose = 1;
				break;
      		case 'm':
        		memSize = strtoll(optarg, NULL, 10);
        		break;
      		case 't':
        		numThreads = strtoll(optarg, NULL, 10);
       		    if ((numThreads <= 0) || (numThreads > MAX_THREADS)) {
			        printf("Number of threads has to be between 1 and %d.\n", MAX_THREADS);
        			exit(1);
        		}
	       		break;
			case '?':
        		if (optopt == 'm' || optopt == 't')
		        	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				fprintf (stderr, "Usage: greedy [-v] [-m memsize] [-t #threads] n\n");

        		return 1;
			default:
				abort();
		}
	}
    if (optind+1 != argc) {
		fprintf (stderr, "Usage: greedy [-v] [-m memsize] n\n");
		return 1;
	}

    n = strtoll(argv[optind], NULL, 10);
    upperBoundDiff = n*(n+1)/2;
    globalOffset = 0;
	primesieve_init(&it);	
    pthread_mutex_init(&mutex, NULL); /* initialize lock */

	while (!bestValue) {
	    fillArrayOfPrimes(memSize);
	    for (i = 0; i < numThreads; i++) {
    	    tab[i] = i+globalOffset;
        	pthread_create(&ID[i], NULL, mainLoop, &tab[i]);
    	}
	    for (i = 0; i < numThreads; i++) {
        	pthread_join(ID[i], &exitPtr[i]);
			if (verbose)
	        	printf("Le thread %d returns %" PRIdFAST64 ".\n", i, *(int_fast64_t *) exitPtr[i]);
        	free(exitPtr[i]);
        }
        globalOffset += memSize;
    }
	pthread_mutex_destroy(&mutex); /* destroy lock */
		
	printf("For n=%" PRIdFAST64 ", a start value of %" PRIdFAST64 " has been found\n", n, bestValue);
	printf("Verifying...\n");

	int iter;
	int_fast64_t res;
	if ((res = CheckSequence(bestValue, n, &iter)))
		printf("ERROR: %" PRIdFAST64 " is prime (%" PRIdFAST64 ") at iteration %d\n", bestValue, res, iter);
	else
		printf("SUCCESS! %" PRIdFAST64 " is the correct answer.\n", bestValue);

  	primesieve_free_iterator(&it);
  	free(primeArray);
}


/*********************************************************************/

/* This function is used for verification purpose. Given an initial value A, it will
 *  check that no member of the sequence A, A+1, A+1+2,... of length n is a prime.
 * It returns 0 if the sequence is correct and the 'incorrect' prime otherwise.
 *  'iter' is used to keep track of the iteration number (so the calling code knows
 *  which An is prime).
 */
int_fast64_t CheckSequence(int_fast64_t initialValue, int_fast64_t n, int *iterationNbr) {
	int_fast64_t nextPrime;
	*iterationNbr = 1;
	
	primesieve_jump_to(&it, initialValue, initialValue + n*(n-1));
	do {
		while ((nextPrime = primesieve_next_prime(&it)) < initialValue)
			; // Get the first prime to check
		if (nextPrime == initialValue)
			return initialValue;
		while ((initialValue += (*iterationNbr)++) < nextPrime) {
			if (*iterationNbr >= n)
				break; // get next prime
		}
		if (nextPrime == initialValue)
			return initialValue;
	} while (*iterationNbr < n);
	if (verbose)
		printf("Last check was for iteration %d, with value (%" PRIdFAST64 ")\n", *iterationNbr, initialValue);
	return 0;
}

