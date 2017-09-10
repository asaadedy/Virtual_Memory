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

// Using Linux singly linked tail list as a queue
static STAILQ_HEAD(stailhead, entry) head = STAILQ_HEAD_INITIALIZER(head);
// static struct stailhead *headp;

// A struct to store a LL and a frame index
struct entry{
    int frame;
    STAILQ_ENTRY(entry) entries;
};
/* Page to evict is chosen using the fifo algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int fifo_evict() {
    struct entry *new_entry = STAILQ_FIRST(&head);  // Get the head of the queue
    int victim = new_entry->frame;  	
    STAILQ_REMOVE_HEAD(&head, entries); // Remove the head from the queue
	return victim;  // Return the map index of the removed head
}

/* This function is called on each access to a page to update any information
 * needed by the fifo algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void fifo_ref(pgtbl_entry_t *p) {
    int frame = (int) p->frame >> PAGE_SHIFT;
    struct entry *queue_entry;
    
    int referenced = 0;
    // Check if the referenced frame was in the queue before
    STAILQ_FOREACH(queue_entry, &head, entries){
        if(queue_entry->frame == frame){
            referenced = 1;
        }  
    }
    // If the frame wasn't in the queue, add it at the tail of the queue
    if(referenced == 0){
        struct entry *new_entry = malloc(sizeof(struct entry));
        new_entry->frame = frame;
        STAILQ_INSERT_TAIL(&head, new_entry, entries);
    }
    return;
}

/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void fifo_init() {
    struct entry *n1;
    struct entry *n2;
    n1 = STAILQ_FIRST(&head);
    while (n1 != NULL) {
        n2 = STAILQ_NEXT(n1, entries);
        free(n1);
        n1 = n2;
    }
    STAILQ_INIT(&head);
}
