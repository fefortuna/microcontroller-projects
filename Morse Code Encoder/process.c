#include "3140_concur.h"
#include "utils.h"
#include "shared_structs.h"
#include "lcd_funcs.h"
#include "codec.h"
#include <fsl_device_registers.h>

process_t * process_queue = NULL;
process_t * current_process = NULL;

void enqueue (process_t * proc) {
	if (process_queue == NULL){
		process_queue = proc;
		proc->next = NULL;
	}
	else{
		process_t * tmp = process_queue;
		while (tmp->next != NULL){
			tmp = tmp->next;
		}
		tmp->next = proc;
		proc->next = NULL;
	}
}


void process_start (void){
				
	
	// Enable PIT0 and PIT1 Interrupts
	NVIC_EnableIRQ(PIT0_IRQn);
	NVIC_EnableIRQ(PIT1_IRQn);
	
		
	SIM->SCGC6 |= SIM_SCGC6_PIT_MASK; 	// Enable clock to PIT module
	
	// Setup Timer pit0
    PIT->CHANNEL[0].LDVAL = DEFAULT_SYSTEM_CLOCK/30;  // Set load value of zeroth PIT
    PIT->CHANNEL[0].TCTRL = 2; 					// Enable timer interrupt, but not timer
	
	// Setup Timer pit1
	// Note: DEFAULT_SYSTEM_CLOCK = 20971520u ~ 20MHz
    PIT->CHANNEL[1].LDVAL = DEFAULT_SYSTEM_CLOCK/1000;  // Set load value of first PIT to equal clock_freq/1000
    PIT->CHANNEL[1].TCTRL = 3; 					// Enable timer interrupt and timer
			
	
		// Activate PIT module
	PIT->MCR = 0x00; 	
	
	NVIC_SetPriority(PIT1_IRQn, 0);
	NVIC_SetPriority(PIT0_IRQn, 2);
	NVIC_SetPriority(SVCall_IRQn, 1);

	lcd_init();
	codec_init();
	
	// Begin process
	process_begin();
};


int process_create (void (*f)(void), int n){
	
	//PIT->CHANNEL[0].TCTRL = 1; // Disable interrupts
  process_t * addr = (process_t*) malloc(sizeof(process_t));  // Allocates space for PCB
	unsigned int * sp = process_stack_init (f, n); // Allocates a stack for a process, and sets up the stack's initial state
	//PIT->CHANNEL[0].TCTRL = 3; // Reenable interrupts	
	
	if (sp == NULL || addr == NULL){
    // If either allocation failed, return null
		return (-1);
	}
	else {
		addr->stack_pointer = sp;
		addr->original_sp = sp;
		addr->stack_frame_size = n;
		addr->next = NULL;
		enqueue(addr);
		return 0;
	}
};



unsigned int * process_select (unsigned int * cursp){

	// Handle current process
	if (cursp == NULL && current_process == NULL){
		// No process has started yet
    }
    else if (cursp == NULL && current_process != NULL){
        // Current process has finished running

        // Free up PCB and stack space
        PIT->CHANNEL[0].TCTRL = 1; // Disable interrupts
        process_stack_free(current_process->original_sp, current_process->stack_frame_size); //deallocate stack memory
        free(current_process); // deallocate struct memory
        PIT->CHANNEL[0].TCTRL = 3; // Reenable interrupts   
    }
	else{
		// Add currently runnning process to queue
		current_process->stack_pointer = cursp; // Update process's stack pointer
		enqueue(current_process);
	}
	
	// Handle next process to be run
	if (process_queue == NULL){
		return NULL;	// If there are no processes left to be run
	}
	else{
		current_process = process_queue;			// Set current_process to be head of queue
		process_queue = process_queue->next;		// Set head of queue to be next item in queue
		return (current_process->stack_pointer);	// return current process's stack pointer
	}
}



