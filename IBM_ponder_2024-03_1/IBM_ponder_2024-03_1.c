/*********************************************************************
 * This code solves the 'IBM Ponder this' challenge from March 2024
 * See: research.ibm.com/haifa/ponderthis/challenges/March2024.html
 *
 * The algorithm uses a prime generator library.
 * It can be found at github.com/kimwalisch/primesieve/tree/master
 * and has to be installed before compiling.
 *
 * Usage: Usage: IBM_ponder_2024-03_1 [-v] [-m memSize] [-s startValue] n
 *  Options:
 *   -v
 *		verbose mode. Print information during the search.
 *
 *   -m memSize
 *		The size of the allocated array to rule out integers will be
 *		memSize bytes. Default is ten millions.
 *
 *   -s startValue
 *		The search will start at the given startValue. Useful if a
 *		lower bound for the true correct value is known as it will
 *		save search time.
 *
 ********************************************************************/

 
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>

#include <primesieve.h>

// Function prototypes
void initArray(int_fast64_t size);
int_fast64_t processArray(int_fast64_t offset, int_fast64_t startValueIndex,
                          int_fast64_t n, int_fast64_t size);
int_fast64_t look4StartValue(int_fast64_t startValue, int_fast64_t n, int_fast64_t size);
int_fast64_t CheckSequence(int_fast64_t initialValue, int_fast64_t n, int *iter);

/* This iterator is used by the primesieve library to generate primes
 *  one after the other.
 */
primesieve_iterator it;

/* Global array representing each tested number */
char *numberArray = NULL;

int verbose = 0; // Do we want some information while program is running?

/* Allocates (if not already done) an array of char of the given size.
 * This array represent each tested number. Each element is set to one
 * until it is ruled out by the algorithm (and then switched to zero).
 */
void initArray(int_fast64_t size) {
	if (!numberArray) {
		numberArray = malloc(sizeof(char) * size);
		if (!numberArray) {
			printf("ERROR: cannot allocate enough memory for numbers array.\n");
			exit(1);
		}
	}
	if (verbose)
		printf("Initializing numbers array...\n");
	for (int_fast64_t i = 0; i < size; i++)
		numberArray[i] = 1;
	if (verbose)
		printf("Allocation done !\n");
}

/* This is the function that does the job of eliminating integers that cannot be 
 * the initial value of the sequence. It does so by generating primes and 
 * working backwards: if p is prime, p-1, p-1-2, p-1-2-3... cannot be 
 * an correct initial value for the sequence.
 * - the global 'numberArray' is used to keep track of which integer
 *   has been eliminated, 'size' is the size of this array.
 *   If numberArray[i] == 0, it means that integer has been crossed out.
 * - That array may not be large enough for all the integers we want to try, 
 *   so blocks of integers are tested one after the other. That means element 0
 *   of the array does not represent integer 0 but rather integer of value 'offset'.
 * - 'startValueIndex' is the index from which to generate primes.
 *    If we want to check a new array (starting at real value offset), this index
 *    will be 0 but we may know that none of the first integers can be a correct initial
 *    value so we may start at an larger index.
 * - n is of course the sequence desired index. Small remark: we have n-1 iterations
 *   to do.`
 * If a correct initial value is found (ie: no tested prime has eliminated it),
 *  the function will return its index in the array (so the real value is index+offset).
 * If all integers have been ruled out, the function returns -1.
 */
 int_fast64_t processArray(int_fast64_t offset, int_fast64_t startValueIndex,
                           int_fast64_t n, int_fast64_t size) {

	int_fast64_t possibleStartIndex = startValueIndex;
	int_fast64_t primeCounter = 0;
	n--; /* There are in fact n-1 additions to do */
	int_fast64_t upperBoundDiff = n*(n+1)/2; // no need to test above
	int_fast64_t lastPrime, offsetPrime, initialOffsetPrime, i;

	// Start again from the first prime after the initial value (which is offset)
	primesieve_jump_to(&it, offset + startValueIndex, offset + size + 2*upperBoundDiff);
		
	do {
		primeCounter++;
		lastPrime = primesieve_next_prime(&it);
		if (verbose && !(primeCounter & 0xFFFFF))
			// print tested prime once in a while
			printf("Testing Prime=%" PRIdFAST64 "\n", lastPrime);
		offsetPrime = initialOffsetPrime = lastPrime - offset;
		i = 0;
		while (i <= n) { // rule out integers backwards
			offsetPrime -= (i++);
			if (offsetPrime < 0)
				break;
			if (offsetPrime >= size)
				continue;
			numberArray[offsetPrime] = 0;
		}
		// If the possible correct value has been rules out, find the smallest new one
		if (!numberArray[possibleStartIndex]) {
			do {
				possibleStartIndex++;
			} while ((possibleStartIndex < size) &&	!numberArray[possibleStartIndex]);
			if (possibleStartIndex == size)
				return -1; // We have cleared all array
		}
	} while ((possibleStartIndex + upperBoundDiff) >= initialOffsetPrime);
	return possibleStartIndex;
}

/* This function calls the previous one which will test all integers in the 
 *  current block array. If no possible starting value is found, a new array is used,
 *  representing the next block of integer, so startValue is increased by the size
 *  of the array.
 */
int_fast64_t look4StartValue(int_fast64_t startValue, int_fast64_t n, int_fast64_t size) {
	int_fast64_t correctStartIndex;
		
	while (1) {
		initArray(size);
		correctStartIndex = processArray(startValue, 0 , n, size);
		if (correctStartIndex >= 0) // Value is found!
			return correctStartIndex + startValue;
		else {
			if (verbose)
				printf("Numbers array is full, using new one.\n");
			startValue += size;
		}
	}
}

/* Main function:
 *  check arguments, initialize prime generator, compute correct start value
 *  and check its correctness.
*/
int main(int argc, char **argv) {
	int_fast64_t n;
	int_fast64_t memSize = 10000000L; // default memory size of 10 millions
	int_fast64_t startValue = 0;
	int c;

	while ((c = getopt (argc, argv, "vm:s:")) != -1) {
		switch (c) {
			case 'v':
				verbose = 1;
				break;
			case 'm':
				memSize = strtoll(optarg, NULL, 10);
				break;
			case 's':
				startValue = strtoll(optarg, NULL, 10);
				break;
			case '?':
				if (optopt == 'm' || optopt == 's')
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				fprintf (stderr, "Usage: IBM_ponder_2024_03 [-v] [-m memSize] [-s startValue] n\n");
				return 1;
			default:
				abort();
		}
	}
	if (optind+1 != argc) {
		fprintf (stderr, "Usage: IBM_ponder_2024_03 [-v] [-m memSize] [-s startValue] n\n");
		return 1;
	}

	n = strtoll(argv[optind], NULL, 10);

	primesieve_init(&it);

	if (verbose)
		printf("Looking for correct start value for n=%" PRIdFAST64 "\n", n);
	startValue = look4StartValue(startValue, n, memSize);
	if (verbose)
		printf("For n=%" PRIdFAST64 ", start value = %" PRIdFAST64 "\n\n", n, startValue);

	printf("For n=%" PRIdFAST64 ", a start value of %" PRIdFAST64 " has been found\n", n, startValue);
	printf("Verifying...\n");

	int iter;
	int_fast64_t res;
	if ((res = CheckSequence(startValue, n, &iter)))
		printf("ERROR: %" PRIdFAST64 " is prime (%" PRIdFAST64 ") at iteration %d\n", startValue, res, iter);
	else
		printf("SUCCESS! %" PRIdFAST64 " is the correct answer.\n", startValue);

	primesieve_free_iterator(&it);
	free(numberArray);
}


/*********************************************************************/

/* This function is used for verification purpose. Given an initial value A, it will
 *  check that no member of the sequence A, A+1, A+1+2,... of length n is a prime.
 * It returns 0 if the sequence is correct and the 'incorrect' prime otherwise.
 *  'iterationNbr' is used to keep track of the iteration number
 *  (so the calling code knows which An is prime).
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
