#ifndef __SHARED_STRUCTS_H__
#define __SHARED_STRUCTS_H__



/** Implement your structs here */

/**
 * This structure holds the process structure information
 */
struct process_state {
	unsigned int * stack_pointer;
	unsigned int * original_sp;
	int stack_frame_size;
	struct process_state* next;
	char blocked;
};
typedef struct process_state process_t;

/** Shared function declarations **/
void enqueue (process_t * proc);


/**
 * This defines the lock structure
 */
typedef struct lock_state {
	char locked;
	struct process_state* waitlist;
} lock_t;

/**
 * This defines the conditional variable structure
 */
//typedef struct cond_var {
//
//} cond_t;

#endif
