This is my solution the the March 2024 IBM Ponder this problem.

[March 2024 IBM Ponder this problem](https://research.ibm.com/haifa/ponderthis/challenges/March2024.html)

# The problem

Let $X_n$ be the sequence of $n$ natural numbers $a_0, a_1,\ldots, a_{n-1}$ for which three properties are true:
1. $a_i=a_{i-1} + i$ for $0 < i < n$
2. None of the sequence terms is prime.
3. There is no other sequence with the above properties and a smaller initial term $a_0$.

The first such sequences are:

$X_1 = [1]$

$X_2 = [8,9]$

$X_3 = [9, 10, 12]$

$X_4 = [9, 10, 12, 15]$

$X_5 = [15, 16, 18, 21, 25]$

Your Goal: Please find the initial term $a_0$ of the sequence $X_{1000}$

A Bonus "*" will be given for finding the initial term $a_0$ of the sequence $X_{2024}$

# Algorithm 1

There is an obvious naive algorithm which consists in trying all integers as the starting point of the sequence:

1. Set $a_i=a_0$ to next integer.
2. Check if $a_i$ is prime.
    - if so, go back to step 1, trying the next integer $a_0+1$,
    - otherwise increment $i$ and compute $a_i = a_{i-1} + i$ and go back to step 2 until $i=n$, in which case $a_0$ is a valid initial term for the sequence.

But my first opinion was that it would be impracticable for large $n$ (spoiler: it is not, see below).

So I reversed the point of view and instead of starting from integers, I decided to use the prime numbers to ruled out integers:

1. Start with a prime number $p=b_0$. Rule out integer $b_0$ from starting the sequence.
2. Going backwards, compute $b_i = b_{i-1} - i$ and rule out integer $b_i$ from starting the sequence as after $i$ steps in the sequence, we would stumble on $p$ which is prime. Repeat step 2 until $i=n-1$.
3. Repeat step 1 with the next prime $p'$.

When should we stop? We know that if $a_0$ is the initial term of the sequence, the last term $a_{n-1}$ equals $a_0 + \frac{n(n-1)}{2}$. So at each step, we remember the smallest non-ruled out integer and if the considered prime number is greater than it added by $\frac{n(n-1)}{2}$, going backwards $n$ steps will not be enough to eliminate the integer which is therefore the smallest initial term. Example for $n=3$
1. $2$ rules out $2$ and $2-1=1$; $3$ is the smallest non ruled-out.
2. $3$ rules out $3$ and $2$; $4$ is the smallest.
3. $5$ rules out $5$, $4$ and $4-2=2$; $6$ is the smallest.
4. $7$ rules out $7$, $6$ and $4$; $8$ is the smallest.
5. $11$ rules out $11$, $10$ and $8$; $9$ is the smallest.
6. $13$ is greater than $9+\frac{3\times2}{2}$, so $9$ cannot be ruled out anymore and is the smallest initial term.

## Code

The code can be found in the `IBM_ponder_2024-03_1` folder.

One issue was to be able to generate prime numbers. I went on using the [prime-numbers website](http://www.prime-numbers.org/) where a list of all prime numbers up to one hundred billions can be downloaded, but:
- That proved not to be enough to solve the bonus question ($X_{2024}$),
- There are 10,000 prime numbers missing from the first file!

So I ended up installing the [primesieve library](https://github.com/kimwalisch/primesieve) and generate prime on the fly while running the code.

Another issue is how to store the array of integers while they are being ruled out as memory is not infinite and we do not know the size of the initial term. The solution is to work one block of integers $[0, m-1]$ at a time. If they are all ruled out (otherwise we have found $a_0$), we start again with the same array, now representing integers in the range $[m, 2m-1]$ and all computations start with an offset of $m$. An argument to the command sets the desired array size.

That code enabled me to compute $X_{1000}$ in a few seconds and $X_{2024}$ in a few hours (with a previous less optimized version of the code).

# Algorithm 2

So problem solved. I just wanted to bragg and show how clever my algorithm was by comparing it to the obviously inferior naive algorithm...
So I coded it (see the `IBM_ponder_2024-03_2` folder) and much to my disappointment, the naive algorithm happened to be twice to three times quicker...

## Code

The idea is pretty simple: use an array of `char` to mark primes in block $[0, m-1]$. Then try all integers in the block and check if there is any prime in their sequence (the array of primes extends a bit further to be sure to check all numbers in the sequence). If all integers have been tried without success, start again with the block $[m, 2m-1]$.

# Algorithm 3

But wait! Each integer sequence can be checked independently so this is a perfect algorithm waiting to be parallelized.

So we launch $t$ threads with each thread $i$ checking integers $i+kt$ in parallel (see the `IBM_ponder_2024-03_2_MT` folder).

## Code
There are some read-only global variables used by each thread: array of primes (and size and offset value), $n$ and the $\frac{n(n-1)}{2}$ upper bound. As they are on a read-only basis, no protection is necessary.

There is one shared variable, `bestValue` used to communicate between threads when a possible initial value is found. At each iteration, a thread will check whether another thread has found an initial value and will stop if this value is smaller than our current tested value. When a thread finds an possible initial value, it will check whether it is smaller than the current best value found so far and update it accordingly (using a mutual exclusion lock for protection). Beware that this variable has to be declared `volatile` to ensure all threads get any real-time updated value and not a former register or cache value. An argument to the command sets the desired number of threads.

That code enabled me to compute $X_{2024}$ in a 6 minutes on my 2019 iMac with a 8-cores+HT Core i9 and 7 minutes on my Arm M2 mac.






