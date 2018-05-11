#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <limits.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

typedef unsigned long long long_t;

const size_t INFINITY_SQRT = 268435456;

/*
 * In this program:
 * 0 when number is prime
 * 1 otherwise
 */

long_t max_number_;
long_t num_threads_;

long_t * base_primes_;
long_t primes_count_ = 0;

pthread_t * threads_;

void memory_error() {
    printf("ERROR: Not enough memory\n");
}

void sieve_eratosthenes(long_t * numbers, long_t numbers_size) {
    numbers[0] = numbers[1] = 1;
    for (long_t i = 3; i < numbers_size; ++i) {
        if (i % 2 != 0) {
            if (numbers[i] == 0) {
                for (long_t k = i * i; k < numbers_size; k += i) {
                    numbers[k] = 1;
                }
            }
        } else {
            numbers[i] = 1;
        }
    }
    
    for (long_t i = 2; i < numbers_size; ++i) {
        if (numbers[i] == 0) {
            primes_count_++;
        }
    }
}

void set_base_prime_numbers_array(long_t numbers_size) {
    long_t * numbers = (long_t *)calloc(numbers_size, sizeof(long_t));
    if (numbers == NULL) {
        memory_error();
    }
    sieve_eratosthenes(numbers, numbers_size);
    base_primes_ = (long_t *)malloc(primes_count_ * sizeof(long_t));
    if (base_primes_ == NULL) {
        memory_error();
    }
    long_t current_number = 2;
    long_t index = 0;
    while (index < primes_count_) {
        if (numbers[current_number] == 0) {
            base_primes_[index] = current_number;
            index++;
        }
        current_number++;
    }
    free(numbers);
}

void search_the_base_prime_numbers() {
    long_t numbers_size = (max_number_ == INFINITY_SQRT) ? INFINITY_SQRT 
                                                         : (long_t)ceil(sqrt(max_number_)) + 1;
    set_base_prime_numbers_array(numbers_size);
}

void * new_threads_work(void * _thread_id) {
    int thread_id = (long_t)_thread_id;
    long_t workspace_length = max_number_ / num_threads_ + ((max_number_ % num_threads_) != 0);
    char * workspace = (char *)calloc(workspace_length, sizeof(char));
    if (workspace == NULL) {
        memory_error();
    }

    long_t left_bound = thread_id * workspace_length;
    long_t right_bound = MIN(left_bound + workspace_length - 1, max_number_);

    for (long_t i = 0; base_primes_[i] <= right_bound && i < primes_count_; ++i) {
        long_t shift = (base_primes_[i] - left_bound % base_primes_[i]) % base_primes_[i];
        for (long_t k = left_bound + shift; k <= right_bound; k += base_primes_[i]) {
            workspace[k - left_bound] = k != base_primes_[i];
        }
    }

    if (thread_id > 0) {
        pthread_join(threads_[thread_id - 1], NULL);
    }

    for (long_t i = MAX(left_bound, 2); i <= right_bound; ++i) {
        if (!workspace[i - left_bound]) {
            printf("%llu\n", i);
        }
    }
    free(workspace);
    pthread_exit(NULL);
}

int main(int argc, char ** argv) {
 
    num_threads_ = atoll(argv[1]);
    max_number_ = (argc == 2) ? INFINITY_SQRT : atoll(argv[2]);
    num_threads_ = MIN(num_threads_, max_number_ / 10);
    
    search_the_base_prime_numbers();

    threads_ = (pthread_t *)malloc(num_threads_ * sizeof(pthread_t));
    if (threads_ == NULL) {
        memory_error();
    }

    for (long_t i = 0; i < num_threads_; ++i) {
        pthread_create(&threads_[i], NULL, new_threads_work, (void *)i);
    }

    pthread_join(threads_[num_threads_ - 1], NULL);
    
    free(base_primes_);
    free(threads_);

    return 0;
}
