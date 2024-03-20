/*********************************************************************
 * This code solves the 'IBM Ponder this' challenge from March 2024
 * See: research.ibm.com/haifa/ponderthis/challenges/March2024.html
 *
 * The algorithm uses a prime generator library.
 * It can be found at github.com/kimwalisch/primesieve/tree/master
 * and has to be installed before compiling.
 *
 * Usage: Usage: IBM_ponder_2024-03_2 [-v] [-m memSize] n
 *  Options:
 *   -v
 *		verbose mode. Print information during the search.
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

#include <primesieve.h>

/* This iterator is used by the primesieve library to generate primes
 *  one after the other.
 */
primesieve_iterator it;

char *primeArray = NULL;     /* Array of primes */
int_fast64_t n ;             /* Which X_n do we want? */
int_fast64_t upperBoundDiff; /* Difference between a_0 and a_n, ie: n(n-1)/2 */

int verbose = 0; // Do we want some information while program is running?

// Function prototypes
void fillArrayOfPrimes(int_fast64_t offset, int_fast64_t memSize);
int isCorrectValue(int_fast64_t offset, int_fast64_t value, int_fast64_t n);
int_fast64_t CheckSequence(int_fast64_t initialValue, int_fast64_t n, int *iterationNbr);

/* This function allocates (if not already done) an array of primes. The array
 *  represents integers in the range [offset - offset+memSize].
 *  Each prime integer is marked with a 1 in the array.
 * The array is in fact a bit larger than memSize because to be able
 *  to test integers up to offset+memsize, we need to check primes
 *  up to offset+memSize + upperBoundDiff
 */
void fillArrayOfPrimes(int_fast64_t offset, int_fast64_t memSize) {
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
		printf("Initializing numbers array from %" PRIdFAST64 "\n", offset);
	for (int_fast64_t i = 0; i < primeSize; i++)
		primeArray[i] = 0;
	if (verbose)
		printf("Allocation done !\n");

	// Start from the first prime after the offset and mark 1 for each prime
	primesieve_jump_to(&it, offset, offset + primeSize);
	lastPrime = primesieve_next_prime(&it);
	while ((pIndex = lastPrime - offset) < primeSize) {
		primeArray[pIndex] = 1;
		lastPrime = primesieve_next_prime(&it);
	}
	if (verbose)
		printf("Primes marked !\n");
}

/* Test a value to see if it can be a starting one for the sequence.
 * It computes each value of the sequence a_i = a_i-1 + i and checks
 * whether it is a prime or not.
 * 'offset' is the initial offset of the prime array.
 */
int isCorrectValue(int_fast64_t offset, int_fast64_t value, int_fast64_t n) {
	int_fast64_t i = 0;
	int_fast64_t valueOffset = value - offset;
	while (i < n) {
		if (primeArray[(valueOffset += (i++))])
			return 0;
	}
	return 1;
}

int main(int argc, char **argv) {
	int_fast64_t n, offset = 0;
	int_fast64_t memSize = 10000000L; // default memory size of 10 millions
	int_fast64_t res, startValue = 0;
	int c;

	while ((c = getopt (argc, argv, "vm:")) != -1) {
		switch (c) {
			case 'v':
				verbose = 1;
				break;
      		case 'm':
        		memSize = strtoll(optarg, NULL, 10);
        		break;
			case '?':
        		if (optopt == 'm')
		        	fprintf (stderr, "Option -m requires an argument.\n");
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				fprintf (stderr, "Usage: greedy [-v] [-m memsize] n\n");

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
	primesieve_init(&it);	
		
	/* Initialize prime array */
    fillArrayOfPrimes(0, memSize);
    while (1) {
    	/* Have we ruled out all array? If so, proceed with the next integers block */
    	if (startValue-offset >= memSize)
		    fillArrayOfPrimes(offset = startValue, memSize);
    	res = isCorrectValue(offset, startValue, n);
    	if (res)
    		break;
    	startValue++;
    }

    if (res)
		printf("For n=%" PRIdFAST64 ", a start value of %" PRIdFAST64 " has been found\n", n, startValue);
		
	printf("Verifying...\n");

	int iter;
	if ((res = CheckSequence(startValue, n, &iter)))
		printf("ERROR: %" PRIdFAST64 " is prime (%" PRIdFAST64 ") at iteration %d\n", startValue, res, iter);
	else
		printf("SUCCESS! %" PRIdFAST64 " is the correct answer.\n", startValue);
	exit(0);

	
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
//			printf("%d : %" PRIdFAST64 "\n", *iter, initialValue);
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
