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

// Using Linux singly linked tail list as a stack
// Most recent frames get added to the tail,
// so the least recently used frame is in the head 

// Causes error when initialized in the init function, so initialized here
static STAILQ_HEAD(stailhead, entry) head = STAILQ_HEAD_INITIALIZER(head);
// static struct stailhead *headp;

// A struct to store frame and a LL
struct entry{
    int frame;
    STAILQ_ENTRY(entry) entries;
};
/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
    struct entry *new_entry = STAILQ_FIRST(&head);  // Get the head i.e. LRU frame
    int victim = new_entry->frame;	
    STAILQ_REMOVE_HEAD(&head, entries); // Remove LRU frame from the stack
    return victim;  // Return the frame index of LRU frame

}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {

    int frame = (int) p->frame >> PAGE_SHIFT;
    struct entry *queue_entry;
    struct entry *replace;

    // Check if the referenced frame is on the stack
    STAILQ_FOREACH(queue_entry, &head, entries){
        if(queue_entry->frame == frame){
            replace = queue_entry;
            // If the frame was on the stack, remove it to later add at tail
            STAILQ_REMOVE(&head, replace, entry, entries);
        }  
    }

    struct entry *new_entry = malloc(sizeof(struct entry));
    new_entry->frame = frame;
    // Add referenced frame at tail
    STAILQ_INSERT_TAIL(&head, new_entry, entries);
    return;
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
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
