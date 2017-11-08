/*
 * csim.c
 *
 * Fill in file header comment with your name(s) and a short paragraph about
 * what your program does.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "cachelab.h"
#include <math.h>

typedef unsigned long int mem_addr;

// forward declaration
void simulateCache(char *trace_file, int num_sets, int block_size, int lines_per_set, int verbose);
void addressCalc(mem_addr addy, int *tag, int *set, int block_bits, int tag_bits, int set_bits);
void updateLRU(int *cache, int index, int mru, int lines_per_set);
int  findLRU(int *cache, int index, int lines_per_set);
/**
 * Prints out a reminder of how to run the program.
 *
 * @param executable_name Strign containing the name of the executable.
 */
void usage(char *executable_name) {
	printf("Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>", executable_name);
}

int main(int argc, char *argv[]) {

	int num_lines = 0;
	int block_bits = 0;
	int verbose_mode = 0;
	int num_sets = 2;
	char *trace_filename = NULL;
	
	opterr = 0;

	// TODO: update this to support the h, b, and E options
	int c = -1;

	// Note: adding a colon after the letter states that this option should be
	// followed by an additional value (e.g. "-s 1")
	while ((c = getopt(argc, argv, "vs:E:b:t:")) != -1) {
		switch (c) {
			case 'v':
				// enable verbose mode
				verbose_mode = 1;
				break;
			case 's':
				// specify the number of sets
				// Note: optarg is set by getopt to the string that follows
				// this option (e.g. "-s 2" would assign optarg to the string "2")
				num_sets = 1 << strtol(optarg, NULL, 10);
				break;
			case 't':
				// specify the trace filename
				trace_filename = optarg;
				break;
			case 'E':
				num_lines = 1 << strtol(optarg, NULL, 10);
				break;
			case 'b':
				block_bits = 1 << strtol(optarg, NULL, 10);
				break;
			case '?':
			default:
				usage(argv[0]);
				exit(1);
		}
	}

	// TODO: When you are ready to start using the user defined options, you
	// should add some code here that makes sure they have actually specified
	// the options that are required (e.g. -t and -s are both required).

	if (verbose_mode) {
		printf("Verbose mode enabled.\n");
		printf("Trace filename: %s\n", trace_filename);
		printf("Number of sets: %d\n", num_sets);
	}

	simulateCache(trace_filename, num_sets, num_lines, block_bits, verbose_mode);

    return 0;
}

/*
 * Simulates cache with the specified organization (S, E, B) on the given
 * trace file.
 *
 * @param trace_file Name of the file with the memory addresses.
 * @param num_sets Number of sets in the simulator.
 * @param block_size Number of bytes in each cache block.
 * @param lines_per_set Number of lines in each cache set.
 * @param verbose Whether to print out extra information about what the
 *   simulator is doing (1 = yes, 0 = no).
 */
void simulateCache(char *trace_file, int num_sets, int block_size, 
						int lines_per_set, int verbose) {
	// Variables to track how many hits, misses, and evictions we've had so
	// far during simulation.
	int hit_count = 0;
	int miss_count = 0;
	int eviction_count = 0;
	int addy = 0;
	int set = 0;
	int tag = 0;
	int evict_num = 0;
	int mru = 0;
	char instruction;
	int *cache;
	//int tag_index = 0;

	cache = (int*)calloc( num_sets * lines_per_set * 3 , sizeof(int) );

	FILE *trace_f;
	trace_f = fopen(trace_file, "r");

    while( fscanf(trace_f, "%c%d", &instruction, &addy) > 0){
		addressCalc(addy, &tag, &set, block_size, 64, lines_per_set);

			//Simulate Cache
			
			//Iterates over the set the address is in cheching valid bit and tag
			for( int i = (3 * set * lines_per_set); i < (3 * (set+1)* lines_per_set); i+=3 ){
			
			// i is valid bit index
			// i+1 is tag index

				if( cache[i] == 0 ) { //Store in cache if valid bit is not set
					cache[i] = 1;
					cache[i+1] = tag;
					miss_count++;
					continue;
				}	
				else if (cache[i] == 1){ //Check tag if valid bit is set
					if(cache[+1] == tag ){
					hit_count++;
					continue;
					}
				}
			}	

		//Evict LRU
		evict_num = findLRU(cache, set, lines_per_set);
		mru = ((evict_num - (3*(set*lines_per_set)))/ 3);
		updateLRU(cache, set, mru, lines_per_set);
		miss_count++;
		eviction_count++;

	}	


	// TODO: This is where you will fill in the code to perform the actual
	// cache simulation. Make sure you split your work into multiple functions
	// so that each function is as simple as possible.

    printSummary(hit_count, miss_count, eviction_count);

}

void addressCalc(mem_addr addy, int *tag, int *set, int block_bits, int tag_bits, int set_bits) {
	block_bits = log(block_bits) / log(2);
	set_bits = log(set_bits) / log(2);
	int mask;
	*tag = (addy >> (set_bits + block_bits));
	
	if( set_bits == 0 ){
		return;
	}

	mask = 0;
	for(int i = 0; i < set_bits; i++){
		mask |= (1 << i);
	}
	*set = mask & (addy >> block_bits);
}

void updateLRU(int *cache, int set_num, int mru, int lines_per_set) {
	int index_LRU =  (3*(set_num*lines_per_set)) + 2;
	int cache_index = 0;
	for(int i = 1; i <= lines_per_set; i++) {
		cache_index = (i*3) + index_LRU;
		if(i == mru) {
			cache[cache_index] = 1;
		}
		else {
			cache[cache_index]++;
		}
	}
}

int findLRU(int *cache, int set_num, int lines_per_set) {
	int index_LRU = (3*(set_num*lines_per_set)) + 2;
	//int cache_index = 0;
	int num_LRU = index_LRU;
	for(int i = 1; i <= lines_per_set; i++) {
  		if(cache[index_LRU] < cache[num_LRU]) {
	 		num_LRU = index_LRU;
		}
	}
	return num_LRU;
}	
