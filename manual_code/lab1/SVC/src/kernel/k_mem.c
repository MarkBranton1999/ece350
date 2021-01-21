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

#ifdef DEBUG_0

	//check if address input is NULL
	if (!ptr) {
			return NULL;
	}

	//get the size of this block to be deleted
	//unsigned int given_address = (U32) ptr;
	unsigned int given_address = 12345;
	unsigned int given_address_block_size = given_address - (U32) SIZE_T_BYTES;

	//set up node to be inserted into the list of free blocks
	struct mem_node *deallocated_space;
	deallocated_space = (struct mem_node *)given_address;		// Assign head to start of Free Memory Space
	deallocated_space->size = given_address_block_size;	// Size should be size of Free Memory Space
	deallocated_space->next_node = NULL;

	//variables
	struct mem_node *prev_address = NULL;
	struct mem_node *next_address = NULL;
	struct mem_node *current_pointer;

	//traverse and find where this allocated block is located...
    current_pointer = head;

    printf("Address: %x\t",(U32)&current_pointer);




    while(current_pointer != NULL)
    {
    	//Error condition - if we found node in free list for this address, DANGER - DEALLOCATING FREE SPACE
    	if ((U32)&current_pointer == given_address) {
    			break;
    			return NULL;
    	}

    	//check location
    	if ((U32)&(current_pointer->next_node) > given_address) {

    		//track the adjacent free space around this allocated node
    		prev_address = current_pointer;
    		next_address = current_pointer->next_node;
    		break;
    	}
    	current_pointer = current_pointer->next_node;
    }

    //get the sizes of adjacent blocks to check against
    unsigned int prev_free_size = prev_address->size;
    unsigned int next_free_size = next_address->size;

    //insert the deallocated block node in list
    prev_address->next_node = deallocated_space;
    deallocated_space->next_node = next_address;

    unsigned int check = 0;

    //check if prev_address is free
    if ((U32)&prev_address + prev_free_size == (U32)given_address) {
    	prev_address->size = prev_address->size + deallocated_space->size;
    	prev_address->next_node = next_address;
    	check++;
    }

    //check if next_address is free
	if ((U32)given_address + given_address_block_size == (U32)&next_address) {
		next_address->size = next_address->size + deallocated_space->size;
		prev_address->next_node = next_address;
		check++;
	}

	//if both sides of deallocated block are free space, merge both
	if (check == 2) {
		prev_address->next_node = next_address->next_node;
		prev_address->size = prev_address->size + next_address->size - deallocated_space->size;
		//delete next_address;
	}

	//delete deallocated_space node;


    printf("current_pointer: %x\n", (current_pointer->size));

    printf("Before: 0x%x\r\n", (U32)(current_pointer->size));
    printf("Result: 0x%x\r\n", (U32)(current_pointer->size) + (U32) SIZE_T_BYTES);

    printf("k_mem_dealloc: freeing 0x%x\r\n", (U32) ptr);


#endif /* DEBUG_0 */

    return RTX_OK;
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