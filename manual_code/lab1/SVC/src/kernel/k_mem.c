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

int k_mem_init(void) {
    unsigned int end_addr = (unsigned int) &Image$$ZI_DATA$$ZI$$Limit;
    if(end_addr >= END_OF_FREE_MEM){
    	return -1;
    }
    head = (struct mem_node *)end_addr;		// Assign head to start of Free Memory Space
    if(sizeof(head) >= END_OF_FREE_MEM - end_addr){
    	return -1;
    }
    head->size = END_OF_FREE_MEM - end_addr - SIZE_T_BYTES;	// Size should be size of Free Memory Space
    head->next_node = NULL;						// Initialize next_node to NULL
#ifdef DEBUG_0
    printf("k_mem_init: image ends at 0x%x\r\n", end_addr);
    printf("k_mem_init: RAM ends at 0x%x\r\n", RAM_END);
    printf("k_mem_init: first free chunk size: %d\r\n", head->size);
    printf("k_mem_init: where is head pointing to: 0x%x\r\n", head);
#endif /* DEBUG_0 */
    return RTX_OK;
}

void* k_mem_alloc(size_t size) {
    if (size == 0)
    	return NULL;

    //byte aligned actual size you're looking for
	unsigned int actual_size = (unsigned int)((size + SIZE_T_BYTES + 3) & ~0x03);
	//if this is greater than largest possible size, return null
    if (actual_size > (END_OF_FREE_MEM - (unsigned int)&Image$$ZI_DATA$$ZI$$Limit) - SIZE_T_BYTES)
    	return NULL;

	#ifdef DEBUG_0
		printf("mem_alloc: given size: %d, actual_size: %d\r\n", size, actual_size);
	#endif /* DEBUG_0 */

    unsigned int currentAddress = (unsigned int) head;
    unsigned int prevAddress = 0;

	#ifdef DEBUG_0
			printf("mem_alloc: head address: 0x%x, head size: %d, head next node: 0x%x\r\n", currentAddress, ((struct mem_node *)currentAddress)->size, ((struct mem_node *)currentAddress)->next_node);
	#endif /* DEBUG_0 */

    //loop through all the free nodes
    while((struct mem_node *)currentAddress != NULL) {

    	//free node found code BEGIN
    	if (((struct mem_node *)currentAddress)->size >= actual_size) {

    		//let's keep track of the address we want to return
    		unsigned int returnAddress = currentAddress + SIZE_T_BYTES;

			#ifdef DEBUG_0
				printf("mem_alloc: return address: 0x%x\r\n", returnAddress);
			#endif /* DEBUG_0 */

    		//if the free chunk's size is exactly the size we're looking for
    		if (((struct mem_node *)currentAddress)->size == actual_size) {

    			//if prevAddress exists, i.e., it's not the first node on the linked list
    			if (prevAddress) {
    				((struct mem_node *)prevAddress)->next_node = ((struct mem_node *)currentAddress)->next_node;
    			}
    			//it is the first node on the linked list. In this case, just push head up so it points to NULL
    			else {
    				currentAddress += actual_size;
    				head = (struct mem_node *)currentAddress;
    			}
				#ifdef DEBUG_0
					printf("mem_alloc: I shouldn't be here\r\n");
				#endif /* DEBUG_0 */
    		}

    		//if the free chunk's size is greater than the size we're looking for
    		else if (((struct mem_node *)currentAddress)->size > actual_size) {

    			//let's keep track of the new size using the size we've set earlier
    			int newSize = ((struct mem_node *)currentAddress)->size - actual_size;
    			//let's keep track of the next node's address
    			unsigned int nextNodeAddress = (unsigned int)((struct mem_node *)currentAddress)->next_node;
    			//now let's increment the address to uninitialized territory
    			currentAddress += actual_size;
				#ifdef DEBUG_0
						printf("mem_alloc: modified address: 0x%x, modified size: %d, constant next node: 0x%x\r\n", currentAddress, newSize, nextNodeAddress);
				#endif /* DEBUG_0 */

    			//now let's set the values in that uninitialized territory
    			((struct mem_node *)currentAddress)->size = newSize;
    			((struct mem_node *)currentAddress)->next_node = (struct mem_node*)nextNodeAddress;

				#ifdef DEBUG_0
						printf("mem_alloc: current address: 0x%x, current size: %d, current next node: 0x%x\r\n", currentAddress, ((struct mem_node *)currentAddress)->size, ((struct mem_node *)currentAddress)->next_node);
				#endif /* DEBUG_0 */

    			//if not first free node, let's make sure prevAddress's next_node points to the right location
    			if (prevAddress) {
    				((struct mem_node *)prevAddress)->next_node += actual_size;
					#ifdef DEBUG_0
							printf("mem_alloc: prev address: 0x%x, prev size: %d, prev next node: 0x%x\r\n", prevAddress, ((struct mem_node *)prevAddress)->size, ((struct mem_node *)prevAddress)->next_node);
					#endif /* DEBUG_0 */
    			}
    			//if first node, move it by the right amount so it's looking at the modified current address
    			else
    				head = (struct mem_node *)currentAddress;
    		}

			#ifdef DEBUG_0
					printf("mem_alloc: return address shouldn't have changed: 0x%x\r\n", returnAddress);
			#endif /* DEBUG_0 */

    		//return the initial currentAddress + SIZE_T_BYTES
    		return (void *)returnAddress;
    	}
    	//free node found code END

		#ifdef DEBUG_0
				printf("mem_alloc: the search continues...\r\n");
		#endif /* DEBUG_0 */
    	//moving to the next free node
    	prevAddress = currentAddress;
    	currentAddress += (unsigned int)((struct mem_node *)currentAddress)->next_node;
		#ifdef DEBUG_0
				printf("mem_alloc: next step prev: 0x%x, next step current: 0x%x\r\n", prevAddress, currentAddress);
		#endif /* DEBUG_0 */
    }

    return NULL;
}

int k_mem_dealloc(void *ptr) {
#ifdef DEBUG_0
    printf("k_mem_dealloc: freeing 0x%x\r\n", (U32) ptr);
#endif /* DEBUG_0 */
    return RTX_OK;
}

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
