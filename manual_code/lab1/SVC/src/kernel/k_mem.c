/*
 ****************************************************************************
 *
 *                  UNIVERSITY OF WATERLOO ECE 350 RTOS LAB
 *
 *                     Copyright 2020-2021 Yiqing Huang
 *
 *          This software is subject to an open source license and
 *          may be freely redistributed under the terms of MIT License.
 ****************************************************************************
 */

/**************************************************************************//**
 * @file        k_mem.c
 * @brief       Kernel Memory Management API C Code
 *
 * @version     V1.2021.01
 * @authors     Yiqing Huang, Shivansh Vij, Mark Branton, Aarti Vasudevan, Aishwarya Raju
 * @date        2021 JAN
 *
 * @note        skeleton code
 *
 *****************************************************************************/

/** 
 * @brief:  k_mem.c kernel API implementations, this is only a skeleton.
 * @author: Yiqing Huang
 */

#include "k_mem.h"
#include "Serial.h"
#ifdef DEBUG_0
#include "printf.h"
#endif  /* DEBUG_0 */

/*
 *==========================================================================
 *                            GLOBAL VARIABLES
 *==========================================================================
 */
// kernel stack size, referred by startup_a9.s
const U32 g_k_stack_size = KERN_STACK_SIZE;

// task kernel stacks
U32 g_k_stacks[MAX_TASKS][KERN_STACK_SIZE >> 2] __attribute__((aligned(8)));

// struct for k_mem_init's Free List nodes. Using a linked list implementation
struct mem_node {
	size_t size;							// Size of the allocated region
	struct mem_node *next_node;				// Next free memory block
};


struct mem_node *head __attribute__((aligned(8)));
unsigned int end_addr __attribute__((aligned(8)));
unsigned int max_size __attribute__((aligned(8)));

/*
 *===========================================================================
 *                            MACROS
 *===========================================================================
 */
#define END_OF_FREE_MEM  0xBFFFFFFF
#define SIZE_T_BYTES     4
/*
 *===========================================================================
 *                            FUNCTIONS
 *===========================================================================
 */

// MEM_INIT
int k_mem_init(void) {
    end_addr = (unsigned int) &Image$$ZI_DATA$$ZI$$Limit;
    if(end_addr >= END_OF_FREE_MEM){
    	return -1;
    }
    head = (struct mem_node *)end_addr;		// Assign head to start of Free Memory Space
    if(sizeof(head) >= END_OF_FREE_MEM - end_addr){
    	return -1;
    }
    max_size = END_OF_FREE_MEM - end_addr - SIZE_T_BYTES;
    head->size = max_size;	// Size should be size of Free Memory Space
    head->next_node = NULL;						// Initialize next_node to NULL
#ifdef DEBUG_0
    printf("k_mem_init: image ends at 0x%x\r\n", end_addr);
    printf("k_mem_init: RAM ends at 0x%x\r\n", RAM_END);
    printf("k_mem_init: first free chunk size: %d\r\n", head->size);
    printf("k_mem_init: where is head pointing to: 0x%x\r\n", head);
#endif /* DEBUG_0 */
    return RTX_OK;
}

// MEM_ALLOC
void* k_mem_alloc(size_t size) {
    if (size == 0)
    	return NULL;
    struct mem_node *current = head;
    struct mem_node *prev = head;
    unsigned int found = 0;
	#ifdef DEBUG_0
		printf("mem_alloc: head: 0x%x, current: 0x%x, prev: 0x%x\r\n", head, current, prev);
		printf(" ........................................................................................\r\n");
	#endif /* DEBUG_0 */

    //byte aligned actual size you're looking for (with SIZE_T_BYTES available for storing the space)
	unsigned int actual_size = (unsigned int)((size + SIZE_T_BYTES + 3) & ~0x03);

	//if this is greater than largest possible size, return null
    if (actual_size > max_size)
    	return NULL;

	#ifdef DEBUG_0
		printf("mem_alloc: given size: %d, actual_size: %d\r\n", size, actual_size);
	#endif /* DEBUG_0 */

    //loop through all the free nodes
    while(current != NULL) {
    	//free node found, let's leave the loop
    	if (current->size >= actual_size) {
    		found = 1;
    		break;
    	}
    	// free node not found, let's next the next block
    	prev = current;
    	current = current->next_node;
		#ifdef DEBUG_0
			printf("mem_alloc: head: 0x%x, current: 0x%x, prev: 0x%x\r\n", head, current, prev);
		#endif /* DEBUG_0 */
    }

    if(!found) // Didn't find anything in while loop, so nothing to allocate
    	return NULL;

    // Else we have space available
    //let's keep track of the address we want to return
    unsigned int returnAddress = (unsigned int)current + SIZE_T_BYTES;

	#ifdef DEBUG_0
		printf("mem_alloc: return address: 0x%x\r\n", returnAddress);
		printf("mem_alloc: current_address: 0x%x\r\n", (unsigned int)current);
	#endif /* DEBUG_0 */

	//Now we memorize the location of the size we're allocating to the current address
	size_t *allocated_size = (size_t *)current;

	// We need to split the block
	if (current->size > actual_size) {
		// Save the values from the current node before we overwrite it
		unsigned int new_block_size = current->size - actual_size; //actual_size contains SIZE_T_BYTES
		struct mem_node *new_block_next_node = current->next_node;

		#ifdef DEBUG_0
			printf("mem_alloc: new_block_next_node: 0x%x, new_block_size: %d\r\n", (unsigned int)new_block_next_node, new_block_size);
		#endif /* DEBUG_0 */

		// Convert prev to a number, increment it by the size being allocated, and return a pointer to that
		struct mem_node *new_block = (struct mem_node *)((unsigned int)current + actual_size);

		#ifdef DEBUG_0
			printf("mem_alloc: new_block_address: 0x%x\r\n", (unsigned int)new_block);
			printf("mem_alloc: head: 0x%x, prev: 0x%x\r\n", (unsigned int)head, (unsigned int)prev);
		#endif /* DEBUG_0 */

		// Start overwriting the current node and replacing prev
		new_block->size = new_block_size;
		new_block->next_node = new_block_next_node;
		if ((unsigned int)prev == (unsigned int)head)
			head = new_block;
		else
			prev->next_node = new_block;

		#ifdef DEBUG_0
			printf("mem_alloc: (EQUAL) head: 0x%x, prev: 0x%x, new_block: 0x%x\r\n", (unsigned int)head, (unsigned int)prev, (unsigned int)new_block);
		#endif /* DEBUG_0 */
	} else {
		// The block can be used whole, no need to split
		if ((unsigned int)prev == (unsigned int)head)
			head = current->next_node;
		else
			prev->next_node = current->next_node;
	}

	// Overwrite the SIZE_T_BYTES at the start of current
	*allocated_size = actual_size;
	#ifdef DEBUG_0
		printf("mem_alloc: allocated_size_address: 0x%x, allocated_size_value: %d\r\n", (unsigned int)allocated_size, *allocated_size);
	#endif /* DEBUG_0 */

	//Now we have a chunk of memory free at returnAddress,
	// and the SIZE_T_BYTES before it are filled with the size being allocated

	#ifdef DEBUG_0
		printf("mem_alloc: return address shouldn't have changed: 0x%x\r\n", returnAddress);
		printf("k_mem_alloc: requested memory size = %d\r\n", size);
		printf(" ........................................................................................\r\n");
	#endif /* DEBUG_0 */

    //return the initial currentAddress + SIZE_T_BYTES
    return (void *)returnAddress;
}


// MEM_DEALLOC
int k_mem_dealloc(void *ptr) {

	//check if address input is NULL
	if (!(unsigned int)ptr) {
		#ifdef DEBUG_0
			printf("No input given - ERROR\r\n");
		#endif /* DEBUG_0 */
		return RTX_ERR;
	}

	// get address where size lives (the real start of the pointer)
	size_t *size_address = (size_t *)((unsigned int)ptr - SIZE_T_BYTES);

    if ((unsigned int)size_address < end_addr || (unsigned int)size_address >= END_OF_FREE_MEM) {
		#ifdef DEBUG_0
    		printf("Address out of bounds - ERROR\r\n");
		#endif /* DEBUG_0 */
		return RTX_ERR;
    }

	//set up node to be inserted into the list of free blocks
	struct mem_node *deallocated_space = (struct mem_node *)(size_address);
	deallocated_space->size = *size_address;
	deallocated_space->next_node = NULL;

	#ifdef DEBUG_0
		printf(" ........................................................................................\r\n");
		printf("Deallocated location: 0x%x\r\n", size_address);
		printf("Deallocated SIZE: %d\r\n", deallocated_space->size);
		printf("Starting head: 0x%x\r\n", (unsigned int)head);
	#endif /* DEBUG_0 */

	// Check if head comes after the space being deallocated
	// This is guaranteed when the first allocated memory block is deallocated
	if((unsigned int)head > (unsigned int)size_address) {

		// We first check whether a merge is necessary
		if((unsigned int)size_address + deallocated_space->size == (unsigned int)head) {
		    // Merge is necessary
		    deallocated_space->next_node = head->next_node;
		    deallocated_space->size += head->size;
		    head = deallocated_space;
		} else {
		    deallocated_space->next_node = head;
		    head = deallocated_space;
		}

		#ifdef DEBUG_0
			printf("It was the first node, we're done\r\n");
			printf("head: 0x%x, head_next: 0x%x, head_size: %d\r\n", (unsigned int)head, (unsigned int)head->next_node, head->size);
		#endif /* DEBUG_0 */
		return RTX_OK;
	}

	//variables for traversal
	struct mem_node *prev_address = head;
	struct mem_node *current_address = head;

	#ifdef DEBUG_0
		printf("Starting Previous Pointer (also head): 0x%x\r\n", prev_address);
		printf("Starting Current Pointer (also head): 0x%x\r\n", current_address);
	#endif /* DEBUG_0 */

	//traverse and find the address of the block next in free memory space
    while(current_address != NULL) {
    	if ((unsigned int)current_address > (unsigned int)size_address)
    		break;

    	//Error condition - if we found node in free list AREA for this address, DANGER - DEALLOCATING FREE SPACE
    	if ((unsigned int)size_address == (unsigned int)current_address) {
    		#ifdef DEBUG_0
    			printf("Address is a node on free list! - ERROR\r\n");
    		#endif /* DEBUG_0 */

    	    return RTX_ERR;
    	}

    	if ((unsigned int)size_address < (unsigned int)(current_address) + current_address->size) {

    		#ifdef DEBUG_0
    	    	printf("Address is in the free space! - ERROR\r\n");
    		#endif /* DEBUG_0 */

    	    return RTX_ERR;
    	}

    	prev_address = current_address;
    	current_address = current_address->next_node;
		#ifdef DEBUG_0
    		printf("Previous Pointer: 0x%x\r\n", prev_address);
			printf("Current Pointer: 0x%x\r\n", current_address);
		#endif /* DEBUG_0 */
    }

    // Now we have current_address holding the address of the node after our deallocated
    // TAKE CARE OF MERGING
	if (((unsigned int)prev_address + prev_address->size == (unsigned int)size_address) &&
			((unsigned int)size_address + deallocated_space->size == (unsigned int)current_address)) {

		#ifdef DEBUG_0
		printf("Left and Right of node are free\r\n");
		#endif /* DEBUG_0 */

		prev_address->next_node = current_address->next_node;
		prev_address->size = prev_address->size + current_address->size + deallocated_space->size;
		deallocated_space->next_node = NULL;
		current_address->next_node = NULL;

	} else if ((unsigned int)prev_address + (unsigned int)prev_address->size  == (unsigned int)size_address) {

		//LEFT SIDE OF DEALLOCATED IS FREE
		#ifdef DEBUG_0
		printf("Left side is free!\r\n");
		#endif /* DEBUG_0 */

		prev_address->size = prev_address->size + deallocated_space->size;
		prev_address->next_node = current_address;
		deallocated_space->next_node = NULL;

	} else if ((unsigned int)size_address + deallocated_space->size == (unsigned int)current_address) {

		//RIGHT SIDE OF ALLOCATED IS FREE
		#ifdef DEBUG_0
		printf("Right side is free!\r\n");
		#endif /* DEBUG_0 */

		current_address->size = current_address->size + deallocated_space->size;
		prev_address->next_node = current_address;
		deallocated_space->next_node = NULL;

	} else {
	    deallocated_space->next_node = current_address;
	    prev_address->next_node = deallocated_space;
	}

	#ifdef DEBUG_0
		printf("Head Address: 0x%x\r\n", head);
		printf("Head Next: 0x%x\r\n", head->next_node);
		printf("Head Size: %d\r\n", head->size);
	#endif /* DEBUG_0 */

	#ifdef DEBUG_0
	printf(" ........................................................................................\r\n");
	#endif /* DEBUG_0 */

    return RTX_OK;
}


// EXTRACT_FRAGMENT
int k_mem_count_extfrag(size_t size) {
	#ifdef DEBUG_0
    	printf("k_mem_extfrag: size = %d\r\n", size);
	#endif /* DEBUG_0 */
    struct mem_node *start = head;
    int frags = 0;

    while(start != NULL) {
    	if(start->size < size) {
    		frags++;
    	}
    	start = start->next_node;
    }

	#ifdef DEBUG_0
    	printf("k_mem_extfrag: frags = %d\r\n", frags);
	#endif /* DEBUG_0 */
    return frags;
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
