/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*                            ATMEL  AVR32 UC3  Application Configuration File
*
*                                 (c) Copyright 2007; Micrium; Weston, FL
*                                           All Rights Reserved
*
* File    : APP_CFG.H
* By      : Fabiano Kovalski
*********************************************************************************************************
*/


/*
**************************************************************************************************************
*                                               STACK SIZES
**************************************************************************************************************
*/

#ifndef APP_CFG
	#define APP_CFG
	#define  APP_TASK_START_STK_SIZE          256

	/*
	**************************************************************************************************************
	*                                             TASK PRIORITIES
	**************************************************************************************************************
	*/

	#define  APP_TASK_START_PRIO                1
	#define  OS_TASK_TMR_PRIO                   28


	// @TODO move me into APP_config.h
	#define DATA_COM   1
	/** Caractere pour debuter le programe */
	#define CH_START 's'
	/** Caractere pour arreter le programe */
	#define CH_STOP 'x'
	
	
	#define INTERUTP_MODE
	#define DEBUG
	
	//extra option for debug mode
	#ifdef DEBUG
		#define SEMAPHORE_DEBUG
		#define DEBUG_COM  0
	#endif
#endif
