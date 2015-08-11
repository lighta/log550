/*!
 * \file config.h
 *
 * \brief Definition / Configuration du programme
 * \author
 *	Max Moreau \n
 *	Charly Simon\n
 *	Benoit Rudelle 
 * \date 2015-03-08 19:18:00
 */ 


#ifndef CONFIG_H_
#define CONFIG_H_


#ifdef DEBUG
	#define RUN_TEST //Should we run our test or not ?
#endif

/** nom de la carte, pour inclure les bon headers */
#define BOARD EVK1100
//#include "evk1100.h" //already include in board.h if BOARD == EVK1100

/** Frequence de base de la carte */
#define F_BOARD			FOSC0

/** Valeur binaire de false */
#define FALSE           0
/** Valeur binaire de true  */
#define TRUE            1
/** Stopper l'acquisition */
#define STOP            FALSE
/** Demarer l'acquisition */
#define START           TRUE

/** Numero du channel Timer_counter 0  */
#define TC_CHANNEL_0	0
/** Numero du channel Timer_counter 1  */
#define TC_CHANNEL_1	1
/** Adresse du Timer Couter */
#define TC				(&AVR32_TC)
/** Mapper le timer d'IRQ */
#define TC_IRQ_GROUP	AVR32_TC_IRQ_GROUP
/** Nom interne pour le mappage a l'IRQ du TC0 */
#define TC_IRQ_0		AVR32_TC_IRQ0
/** Nom interne pour le mappage a l'IRQ du TC1 */
#define TC_IRQ_1		AVR32_TC_IRQ1

/************************************************************************
* GIPO88 (PX16) - PB0
*   DOC32058.pdf p.45 
*	DOC32058.pdf p.176  
* GPIO port = floor((GPIO number) / 32), example: floor((36)/32) = 1
* GPIO pin = GPIO number mod 32, example: 36 mod 32 = 4  
************************************************************************/
 /** port = GPIO_PUSH_BUTTON_0/32 (PB0 speed up) */
#define GPIO_PB0_PORT		2
/** pin = GPIO_PUSH_BUTTON_0%32 (PB0 speed up) */	
#define GPIO_PB0_PIN		24
/** port = GPIO_PUSH_BUTTON_1/32 (PB1 (Switch Mode button)) */
#define GPIO_PB1_PORT		2
/** pin = GPIO_PUSH_BUTTON_0%32 (PB1 (Switch Mode button)) */
#define GPIO_PB1_PIN		21

/**
* GPIO_PUSH_BUTTON_2/32 (PB2)
* (Start / Stop button)
*/
//#define GPIO_PB2_PORT		2
//#define GPIO_PB2_PIN		18

/** Joystick port (up) (Start / Stop button) */
#define GPIO_JS_UP_PORT		0	//	GPIO_JOYSTICK_UP / 32
/** Joystick pin (up) (Start / Stop button) */
#define GPIO_JS_UP_PIN		26	//	GPIO_JOYSTICK_UP mod 32
// #define GPIO_JS_DOWN_PORT	0	//	GPIO_JOYSTICK_DOWN / 32
// #define GPIO_JS_DOWN_PIN	27	//	GPIO_JOYSTICK_DOWN mod 32

/** Caractere pour debuter le programe */
#define CH_START 's'
/** Caractere pour arreter le programe */
#define CH_STOP 'x'

/** Frequence des LED en Hertz; 1 Hertz = 1 on/off des LED */
#define FREQ_LED						4	/*! Frequence des LED en Hertz; 1 Hertz = 1 on/off des LED */
/**  Pas d'augmentation de frequence d'adc si possible */
#define ADC_INCR	20
/**  Pas de decrementation de frequence si possible */
#define ADC_DECR	60
/**  Seuil de declenchement pour modification du RC */
#define ADC_THRESHOLD	50
/**  Seuil d'ADC pour difference par rapport a valeur minimal ou maximal */
#define ADC_SEUIL	50

/**  Nombre de donner dans FIFO avant declenchement warning UART, mode normal (0) */
#define UART_WMAX_NORM	1
/**  Nombre de donner dans FIFO avant declenchement warning UART, mode optimiser (1) */
#define UART_WMAX_OPTI	1
/** Valeur maximal du baudrate pour RS232 (en bit/s) */
#define BAUDRATE_MAX		115200
/** Valeur par default du baudrate pour RS232 (en bit/s) */
#define BAUDRATE_DEFAULT	57600
/** Valeur maximal de l'ethernet (en o/s) */
#define ETH_MAX		12500000

/** Diviseur de la clock 1 */
#define TC_CLOCK1_DIV	1
/** Diviseur de la clock 2 */
#define TC_CLOCK2_DIV	2
/** Diviseur de la clock 3 */
#define TC_CLOCK3_DIV	8
/** Diviseur de la clock 4 */
#define TC_CLOCK4_DIV	32
/** Diviseur de la clock 5 */
#define TC_CLOCK5_DIV	128

/** Precalcul frequence de l'horloge 0 */
#define FCHZ_C0  (F_BOARD / TC_CLOCK4_DIV)
/** Precalcul frequence de l'horloge 1 */
#define FCHZ_C1  (F_BOARD / TC_CLOCK2_DIV)

#endif /* CONFIG_H_ */
