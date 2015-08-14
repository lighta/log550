/*!
 * \file app_inc.h
 *
 * \brief Abstraction des cochonerie du programme
 * \author
 *	Max Moreau
 * \date 2015-08-14 19:39:23
 */

#ifndef APP_INC
	#define APP_INC
	
	#ifdef INTERUTP_MODE
		#include "fifo.h"
	#else
		#define ADC_QSIZE 10
	#endif

	#define AVR32_ADC_ADDRESS                  0xFFFF3C00	///< (ASF) Adresse ADC
	#define AVR32_ADC                          (*((volatile avr32_adc_t*)AVR32_ADC_ADDRESS))	///< (ASF) ADC
	/* Definitions of the potentiometer */
	#define ADC_POTENTIOMETER_CHANNEL   1				///< Canal potentiomètre
	#define ADC_POTENTIOMETER_PIN       AVR32_ADC_AD_1_PIN		///< Pin GPIO potentiomètre
	#define ADC_POTENTIOMETER_FUNCTION  AVR32_ADC_AD_1_FUNCTION	///< Fonction GPIO potentiomètre
	/* Definitions of the light sensor */
	#define ADC_LIGHT_CHANNEL           2				///< Canal lumière
	#define ADC_LIGHT_PIN               AVR32_ADC_AD_2_PIN		///< Pin GPIO lumière
	#define ADC_LIGHT_FUNCTION          AVR32_ADC_AD_2_FUNCTION	///< Fonction GPIO lumière
	// other shits..
	#define GPIO_PB0_PORT		2
	#define GPIO_PB0_PIN		24
	#define LED_MONO0_GREEN   0x01 //LED0
	#define LED_MONO1_GREEN   0x02 //LED1
	#define LED_MONO2_GREEN   0x04 //LED2
	#define LED_MONO3_GREEN   0x08 //LED3
	#define LED_BI0_GREEN     0x20 //LED4
	#define LED_BI0_RED       0x10 //LED5
	#define LED_BI1_GREEN     0x80 //LED6
	#define LED_BI1_RED       0x40 //LED7

	//delay de depart de toute les task pour laisser le tmps a idle de calculer son max incrementation par tick
	#define TASK_INIT_DLY ( OS_TICKS_PER_SEC*4)
	//nombre de tick necessaire pour calcul du nb_max_incr/tick, (must be < TASK_INIT_DLY)
	#define OFFSET_DLY TASK_INIT_DLY/2


	/**** Reservation du STACK de chaque tache ************************************************/
	#define OS_TASK_STK_SIZE 256
	#define MAX_TASK 8

	#define STATS_NB_TICK 10 // nombres de tick utilise pour le pourcentage


	#define GPIO_SUCCESS					0 ///< (ASF) Function successfully completed.
	#define GPIO_INVALID_ARGUMENT			1 ///< (ASF) Input parameters are out of range.
	

	
	//! Hardware descriptors of all LEDs.
	#define LED_COUNT 8
	
	#define LED0_GPIO   AVR32_PIN_PB27
	#define LED1_GPIO   AVR32_PIN_PB28
	#define LED2_GPIO   AVR32_PIN_PB29
	#define LED3_GPIO   AVR32_PIN_PB30
	#define LED4_GPIO   AVR32_PIN_PB19
	#define LED5_GPIO   AVR32_PIN_PB20
	#define LED6_GPIO   AVR32_PIN_PB21
	#define LED7_GPIO   AVR32_PIN_PB22
	#define LED0_PWM	(-1)
	#define LED1_PWM	(-1)
	#define LED2_PWM	(-1)
	#define LED3_PWM	(-1)
	#define LED4_PWM	  0
	#define LED5_PWM	  1
	#define LED6_PWM	  2
	#define LED7_PWM	  3
	#define LED0_PWM_FUNCTION   (-1)
	#define LED1_PWM_FUNCTION   (-1)
	#define LED2_PWM_FUNCTION   (-1)
	#define LED3_PWM_FUNCTION   (-1)
	#define LED4_PWM_FUNCTION   AVR32_PWM_0_FUNCTION
	#define LED5_PWM_FUNCTION   AVR32_PWM_1_FUNCTION
	#define LED6_PWM_FUNCTION   AVR32_PWM_2_FUNCTION
	#define LED7_PWM_FUNCTION   AVR32_PWM_3_FUNCTION
	
	typedef const struct {
		struct {
			CPU_INT32U PORT;	 //!< LED GPIO port.
			CPU_INT32U PIN_MASK; //!< Bit-mask of LED pin in GPIO port.
		} GPIO; //!< LED GPIO descriptor.

		struct {
			CPU_INT32S CHANNEL;  //!< LED PWM channel (< 0 if N/A).
			CPU_INT32S FUNCTION; //!< LED pin PWM function (< 0 if N/A).
		} PWM; //!< LED PWM descriptor.
	} tLED_DESCRIPTOR;
	/** A type definition of pins and modules connectivity. */
	typedef struct {
		uint32_t pin; /**< Module pin. */
		uint32_t function; /**< Module function. */
	} gpio_map_t[];
#endif
