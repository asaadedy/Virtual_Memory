#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "pagetable.h"
#include "sim.h"


extern int debug;

extern struct frame *coremap;

addr_t *history;    // An array that consists of all referenced addresses
int position;   // Current position in the history array
int line_count; // The number of elements the history array

/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
    // Stores the position of next reference
    unsigned long int *counter = malloc(sizeof(unsigned long int)*memsize);
    int further = 0;    // The furthest position of any referenced address in the memory
    int frame = 0;  // Frame to evict
    int i, j;   // Initialize to avoid warnings

    // Initialize the position as far as possible at the beginning 
    // to simulate no more references to the address
    for(i = 0; i < memsize; i++){
        counter[i] = ULONG_MAX;
    }
    for(i = 0; i < memsize; i++){
        for(j = position + 1; j < line_count; j++){
            // If found next reference of an address, store the position 
            // and go to the next address 
            if(coremap[i].virt_addr == history[j]){
                counter[i] = j;
                break;
            }
        }
        // If the address wasn't found, it's never referensed again,
        // so evict it 
		if(counter[i] == ULONG_MAX){
			free(counter);
            return i;    
		}
    }
    // Iterate through the array and find the furthest address, save its index 
    for(i = 0; i < memsize; i++){
       if (counter[i] > further) {
           further = counter[i];
           frame = i;
       }
    }
    free(counter);
	return frame;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {
    // Move the current position in the history on each reference
    position++;
	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
    position = 0;
	char buf[MAXLINE];
	addr_t vaddr = 0;
	char type;
    line_count = 0;
    FILE *tfp;
    
	if((tfp = fopen(tracefile, "r")) == NULL) {
		perror("Error opening tracefile:");
		exit(1);
	}
    
    // Read the file once to count the number of lines, 
    // so that the history can be initialized
	while(fgets(buf, MAXLINE, tfp) != NULL) {
		if(buf[0] != '=') {
			line_count++;
		} else {
			continue;
        }
    }
    
	rewind(tfp);
	history = malloc(sizeof(addr_t)*line_count);
	int idx = 0;

    // Read the file, taken from sim.c
	while(fgets(buf, MAXLINE, tfp) != NULL) {
		if(buf[0] != '=') {
			sscanf(buf, "%c %lx", &type, &vaddr);
			history[idx] = vaddr;
			idx++;
		} else {
			continue;
		}
	}
    rewind(tfp);
     
}


