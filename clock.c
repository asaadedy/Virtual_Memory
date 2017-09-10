#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/queue.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

// Using Linux singly linked tail list as a circular linked list

static STAILQ_HEAD(stailhead, entry) head = STAILQ_HEAD_INITIALIZER(head);
// static struct stailhead *headp;

// A struct to store a LL and a frame index
struct entry{
    int frame;
    STAILQ_ENTRY(entry) entries;
};

// A struct that stores the position of the clock hand
static struct entry *hand_pos;

/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
    int evicted = 0;   // A flag to check whether a frame was evicted
    // struct entry *list_head = STAILQ_FIRST(&head);  // Get the head of the queue
    
    int evicted_frame;
    // If it's first eviction, set clock hand to the head
    if (hand_pos == NULL) {
        hand_pos = STAILQ_FIRST(&head);
    }
    while(evicted == 0){
        pgtbl_entry_t *hand_pte = coremap[hand_pos->frame].pte;

		// Find a frame where REF bit is 0
        if(!(hand_pte->frame & PG_REF)) {
            evicted_frame = hand_pte->frame >> PAGE_SHIFT;
            evicted = 1;
        } else {
		// If REF is 1, set it to 0
            hand_pte->frame = hand_pte->frame & ~(PG_REF);
        } 
        // If reached the last element of the LL
        if(STAILQ_NEXT(hand_pos, entries) == NULL) {
            // Cycle back to the start to the LL
            hand_pos = STAILQ_FIRST(&head);
        } else {
            // Else, the clock hand moves as usual
            hand_pos = STAILQ_NEXT(hand_pos, entries);
        }
    }    
	return evicted_frame;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
    int frame = (int) p->frame >> PAGE_SHIFT;
    struct entry *queue_entry;
    
    int referenced = 0;
    // Check if the referenced frame was in the LL before
    STAILQ_FOREACH(queue_entry, &head, entries){
        if(queue_entry->frame == frame){
            referenced = 1;
        }  
    }
    // If the frame wasn't in the LL, add it at the tail of the LL
    if(referenced == 0){
        struct entry *new_entry = malloc(sizeof(struct entry));
        new_entry->frame = frame;
        STAILQ_INSERT_TAIL(&head, new_entry, entries);
    }
    return;
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
    // Clear the list in case initialization is called twice
    // in the same process
    struct entry *n1;
    struct entry *n2;
    n1 = STAILQ_FIRST(&head);
    while (n1 != NULL) {
        n2 = STAILQ_NEXT(n1, entries);
        free(n1);
        n1 = n2;
    }
    // Initialize the list
    STAILQ_INIT(&head);
}
