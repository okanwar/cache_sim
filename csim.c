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

//Line struct
struct Line{
	int valid_bit;
	int tag;
	int lru_num;
};

typedef struct Line Line;

//Set struct
struct Set{
	int num_lines;
	Line *lines;
};

typedef struct Set Set;

//Cache struct
struct Cache{
	int num_sets;
	Set *sets;
};

typedef struct Cache Cache;


// forward declaration
void simulateCache(char *trace_file, int num_sets, int lines_per_set, int block_size, int verbose);
void addressCalc(mem_addr addy, int *tag, int *set, int block_bits, int num_sets);
void verbosePrint( char op, mem_addr addy, int size, int resultCode);
void initCache(Cache *cache, int num_sets, int lines_per_set);
int trace(Cache *cache, mem_addr addy, int size, int block_bits, int num_sets, int *hit_count, int *miss_count, int *eviction_count);
void updateLRU(Cache *cache, int set_num, int mru_line);


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
			case 'E':
				num_lines = strtol(optarg, NULL, 10);
				break;
			case 'b':
				block_bits = strtol(optarg, NULL, 10);
				break;
			case 't':
				trace_filename = optarg;
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
void simulateCache(char *trace_file, int num_sets, int lines_per_set, int block_size, int verbose) {

	// Variables to track how many hits, misses, and evictions we've had so
	// far during simulation.
	int hit_count = 0;
	int miss_count = 0;
	int eviction_count = 0;
	mem_addr addy = 0;
	int size = 0;
	char instruct[10];
	int result_code = 0;
	Cache cache;
	initCache(&cache, num_sets, lines_per_set);
	FILE *fp;
	fp = fopen(trace_file, "r");



	while(fscanf(fp, "%s %lx, %d", instruct, &addy, &size) == 3) {
		//printf("instruct = %s current = %lx size = %d\n", instruct, addy, size);

		//Simulate cache
		switch(instruct[0]){
			
			case 'I':
				break;
			case 'L':
				result_code = trace(&cache, addy, size,block_size, num_sets, &hit_count, &miss_count, &eviction_count);
				if(verbose){
					verbosePrint( instruct[0], addy, size, result_code);
				}
				break;
			case 'S':
				result_code = trace(&cache, addy, size,block_size, num_sets, &hit_count, &miss_count, &eviction_count);
				if(verbose){
					verbosePrint( instruct[0], addy, size, result_code);
				}
				break;
			case 'M':
				//test
				result_code = trace(&cache, addy, size,block_size, num_sets, &hit_count, &miss_count, &eviction_count);
				trace(&cache, addy, size,block_size, num_sets, &hit_count, &miss_count, &eviction_count);
				if(verbose){	
					switch (result_code){
						case 1:
							verbosePrint( instruct[0], addy, size, 4);
							break;
						case 2:
							verbosePrint( instruct[0], addy, size, 6);
							break;
						case 3:
							verbosePrint( instruct[0], addy, size, 5);
							break;
						default:
							printf("default case\n");
					}
				}
				break;
			default:
				break;


		}
	}

    printSummary(hit_count, miss_count, eviction_count);

}




int trace(Cache *cache, mem_addr addy, int size, int block_bits, int num_sets, int *hit_count, int *miss_count, int *eviction_count){
	
	//Calculate set and tag
	int set_ = 0;
	int tag_ = 0;
	int lru_num = 0;
	int lru_line = 0;
	addressCalc( addy, &tag_, &set_, block_bits, num_sets);

	for( int i = 0; i < cache->sets[set_].num_lines; i++ ) { //iterates over lines in set_
		
		if( cache->sets[set_].lines[i].valid_bit == 0 ){ //valid bit is 0
			
			cache->sets[set_].lines[i].valid_bit = 1;
			cache->sets[set_].lines[i].tag = tag_;
			*miss_count = *miss_count + 1;
			updateLRU( cache, set_, i);
			return 1;

		}

		if( cache->sets[set_].lines[i].tag == tag_ ){ //valid bit is 1 check tags
			
			*hit_count = *hit_count + 1;
			return 2;
		}

	}

	//Evict
	
	//Find line to evict
	lru_num = cache->sets[set_].lines[0].lru_num;
	
	//See if there is a least recently used line
	for(int i = 0; i < cache->sets[set_].num_lines; i++ ){
	
		if( cache->sets[set_].lines[i].lru_num > lru_num ){
			lru_num = cache->sets[set_].lines[i].lru_num;
			lru_line = i;

		}
	}

	//Evict
	cache->sets[set_].lines[lru_line].tag = tag_;
	updateLRU( cache, set_, lru_line);
	*miss_count = *miss_count + 1;
	*eviction_count = *eviction_count + 1;
	return 3;

}

void updateLRU(Cache *cache, int set_num, int mru_line){
	
	//update lru values in set
	for(int i=0; i < cache->sets[set_num].num_lines; i++){
		
		if(i == mru_line){//Found mru line
			cache->sets[set_num].lines[i].lru_num = 0;
		} else { //Increment lru_num
			cache->sets[set_num].lines[i].lru_num++;
		}
	}
}

void initCache(Cache *cache, int num_sets, int lines_per_set){
	cache->num_sets = num_sets;
	cache->sets = calloc( num_sets, sizeof(Set) );

	for(int i = 0; i < cache->num_sets; i++) { //Initializes sets
		cache->sets[i].num_lines = lines_per_set;
		cache->sets[i].lines = calloc( lines_per_set, sizeof(Line) );

		for(int j = 0; j < cache->sets[j].num_lines; j++) { // Initialize lines in set
			cache->sets[i].lines[j].valid_bit = 0;
			cache->sets[i].lines[j].lru_num = (j+1);
		}
	}
}

void verbosePrint( char op, mem_addr addy, int size, int resultCode){

	switch(resultCode){
		case 1:
			printf("%c %lx,%d miss\n", op, addy, size);
			break;
		case 2:
			printf("%c %lx,%d hit\n", op, addy, size);
			break;
		case 3:
			printf("%c %lx,%d miss eviction\n", op, addy, size);
			break;
		case 4:
			printf("%c %lx,%d miss hit\n", op, addy, size);
			break;
		case 5:
			printf("%c %lx,%d miss eviction hit\n", op, addy, size);
			break;
		case 6:
			printf("%c %lx,%d hit hit\n", op, addy, size);
			break;
		default:
			printf("NULL Verbose\n");
			break;
	}
}

void addressCalc(mem_addr addy, int *tag, int *set, int block_bits, int num_sets) {
	int set_bits = log(num_sets) / log(2);
	//printf("block:%d set_bits:%d\n", block_bits, set_bits);
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
	//printf("Address calc: set:%d tag:%d\n", *set, *tag);
}

