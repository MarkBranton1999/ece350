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
 * @file        ae_mem.c
 * @brief       memory lab auto-tester
 *
 * @version     V1.2021.01
 * @authors     Yiqing Huang
 * @date        2021 JAN
 *
 *****************************************************************************/

#include "rtx.h"
#include "Serial.h"
#include "printf.h"

int test_mem(void) {

	//VARIABLES
	void *p[5];
	int n;
	U32 result = 0;

	//CASE 1: MEM_INIT() INITIALISES THE FREE BLOCK
	n = mem_count_extfrag(1070585250 );
	if (n == 1) {
		result |= BIT(0);
	}

	//CASE 2: ALLOC AND DE_ALLOC SAME NUMBER OF TIMES - in the end, it should be back to how it orignally was
	p[0] = mem_alloc(25);
	p[1] = mem_alloc(25);
	mem_dealloc(p[1]);
	p[3] = mem_alloc(25);
	mem_dealloc(p[3]);
	p[2] = mem_alloc(25);
	mem_dealloc(p[2]);
	mem_dealloc(p[0]);
	p[4] = mem_alloc(25);
	mem_dealloc(p[4]);
	n = mem_count_extfrag(1070585250);

	if (n == 1) {
		result |= BIT(1);
	}

	//CASE 3: DEALLOCATING SAME ADDRESS SHOULD GIVE ERROR
	p[0] = mem_alloc(20);
	p[1] = mem_alloc(20);
	mem_dealloc(p[1]);
	mem_dealloc(p[1]);

	if (mem_dealloc(p[1]) == RTX_ERR) {
		result |= BIT(2);
	}
	mem_dealloc(p[0]);

	//CASE 4: REUSING NEWLY FREE REGION TO ASSIGN A BIG BLOCK - first fit should assign 2nd free node available
	p[0] = mem_alloc(8);
	p[1] = mem_alloc(20);
	mem_dealloc(p[0]);
	p[2] = mem_alloc(20);

	n = mem_count_extfrag(1070585250);
	if (n == 2) {
		result |= BIT(3);
	}

	return result;
}
/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
