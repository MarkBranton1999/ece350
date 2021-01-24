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
 * @authors     Yiqing Huang
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
    unsigned int end_addr = (unsigned int) &Image$$ZI_DATA$$ZI$$Limit;
#ifdef DEBUG_0
    printf("k_mem_init: image ends at 0x%x\r\n", end_addr);
    printf("k_mem_init: RAM ends at 0x%x\r\n", RAM_END);
#endif /* DEBUG_0 */
    if(end_addr >= END_OF_FREE_MEM){
    	return -1;
    }
    head = (struct mem_node *)end_addr;		// Assign head to start of Free Memory Space
    if(sizeof(head) >= END_OF_FREE_MEM - end_addr){
    	return -1;
    }
    head->size = END_OF_FREE_MEM - end_addr;	// Size should be size of Free Memory Space
    head->next_node = NULL;						// Initialize next_node to NULL
    return RTX_OK;
}

// MEM_ALLOC
void* k_mem_alloc(size_t size) {
#ifdef DEBUG_0
	k_mem_init();
    printf("k_mem_alloc: requested memory size = %d\r\n", size);
#endif /* DEBUG_0 */
    return NULL;
}


// MEM_DEALLOC
int k_mem_dealloc(void *ptr) {

	//check if address input is NULL
	if (!(U32)ptr) {
		#ifdef DEBUG_0
		printf("No input given - ERROR\r\n");
		#endif /* DEBUG_0 */
		return RTX_ERR;
	}

	// get address
	unsigned int given_address = (unsigned int) ptr - (unsigned int)SIZE_T_BYTES;			// actual line
    if (given_address < (unsigned int)&Image$$ZI_DATA$$ZI$$Limit || given_address >= END_OF_FREE_MEM) {
	#ifdef DEBUG_0
	printf("Address out of bounds - ERROR\r\n");
	#endif /* DEBUG_0 */
		return RTX_ERR;
    }

	// ????????????????????????????????????????????????????????
	//get the size of this block to be deleted - THIS IS STILL A QUESTION AS ALLOCATED SIZE NOT STORED IN MEMORY
    //char* given_address_block_size = (char *)((U32)given_address - SIZE_T_BYTES);
    //char* pointer = (char*)0x80302a6c;
    //char value = *pointer;
	char given_address_block_size = 12;

	//variables
	struct mem_node *prev_address = NULL;
	struct mem_node *next_address = NULL;

	//set up node to be inserted into the list of free blocks
	struct mem_node *deallocated_space = NULL;
	deallocated_space = (struct mem_node *)(given_address);		// Assign head to start of Free Memory Space
	deallocated_space->size = given_address_block_size;
	deallocated_space->next_node = NULL;

	#ifdef DEBUG_0
	printf("Deallocated location: 0x%x\r\n", given_address);
	#endif /* DEBUG_0 */


	// Before traversing the free list, there could be an allocated node right before free list!
	if ((U32)given_address < (U32)head) {

		#ifdef DEBUG_0
		printf("Yes it's before head...\r\n");
		#endif /* DEBUG_0 */

		deallocated_space->next_node = head;
	    head->size = head->size + deallocated_space->size;
		head = deallocated_space;

		#ifdef DEBUG_0
		printf("Relocated head to: 0x%x\r\n", head);
		printf(" ........................................................................................\r\n");
		#endif /* DEBUG_0 */

		return RTX_OK;
	}

	struct mem_node *current_pointer = (struct mem_node *)((unsigned int) head);

	#ifdef DEBUG_0
	printf("Starting Current Pointer: 0x%x\r\n", current_pointer);
	#endif /* DEBUG_0 */

	//traverse and find where this allocated block is located...
    while(current_pointer != NULL)
    {
    	//Error condition - if we found node in free list AREA for this address, DANGER - DEALLOCATING FREE SPACE
    	if (given_address == (U32)current_pointer) {

			#ifdef DEBUG_0
			printf("Address is a node on free list! - ERROR\r\n");
			#endif /* DEBUG_0 */

    			return RTX_ERR;
    	}

    	if (given_address < (U32)(current_pointer) + current_pointer->size) {

			#ifdef DEBUG_0
    		printf("Address is in the free space! - ERROR\r\n");
			#endif /* DEBUG_0 */

				return RTX_ERR;
		}

    	//check location and track the adjacent free space around this allocated node
    	if ((U32)(current_pointer->next_node) > given_address) {

			#ifdef DEBUG_0
			printf("Found possible location of node!\r\n");
			#endif /* DEBUG_0 */

    		prev_address = (struct mem_node *)((unsigned int)current_pointer);
    		next_address = (struct mem_node *)((unsigned int)current_pointer->next_node);
    		break;
    	}
    	current_pointer = current_pointer->next_node;
    }

	#ifdef DEBUG_0
	printf("Previous Address: 0x%x\r\n", prev_address);
	printf("Next Address: 0x%x\r\n", next_address);
	#endif /* DEBUG_0 */

    //if we did not set the previous and next free space, this means we could not find the node to be deallocated. return null
    if (!prev_address && !next_address) {
		#ifdef DEBUG_0
		printf("No address located on memory - ERROR\r\n");
		#endif
    	return RTX_ERR;
    }

    //insert the deallocated block node in list
    prev_address->next_node = (struct mem_node *)((unsigned int)deallocated_space);
    deallocated_space->next_node = (struct mem_node *)((unsigned int)next_address);

    //first check if both spaces around is empty

	if (((U32)prev_address + (U32)prev_address->size == (U32)given_address) &&
			((U32)given_address + (U32)given_address_block_size == (U32)next_address)) {

		#ifdef DEBUG_0
		printf("Left and Right of node are free\r\n");
		#endif /* DEBUG_0 */

		prev_address->next_node = next_address->next_node;
		prev_address->size = prev_address->size + next_address->size;
		deallocated_space->next_node = NULL;
		next_address->next_node = NULL;

		return RTX_OK;

	} else {
		//check if prev_address is free
		if ((U32)prev_address + (U32)prev_address->size  == (U32)given_address) {
			#ifdef DEBUG_0
			printf("Left side is free!\r\n");
			#endif /* DEBUG_0 */

			prev_address->size = prev_address->size + deallocated_space->size;
			prev_address->next_node = next_address;
			deallocated_space->next_node = NULL;

			return RTX_OK;
		}

		//check if next_address is free
		if ((U32)given_address + (U32)given_address_block_size == (U32)next_address) {

			#ifdef DEBUG_0
			printf("Right side is free!\r\n");
			#endif /* DEBUG_0 */

			next_address->size = next_address->size + deallocated_space->size;
			prev_address->next_node = next_address;
			deallocated_space->next_node = NULL;

			return RTX_OK;
		}
	}

	#ifdef DEBUG_0
	printf(" ........................................................................................\r\n");
	#endif /* DEBUG_0 */


    return RTX_ERR;
}


// EXTRACT_FRAGMENT
int k_mem_count_extfrag(size_t size) {
#ifdef DEBUG_0
    printf("k_mem_extfrag: size = %d\r\n", size);
#endif /* DEBUG_0 */
    return RTX_OK;
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
