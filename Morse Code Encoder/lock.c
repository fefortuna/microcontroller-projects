#include "lock.h"
#include "3140_concur.h"
#include <stdlib.h>
#include <fsl_device_registers.h>

void lock_enqueue (process_t * proc, lock_t* lock) {
	if (lock->waitlist == NULL){
		lock->waitlist = proc;
		proc->next = NULL;
	}
	else{
		process_t * tmp = lock->waitlist;
		while (tmp->next != NULL){
			tmp = tmp->next;
		}
		tmp->next = proc;
		proc->next = NULL;
	}
}

void l_init(lock_t* l){
	// Initialize a new lock at address l
	l = (lock_t*) malloc(sizeof(lock_t));
	
	l->locked = 0;
	l->waitlist = NULL;
	
}

void l_lock(lock_t* l){
	PIT->CHANNEL[0].TCTRL = 1; // Disable interrupts
	if (l->locked == 1){ // If lock is taken already
		current_process->blocked = 1; 
		lock_enqueue(current_process, l);  // Block current process and add it to the lock's waitlist
		process_blocked();
	}
	l->locked = 1;
	PIT->CHANNEL[0].TCTRL = 3; // Reenable interrupts
}

void l_unlock(lock_t* l){
	PIT->CHANNEL[0].TCTRL = 1; // Disable interrupts
	if (l->waitlist != NULL){ // If there are other processes waiting to access this,
		process_t * nxt = l->waitlist;  // Pop first item off waitlist and unblock it
		nxt->blocked = 0; 
		l->waitlist = nxt->next;
		enqueue(nxt); // Add the unblocked process to the process_queue
	}
	else{
		l->locked = 0; // if no other processes are waiting to use this resource, unlock it
	}
	PIT->CHANNEL[0].TCTRL = 3; // Reenable interrupts
}
