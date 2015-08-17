/******************************************************************************************/
/* Demo2 de UCOS-II																	   */
/*	Programme qui lance 4 taches, chaque tache faisant clignoter un LED				 */
/*	Implique l'utilisation de semaphore, mailbox et de messageQueue.					*/
/*	Implique l'utilisation de USART1, mais sans interruption.						   */
/*																						*/
/*  Attention - Les drivers PM et INTC du ASF entre en conflit avec BSP_Init() de UCOS-II */
/******************************************************************************************/

#include "includes.h"   // Ceci inclu os_cfg.h, config des services noyaux UCOS-II...
#include "gpio_110.h"
#include <stdbool.h>
#include <stdint.h>
#include "cpu.h"
#include "app_cfg.h"
#include "app_inc.h"

//Stack for each thread (stack size doesn't have to be equal
// but seem to be >= 128 from test)
OS_STK Task1_Stk[OS_TASK_STK_SIZE];
OS_STK Task2_Stk[OS_TASK_STK_SIZE];
OS_STK Task3_Stk[OS_TASK_STK_SIZE];
OS_STK Task4_Stk[OS_TASK_STK_SIZE];
OS_STK Task5_Stk[OS_TASK_STK_SIZE];
OS_STK Task6_Stk[OS_TASK_STK_SIZE];
OS_STK Task7_Stk[OS_TASK_STK_SIZE];

#ifdef DEBUG_COM
	OS_STK Task3b_Stk[OS_TASK_STK_SIZE];
#endif

//early function declaration
void init_adc(volatile avr32_adc_t *adc);
void adc_start(volatile avr32_adc_t *adc);
void LED_Display(CPU_INT32U leds);
uint32_t gpio_enable_module(const gpio_map_t gpiomap, uint32_t size);
uint32_t gpio_enable_module_pin(uint32_t pin, uint32_t function);
void Init_IO_Usager(void); // Fonction definie localement


static void Task4_LedTimer(void *p_arg);
static void Task6_Alarm(void *p_arg);
static void Task7_Stats(void *p_arg);

#ifdef INTERUTP_MODE
	static void IRQ_PB0(void);
	static void Task1_SendData_Int(void *p_arg);
	static void IRQ_ADC_endconv(void);
	static void Task5b_ACD_int(void *p_arg);
	static void IRQ_UART_DATA(void);
	#ifdef DEBUG_COM
		static void IRQ_UART_DEBUG(void);
	#endif
#else
	static void Task1_SendData(void *p_arg);
	static void Task5_ACD(void *p_arg);
	static void Task2_HandleUartRD(void *p_arg);
	static void Task3_UARTEvent(void *p_arg);
	#ifdef DEBUG_COM
		static void Task3b_UARTEvent(void *p_arg);
	#endif
#endif

OS_EVENT *Sema_Led;					/// Semaphore  de protection des leds
OS_EVENT *Sema_UART;				/// Semaphore de protection de l'UART
OS_EVENT *Sema_Alarm;				/// Semaphore de declanchement de l'alarm
OS_EVENT *Sema_ACDacq;				/// Semaphore de protection de la variable pour continuer l'acq
OS_EVENT *Sema_ACDstart;			/// Semaphore pour demarrer l'acq

OS_EVENT *Mbox1_UART_RD;			/// Mailbox pour les command recu du UART
OS_EVENT *MsgQ1;					/// MessageQueue retenant pour les acquisitions

//variable global
CPU_INT08U ACDdouble=0;
CPU_INT08U ACDacq=0;				/// Variable d'etat si en acquisition ou non
CPU_INT32U maxOSIdleCtr;			/// Variable de calcul pour le nombre max d'incrementation du counterIdl en pure idle

#ifndef INTERUTP_MODE
	void *MyMsgQ1[ADC_QSIZE];				/// (FIFO pour les acquisitions)
#endif

#ifdef INTERUTP_MODE
	volatile CPU_INT32U pilefifo;				/*! Pile fifo pour nos valeur d'ACD */
	volatile CPU_INT08U nbbuf,nbbuf2;
	OS_EVENT *Sema_ACD_light;
	OS_EVENT *Sema_ACD_pot;
#endif
CPU_INT08U u8LedMap=0;				/// Registre d'etat des LEDs

//constant pour eviter le recalcul dans les boucle, donne la bonne frequence de refresh independant du nb de tick_per_sec
static const CPU_INT16U led_delay = OS_TICKS_PER_SEC / 10; //freq led :  we wait 200ms for next scan OS_TICKS_PER_SEC/(1000/200)/2
static const CPU_INT16U uart_delay = OS_TICKS_PER_SEC / 5; //freq uart :  we wait 200ms for next scan OS_TICKS_PER_SEC/(1000/200)
static CPU_INT16U acq_delay = OS_TICKS_PER_SEC / 500; //freq aqc :  we want 500 acq/s OS_TICKS_PER_SEC/500

#define task_init()  OSTimeDly(TASK_INIT_DLY) //blabla2



// irq pour mode interrupt
#ifdef INTERUTP_MODE
/**
 * Routine d'interutpion du bouton PB0 \n
 * Active/Desactive le doublement de vitesse d'acquisition
 (exemple simple d'interrupt)
 */
static void IRQ_PB0(void) {
	OSIntEnter();
	//gpio_clear_pin_interrupt_flag(GPIO_BTN_0);
	// is the same function in doc32119.
	AVR32_GPIO.port[GPIO_PB0_PORT].ifrc = 1 << GPIO_PB0_PIN; //read
#ifdef DEBUG_COM		 
	BSP_USART_printf(DEBUG_COM, "\r IRQ : Entering IRQ_PB0\n");
#endif		
	if(ACDdouble){
		acq_delay <<= 1; // freq_acquisition *2
		ACDdouble = 0;
	}
	else {
		acq_delay >>= 1; // freq_acquisition \2
		ACDdouble = 1;
	}
#ifdef DEBUG_COM		 
	BSP_USART_printf(DEBUG_COM, "\r IRQ : Leaving IRQ_PB0\n");
#endif		
	OSIntExit();
}

//reception depuis le port DATA
static void IRQ_UART_DATA(void)
{
	 CPU_INT08U char_recu;
	 CPU_SR cpu_sr;
	 
	OSIntEnter();
#ifdef DEBUG_COM		
	BSP_USART_printf(DEBUG_COM, "\r IRQ : Entering IRQ_UART_DATA\n");
#endif		
	// Si cette interruption est lancee par une reception (bit RXRDY=1)
	if (AVR32_USART1.csr & (AVR32_USART_CSR_RXRDY_MASK)) {
		//Lire le char recu dans registre RHR, et le stocker dans un 32bit
		char_recu = (AVR32_USART1.rhr & AVR32_USART_RHR_RXCHR_MASK)&0x7B;
		//Eliminer la source de l'IRQ, bit RXRDY (automatiquement mis a zero a la lecture de RHR)
		//(les seul char qui nous interesse sont CH_STOP et CH_START, ont peut donc masquer et eliminer les autres)
		switch (char_recu) {
			case CH_START:
				OSSemPost(Sema_ACDstart);
				break; //start : demarrer aquisation a partir de MAINTENANT tu commence a prendre les data
			case CH_STOP: 
				OS_ENTER_CRITICAL();
					ACDacq = 0;
				OS_EXIT_CRITICAL();
				break; //stop : arreter aquisition reinit du ADC
			default: break;
		}
	}

#ifdef DEBUG_COM		 
	BSP_USART_printf(DEBUG_COM, "\r IRQ : Leaving IRQ_UART_DATA\n");
#endif		
	OSIntExit();
}

#ifdef DEBUG_COM
//reception depuis le port DEBUG
static void IRQ_UART_DEBUG(void)
{
	CPU_INT08U char_recu;
        CPU_SR cpu_sr;
	
	OSIntEnter();
	#ifdef DEBUG_COM
		BSP_USART_printf(DEBUG_COM, "\r IRQ : Entering IRQ_UART_DATA\n");
	#endif
	// Si cette interruption est lancee par une reception (bit RXRDY=1)
	if (AVR32_USART0.csr & (AVR32_USART_CSR_RXRDY_MASK)) {
		//Lire le char recu dans registre RHR, et le stocker dans un 32bit
		char_recu = (AVR32_USART0.rhr & AVR32_USART_RHR_RXCHR_MASK)&0x7B;
		//Eliminer la source de l'IRQ, bit RXRDY (automatiquement mis a zero a la lecture de RHR)
		//(les seul char qui nous interesse sont CH_STOP et CH_START, ont peut donc masquer et eliminer les autres)
		switch (char_recu) {
			case CH_START: 
			//OS_ENTER_CRITICAL(); //why ?? 2015/04/11 (max)
				OSSemPost(Sema_ACDstart);
			//OS_EXIT_CRITICAL();
				break; //start : demarrer aquisation a partir de MAINTENANT tu commence a prendre les data
			case CH_STOP: 
				OS_ENTER_CRITICAL(); 
					ACDacq = 0; 
				OS_EXIT_CRITICAL();
				break; //stop : arreter aquisition reinit du ADC
			default: break;
		}
	}
	#ifdef DEBUG_COM
		BSP_USART_printf(DEBUG_COM, "\r IRQ : Leaving IRQ_UART_DATA\n");
	#endif
	OSIntExit();
}
#endif	

// IRQ pour ADC en fin d'1 conversion
static void IRQ_ADC_endconv(void)
{
	volatile avr32_adc_t *adc = &AVR32_ADC;
	CPU_INT08U adc_value_pot;
	CPU_INT08U adc_value_light;
        CPU_SR cpu_sr;
	
	OSIntEnter();
	
	/*light interrupt*/
	if( adc->sr & (1 << ADC_LIGHT_CHANNEL)) // Light channel 2
	{
		
		adc_value_light = (((adc->cdr2 >> 2) & 0xFF) | 0x01);
		
		//BSP_USART_ByteWr(DEBUG_COM,adc_value_light);
		//BSP_USART_ByteWr(DATA_COM,adc_value_light);
		
		OS_ENTER_CRITICAL();
			push_light(pilefifo,adc_value_light,nbbuf);
		OS_ENTER_CRITICAL();
		
		OSSemPost(Sema_ACD_light);
 		
	}
	
	/*potentiometer interrupt*/
	if( adc->sr & (1 << ADC_POTENTIOMETER_CHANNEL)) // potentiometer channel 1
	{
		
		adc_value_pot = ((adc->cdr1 >> 2) & 0xFE);

		//BSP_USART_ByteWr(DEBUG_COM,adc_value_pot);
		//BSP_USART_ByteWr(DATA_COM,adc_value_pot);
		
		OS_ENTER_CRITICAL();
			push_pot(pilefifo,adc_value_pot,nbbuf2);
		OS_ENTER_CRITICAL();
		
		OSSemPost(Sema_ACD_pot);
	}
	
	OSIntExit();
	
}
#endif

/**** Fonction principale main()***********************************************************/
int main(void) {
	CPU_IntDis(); /* Descative toute les interrupts pendant l'initialisation  */
	OSInit(); /* Initialise "uC/OS-II, The Real-Time Kernel"			  */

	/* Creation de toute les taches...a des priorites differentes (1<prio<25) */
	OSTaskCreate(Task6_Alarm, NULL, (OS_STK *) & Task6_Stk[OS_TASK_STK_SIZE - 1], 5);
	OSTaskCreate(Task4_LedTimer,		NULL, (OS_STK *) & Task4_Stk[OS_TASK_STK_SIZE - 1],		20); // flash LED
	OSTaskCreate(Task7_Stats,			NULL, (OS_STK *) & Task7_Stk[OS_TASK_STK_SIZE - 1],		6); // getsion des stat

#ifndef INTERUTP_MODE
	OSTaskCreate(Task5_ACD,			NULL, (OS_STK *) & Task5_Stk[OS_TASK_STK_SIZE - 1],		13); //get aquisition
	OSTaskCreate(Task1_SendData, NULL, (OS_STK *) & Task1_Stk[OS_TASK_STK_SIZE - 1], 7);
	OSTaskCreate(Task3_UARTEvent,		NULL, (OS_STK *) & Task3_Stk[OS_TASK_STK_SIZE - 1],		8);	//get uart char COM DATA
	#ifdef DEBUG_COM
		OSTaskCreate(Task3b_UARTEvent,		NULL, (OS_STK *) & Task3b_Stk[OS_TASK_STK_SIZE - 1],	9); //get uart char COM DEBUG
	#endif
	OSTaskCreate(Task2_HandleUartRD,	NULL, (OS_STK *) & Task2_Stk[OS_TASK_STK_SIZE - 1],		11); //start stop acq 
#else
	OSTaskCreate(Task1_SendData_Int,	NULL, (OS_STK *) & Task1_Stk[OS_TASK_STK_SIZE - 1],		7);
    OSTaskCreate(Task5b_ACD_int, NULL, (OS_STK *) & Task5_Stk[OS_TASK_STK_SIZE - 1], 13); //start aquisition
#endif
	
	Sema_Led = OSSemCreate(1);					//On authorize un unique acces a la fois
	//Sema_UART = OSSemCreate(1);				//On authorize un unique acces a la fois
	Sema_Alarm = OSSemCreate(0);				//On bloque, on veut un declenchement pour continuer
	Sema_ACDacq = OSSemCreate(1);				//On authorize un unique acces a la fois
	Sema_ACDstart = OSSemCreate(0);				//On bloque, on veut un declenchement pour continuer
	Mbox1_UART_RD = OSMboxCreate(NULL);			//MailBox,   initialisation  start uar handler
	
#ifndef INTERUTP_MODE
	MsgQ1 = OSQCreate(MyMsgQ1, ADC_QSIZE);	// MsgQueu,   initialisation  fifo declache
#endif

#ifdef INTERUTP_MODE
	Sema_ACD_light = OSSemCreate(0);
	Sema_ACD_pot = OSSemCreate(0);
#endif
	
	
	OSStart(); /* Demarre le multitasking (Donne le controle au noyau uC/OS-II)  */
	// Le code ici ne sera jamais execute
	return (0); /* Pour eviter le warnings en GCC, prototype (int main (void))	*/
}

/******************************************************************************************/
/* Definition des taches sous UCOS-II													 */
/*  - Toutes les taches ont une priorite differente									   */
/*  - Le scheduler de UCOS-II execute TOUJOURS la tache la plus prioritaire qui est READY.*/
/******************************************************************************************/

#ifndef INTERUTP_MODE

/**
 * Task1_SendData == UART_ SendSample ()
 * @brief Vide le MessageQueue et envoi les échantillons vers le UART pour une transmission vers le PC.
 * Utilise le COM DATA
 */
static void Task1_SendData(void *p_arg) {
	(void) p_arg; // Pour eviter le warnings en GCC
	CPU_INT08U err, MsgQDataRX;

	task_init();
#ifdef DEBUG_COM
	BSP_USART_printf(DEBUG_COM, "\r 1 : Entering Task1_SendData\n");
#endif
	while (1) { // Tache, une boucle infinie.
		MsgQDataRX = *((CPU_INT08U *) OSQPend(MsgQ1, 0, &err)); // Attend le MessageQueue
		BSP_USART_ByteWr(DATA_COM, MsgQDataRX);
	}
}

/**
 * Task5_ACD == ADC_Cmd ()
 * ACD = ADC
 * Cette tache demarre les conversions, obtient les echantillons numerises, et les place dans
 * le MessageQ a la bonne vitesse d’acquisition.
 * Si le MessageQueue est plein, envoi l’information à la tache AlarmMsgQ().
 */
static void Task5_ACD(void *p_arg) {
	(void) p_arg;
	
	//@TODO 32 group me ??
	CPU_INT08U err;
	CPU_INT08U adc_value_pot;
	CPU_INT08U adc_value_light;
	volatile CPU_INT08U locACD;
	
	volatile avr32_adc_t *adc;

	task_init();
#ifdef DEBUG_COM		
	BSP_USART_printf(DEBUG_COM, "\r 5 : Entering Task5_ACD\n");
#endif
	locACD = 1;
	adc = &AVR32_ADC;

	while (1) {
		OSSemPend(Sema_ACDstart, 0, &err);
		locACD = 1;
		OSSemPend(Sema_ACDacq, 0, &err);
			ACDacq = 1;
		OSSemPost(Sema_ACDacq);
		
		OSSemPend(Sema_Led, 0, &err);
			u8LedMap |= (u8LedMap << 1)&0x3;
		OSSemPost(Sema_Led);
	
		while (locACD == 1) {
			//Start fetching the data
			if ( adc->sr & (1 << ADC_LIGHT_CHANNEL) &&  adc->sr & (1 << ADC_POTENTIOMETER_CHANNEL)) // Light channel 2
			{
				/*light interrupt*/
				adc_value_light = (((adc->cdr2 >> 2) & 0xFF) | 0x01);
				/*potentiometer interrupt*/
				adc_value_pot = ((adc->cdr1 >> 2) & 0xFE);
				
				//BSP_USART_printf(DEBUG_COM, "%d",adc_value_light);
				if (OS_ERR_Q_FULL == OSQPost(MsgQ1, &adc_value_light))
					OSSemPost(Sema_Alarm); // Signaler dépassement
				
				//BSP_USART_printf(DEBUG_COM, "%d",adc_value_pot);
				if (OS_ERR_Q_FULL == OSQPost(MsgQ1, &adc_value_pot))
					OSSemPost(Sema_Alarm); // Signaler dépassement
			} else {
				// 				OSSemPend(Sema_Led, 0, &err);
				// 					u8LedMap |= LED_MONO3_GREEN; //depassement adc
				// 				OSSemPost(Sema_Led);
			}
			adc_start(adc);
			
			OSSemPend(Sema_ACDacq, 0, &err);
				locACD = ACDacq;
			OSSemPost(Sema_ACDacq);
			OSTimeDly(acq_delay); //autocal independant du nombre de tick_per_sec pour 500 acq
		}
		
		//fin du mode d'acquisition on eteint puis repasse en attente
		OSSemPend(Sema_Led, 0, &err);
			u8LedMap &= LED_MONO1_GREEN;
		OSSemPost(Sema_Led);
	}
}

/**
 * Task2_HandleUartRD == UART_Cmd_RX () (partie traitement)
 * @brief Traite les command recue par Task2_HandleUartRD ou ISR.
 * Si une commande est reçue, traiter celle-ci et envoyer l’ordre d’arrêt ou de démarrage à la tâche ADC_Cmd().
 */
static void Task2_HandleUartRD(void *p_arg) {
	(void) p_arg;
	CPU_INT08U err, MboxDataRX, prev;

	task_init();
#ifdef DEBUG_COM
	BSP_USART_printf(DEBUG_COM, "\r 2 : Entering Task2_HandleUartRD\n");
#endif
	prev = CH_STOP;
	while (1) {
		#ifdef SEMAPHORE_DEBUG
			BSP_USART_printf(DEBUG_COM, "\r 5b : Pending Mbox1_UART_RD\n");
		#endif
		MboxDataRX = *((CPU_INT08U *) OSMboxPend(Mbox1_UART_RD, 0, &err)); // Attend le mailbox
		if (err == OS_ERR_NONE) {
			if (MboxDataRX == CH_START || MboxDataRX == CH_STOP) {
				if (MboxDataRX != prev) { //precedent command asked was same, nothing todo
					if (MboxDataRX == CH_START) {
						//allow task to start+
						#ifdef SEMAPHORE_DEBUG
							BSP_USART_printf(DEBUG_COM, "\r 2 : Post ADCstart\n");
						#endif		
						OSSemPost(Sema_ACDstart); //start ACD thread
						// else there was a error to signal semaphore
					} else { //if CH_STOP
						OSSemPend(Sema_ACDacq, 0, &err);
							ACDacq = 0;
						OSSemPost(Sema_ACDacq);
						//else there was a error to signal semaphore
					}
					prev = MboxDataRX;
				}
			}
		}
		//else there was a freaking error on msgbox
	} //end while
}



/**
 * Task3_UARTEvent == UART_Cmd_RX () (partie acquisition)
 * @brief Vérifie, au 200msec, si des commandes sont reçues par le UART. (com DATA)
 * Si un char est recus transmettre pour traitement a Task2_HandleUartRD.
 */
static void Task3_UARTEvent(void *p_arg) {
	(void) p_arg;
	CPU_INT08U MboxDataPost;
	//CPU_INT08U err;

	task_init();
#ifdef DEBUG_COM
	BSP_USART_printf(DEBUG_COM, "\r 3 : Entering Task3_UARTEvent com=%d\n", DATA_COM);
#endif
	while (1) {
		MboxDataPost = BSP_USART_ByteRd(DATA_COM);
		#ifdef SEMAPHORE_DEBUG
			BSP_USART_printf(DEBUG_COM, "\r 3 : Pending Mbox1_UART_RD\n");
		#endif
		OSMboxPost(Mbox1_UART_RD, &MboxDataPost);
		OSTimeDly(uart_delay); // we wait 200ms for next scan OS_TICKS_PER_SEC/(1000/200)
	}
}



#ifdef DEBUG_COM

static void Task3b_UARTEvent(void *p_arg) {
	(void) p_arg;
	CPU_INT08U MboxDataPost;
	//CPU_INT08U err;

	task_init();
	BSP_USART_printf(DEBUG_COM, "\r 3B : Entering Task3b_UARTEvent com=%d\n", DEBUG_COM);

	while (1) {
		MboxDataPost = BSP_USART_ByteRd(DEBUG_COM);
		#ifdef SEMAPHORE_DEBUG
			BSP_USART_printf(DEBUG_COM, "\r 3b : Pending Mbox1_UART_RD\n");
		#endif
		OSMboxPost(Mbox1_UART_RD, &MboxDataPost);
		OSTimeDly(uart_delay); // we wait 200ms for next scan OS_TICKS_PER_SEC/(1000/200)
	}
}
#endif //DEBUG_COM

#endif //INTERUTP_MODE

#ifdef INTERUTP_MODE

/**
 * Task1_SendData == UART_ SendSample ()
 * @brief Vide le MessageQueue et envoi les échantillons vers le UART pour une transmission vers le PC.
 * Utilise le COM DATA
 */
static void Task1_SendData_Int(void *p_arg) {
	(void) p_arg; // Pour eviter le warnings en GCC
	CPU_INT08U err, MsgQDataRX;
	
	CPU_SR cpu_sr;

	task_init();
	#ifdef DEBUG_COM
		BSP_USART_printf(DEBUG_COM, "\r 1 : Entering Task1_SendData\n");
	#endif
	while (1) { // Tache, une boucle infinie.	
		OSSemPend(Sema_ACD_light,0,&err);
		OS_ENTER_CRITICAL();
			MsgQDataRX = pop_light(pilefifo,nbbuf);
		OS_ENTER_CRITICAL();
		BSP_USART_ByteWr(DATA_COM, MsgQDataRX);
	
		OSSemPend(Sema_ACD_pot,0,&err);
		OS_ENTER_CRITICAL();
			MsgQDataRX = pop_pot(pilefifo,nbbuf2);
		OS_ENTER_CRITICAL();
		BSP_USART_ByteWr(DATA_COM, MsgQDataRX);
	}
}

/**
 * Task5_ACD == ADC_Cmd ()
 * ACD = ADC
 * Cette tache demarre les conversions....
 */
static void Task5b_ACD_int(void *p_arg) {
	(void) p_arg;
	
	//@TODO 32 group me ??
	CPU_INT08U err;
	volatile CPU_INT08U locACD;
	CPU_SR cpu_sr;
	
	volatile avr32_adc_t *adc;

	task_init();
#ifdef DEBUG_COM
	BSP_USART_printf(DEBUG_COM, "\r 5b : Entering Task5b_ACD_intc\n");
#endif
	adc = &AVR32_ADC;
	
	while (1) {
		#ifdef SEMAPHORE_DEBUG
			BSP_USART_printf(DEBUG_COM, "\r 5b : Pending ADCstart\n");
		#endif
		OSSemPend(Sema_ACDstart, 0, &err);
		OS_ENTER_CRITICAL();
			ACDacq = 1;
			nbbuf = 0;
			nbbuf2 = 0;
		OS_ENTER_CRITICAL();
		locACD = 1;
		
		OSSemPend(Sema_Led, 0, &err);
			u8LedMap |= (u8LedMap << 1)&0x3;
		OSSemPost(Sema_Led);
		
		while (locACD == 1) {	
			adc_start(adc);
			//bloqué pour communication avec l'UART
			OS_ENTER_CRITICAL();
				locACD = ACDacq;
			OS_ENTER_CRITICAL();
			OSTimeDly(acq_delay);
		}
		
		//fin du mode d'acquisition on eteint puis repasse en attente
		OSSemPend(Sema_Led, 0, &err);
			u8LedMap &= LED_MONO1_GREEN;
		OSSemPost(Sema_Led);
	}

}
#endif

/**
 * Task6_Alarm == AlarmMsgQ()
 * Cette tâche se réveille uniquement si le MessageQ déborde, et elle commande l’allumage du Led3 en informant la tache LED_flash().
 */
static void Task6_Alarm(void *p_arg) {
	(void) p_arg;
	CPU_INT08U err;
	CPU_INT32U startOSIdleCtr;
	CPU_INT32U maxOSIdleCtr_temp;
	CPU_INT32U maxOSIdleCtr_temp_2;
	CPU_SR cpu_sr;
	OS_CPU_SR os_cpu_sr;

	BSP_Init(); // Set le timer et demarre les ISR_TICKS.
	// Doit absolument etre appele par la premiere tache a operer.
	// Attention, desormais on roule a 48MHz et USART a 48/2MHz

	CPU_IntDis(); // Desactive les IRQ pendant l'initialisation
	Init_IO_Usager(); // Initialisation des differents I/O par l'usager
	CPU_IntEn(); // Reactive les IRQ
	
	OSTimeDly(OFFSET_DLY);
	OS_ENTER_CRITICAL();
	OSIdleCtr = 0L;
	OS_EXIT_CRITICAL();
	OSTimeDly(OS_TICKS_PER_SEC);
	OS_ENTER_CRITICAL();
	maxOSIdleCtr_temp = OSIdleCtr;
	maxOSIdleCtr_temp_2 = maxOSIdleCtr_temp/OS_TICKS_PER_SEC;
	maxOSIdleCtr = (((maxOSIdleCtr_temp*10)/OS_TICKS_PER_SEC)+5)/10; // *10 +5 /10 est pour faire un round up
	OS_EXIT_CRITICAL();
	
#ifdef DEBUG_COM
	BSP_USART_printf(DEBUG_COM, "\r 6 : Finish init of Task6_Alar mmax:  %u  max_r %u max_1s:  %u \n", maxOSIdleCtr_temp_2, maxOSIdleCtr, maxOSIdleCtr_temp);
#endif 
	while (1) {
		OSSemPend(Sema_Alarm, 0, &err); // Attendre dépassement
#ifdef DEBUG_COM
		BSP_USART_printf(DEBUG_COM, "\r 6 : ALARM ALARM !!!\n");
#endif
		OSSemPend(Sema_Led, 0, &err);
		//OS_ENTER_CRITICAL();
			u8LedMap |= LED_MONO2_GREEN; //depassement uart
		//OS_ENTER_CRITICAL();
		OSSemPost(Sema_Led);
	}
}

/**
 * Thread de Benoit (stats)
@TODO merge me in Task4
 */
static void Task7_Stats(void *p_arg) {
	(void) p_arg;
	CPU_INT08U err;
	CPU_INT32U perCPU = 0;
	CPU_INT32U IdleCountTmp, OSIdleCtrTmp;

	task_init();
#ifdef DEBUG_COM	
	BSP_USART_printf(DEBUG_COM, "\r 7 : Entering Task7_Stats\n");
#endif	

	while (1) {
		
		OSTimeDly(led_delay*2); // led_delay est le min de rafraichissement (non visible apres car sur led)
		IdleCountTmp = OSIdleCtr;// important reinitialise pour le prochin compteur
		OSTimeDly(STATS_NB_TICK);
		OSIdleCtrTmp = OSIdleCtr;
		perCPU = 100 - (((OSIdleCtrTmp - IdleCountTmp)*100 )/ (maxOSIdleCtr * STATS_NB_TICK));
#ifdef DEBUG_COM				
		BSP_USART_printf(DEBUG_COM, "\r val_d:%u val_f: %u ,pCPU %3u %% \n",IdleCountTmp,OSIdleCtrTmp,perCPU);		
#endif		
		OSSemPend(Sema_Led, 0, &err);
		switch (perCPU / 20) {
			//5 bi0_green
			//6 bi1_green
			//7 bi0_red
			//8 bi1_red
			// LED_BI0_GREEN|LED_BI0_RED|LED_BI1_GREEN|LED_BI1_RED
			case 0: /* CPU entre 0-20 %. ==> _.V  */
				u8LedMap &= ~(LED_BI0_GREEN | LED_BI0_RED | LED_BI1_RED); //on eteint
				u8LedMap |= LED_BI1_GREEN; //on alume		 
				break;
			case 1: /* CPU entre 20-40 %. ==> V._ */
				u8LedMap &= ~(LED_BI0_RED | LED_BI1_GREEN | LED_BI1_RED); //on eteint 
				u8LedMap |= LED_BI0_GREEN; //on alume
				break;
			case 2: /* CPU entre 40-60 %. ==> V.V */
				u8LedMap &= ~(LED_BI0_RED | LED_BI1_RED); //on eteint
				u8LedMap |= LED_BI0_GREEN | LED_BI1_GREEN; //on alume
				break;
			case 3: /* CPU entre 60-80 %. ==> _.R */
				u8LedMap &= ~(LED_BI0_GREEN | LED_BI0_RED | LED_BI1_GREEN); //on eteint
				u8LedMap |= LED_BI1_RED; //on alume
				break;
			case 4: /* CPU entre 80-100 % ==> R._ */
				u8LedMap &= ~(LED_BI0_GREEN | LED_BI1_GREEN | LED_BI1_RED); //on eteint
				u8LedMap |= LED_BI0_RED; //on alume
				break;
			case 5: /* CPU a 100% ==> R.R */
				u8LedMap &= ~(LED_BI0_GREEN | LED_BI1_GREEN); //on eteint
				u8LedMap |= LED_BI0_RED | LED_BI1_RED; //on alume
				break;
		}
		OSSemPost(Sema_Led);	
	}
}

/**
 * Task4_LedTimer == LED_flash ()
 * @brief Effectue le clignotement des LEDS au 200msec.
 * Led1, clignote toujours des que votre programme demarre
 * Led2, clignote lorsque l’acquisition est en service
 * Led3, alarme, s’allume si le MessageQueue deborde et demeure allume.
 * Extend
 *  Calcule le pourcentage d'utilisation du CPU
 */
static void Task4_LedTimer(void *p_arg) {
	(void) p_arg;
	CPU_INT08U err;
	CPU_INT08U i;
	CPU_INT08U tmpAcq;	//to avoid interblock case

	task_init();
#ifdef DEBUG_COM
	BSP_USART_printf(DEBUG_COM, "\r 4 : Entering Task4_LedTimer\n");
#endif
	i = 0;
	while (1) {
		OSSemPend(Sema_ACDacq, 0, &err);
		tmpAcq = ACDacq;
		OSSemPost(Sema_ACDacq);
		
		OSSemPend(Sema_Led, 0, &err);
		u8LedMap ^= LED_MONO0_GREEN | (LED_MONO1_GREEN & ((tmpAcq == 1) << 1)); //on suppose stop=0
		if ((i & 8) == 0)
			u8LedMap &= ~(LED_MONO2_GREEN); //on eteint les alarmes (UART et ADC)
		LED_Display(u8LedMap);
		OSSemPost(Sema_Led);
		//else error on pend
		OSTimeDly(led_delay); // we wait 200ms for next blink (OS_TICKS_PER_SEC/(1000/(200/2))) 
		++i;
	}
}

/**
*
*/
void Init_IO_Usager(void) {
	//BSP_USART_Init(DATA_COM, 115200);
	BSP_USART_Init(DATA_COM, 57600); //Initialise USART1 at 57600bauds, voir BSP.c
	BSP_USART_printf(DATA_COM, "\r\nPrototype 2 (DATA test)\n");
	
#ifdef DEBUG_COM	
	BSP_USART_Init(DEBUG_COM, 57600); //Initialise USART2 at 57600bauds, voir BSP.c
	BSP_USART_printf(DEBUG_COM, "\r\nPrototype 2 (NEW DEBUG test)\n");
#endif	
	

	//ADC pins
	static const gpio_map_t ADC_GPIO_MAP = {
		{ADC_LIGHT_PIN, ADC_LIGHT_FUNCTION},
		{ADC_POTENTIOMETER_PIN, ADC_POTENTIOMETER_FUNCTION}
	};
	//UART pins
	static const gpio_map_t USART0_GPIO_MAP =
	{
		{AVR32_USART0_RXD_0_0_PIN, AVR32_USART0_RXD_0_0_FUNCTION},
		{AVR32_USART0_TXD_0_0_PIN, AVR32_USART0_TXD_0_0_FUNCTION}
	};
	static const gpio_map_t USART1_GPIO_MAP =
	{
		{AVR32_USART1_RXD_0_0_PIN, AVR32_USART1_RXD_0_0_FUNCTION},
		{AVR32_USART1_TXD_0_0_PIN, AVR32_USART1_TXD_0_0_FUNCTION}
	};
	//ETH pins
	static const gpio_map_t MACB_GPIO_MAP =
	{
		{EXTPHY_MACB_MDC_PIN,     EXTPHY_MACB_MDC_FUNCTION   },
		{EXTPHY_MACB_MDIO_PIN,    EXTPHY_MACB_MDIO_FUNCTION  },
		{EXTPHY_MACB_RXD_0_PIN,   EXTPHY_MACB_RXD_0_FUNCTION },
		{EXTPHY_MACB_TXD_0_PIN,   EXTPHY_MACB_TXD_0_FUNCTION },
		{EXTPHY_MACB_RXD_1_PIN,   EXTPHY_MACB_RXD_1_FUNCTION },
		{EXTPHY_MACB_TXD_1_PIN,   EXTPHY_MACB_TXD_1_FUNCTION },
		{EXTPHY_MACB_TX_EN_PIN,   EXTPHY_MACB_TX_EN_FUNCTION },
		{EXTPHY_MACB_RX_ER_PIN,   EXTPHY_MACB_RX_ER_FUNCTION },
		{EXTPHY_MACB_RX_DV_PIN,   EXTPHY_MACB_RX_DV_FUNCTION },
		{EXTPHY_MACB_TX_CLK_PIN,  EXTPHY_MACB_TX_CLK_FUNCTION}
	};

	gpio_enable_module(ADC_GPIO_MAP, sizeof (ADC_GPIO_MAP) / sizeof (ADC_GPIO_MAP[0]));
	gpio_enable_module(USART0_GPIO_MAP,sizeof(USART0_GPIO_MAP) / sizeof(USART0_GPIO_MAP[0]));
	gpio_enable_module(USART1_GPIO_MAP,sizeof(USART1_GPIO_MAP) / sizeof(USART1_GPIO_MAP[0]));
	gpio_enable_module(MACB_GPIO_MAP, sizeof(MACB_GPIO_MAP) / sizeof(MACB_GPIO_MAP[0]));

#ifdef INTERUTP_MODE    
    AVR32_USART1.ier = AVR32_USART_IER_RXRDY_MASK;
    #ifdef DEBUG_COM
        AVR32_USART0.ier = AVR32_USART_IER_RXRDY_MASK;
    #endif
#endif	
	
	
	init_adc(&AVR32_ADC);

#ifdef INTERUTP_MODE
	BSP_INTC_FastIntReg(&IRQ_PB0, (AVR32_GPIO_IRQ_0 + AVR32_PIN_PX16 / 8), 0);
	BSP_INTC_FastIntReg(&IRQ_ADC_endconv, AVR32_ADC_IRQ, 0);
	BSP_INTC_FastIntReg(&IRQ_UART_DATA, AVR32_USART1_IRQ, 1);
    #ifdef DEBUG_COM
        BSP_INTC_FastIntReg(&IRQ_UART_DEBUG, AVR32_USART0_IRQ, 1);	
    #endif
//	BSP_GPIO_SetIntMode(GPIO_PB0_PIN,BSP_INT_RISING_EDGE);
	AVR32_GPIO.port[GPIO_PB0_PORT].gfers = 1 << (GPIO_PB0_PIN & 0x1F);
	AVR32_GPIO.port[GPIO_PB0_PORT].imr0c = 1 << (GPIO_PB0_PIN & 0x1F);
	AVR32_GPIO.port[GPIO_PB0_PORT].imr1s = 1 << (GPIO_PB0_PIN & 0x1F); //falling edge
	AVR32_GPIO.port[GPIO_PB0_PORT].iers = 1 << (GPIO_PB0_PIN & 0x1F); // Enable interrupt on GPIO88
#endif

#ifdef DEBUG_COM	
	BSP_USART_printf(DEBUG_COM, "\rInitialising MACB...\n");
#endif

	// initialize MACB & Phy Layers
	if (xMACBInit(&AVR32_MACB) == false )
	{
		#if ( (BOARD == EVK1100) || (BOARD==EVK1105) )
		gpio_clr_gpio_pin(LED0_GPIO);
		#endif
		while(1);
	}
	
#ifdef DEBUG_COM	
	BSP_USART_printf(DEBUG_COM, "\rInit Done\n");
#endif
}



/**
 * Initialisation de l'ADC
 * - Lower the ADC clock to match the ADC characteristics
 * - Set Sample/Hold time to max so that the ADC capacitor
 * - Set Startup to max so that the ADC capacitor
 * - Enable light and potentometer
 */
void init_adc(volatile avr32_adc_t *adc) {
	// Lower the ADC clock to match the ADC characteristics (because we configured
	// the CPU clock to 12MHz, and the ADC clock characteristics are usually lower;
	// cf. the ADC Characteristic section in the datasheet).
	adc->mr |= 0x1 << AVR32_ADC_MR_PRESCAL_OFFSET;
	
	/* Set Sample/Hold time to max so that the ADC capacitor should be
	 * loaded entirely */
	adc->mr |= 0xF << AVR32_ADC_SHTIM_OFFSET;
	/* Set Startup to max so that the ADC capacitor should be loaded
	 * entirely */
	adc->mr |= 0x1F << AVR32_ADC_STARTUP_OFFSET;
	
	/************************************************************************/
	/* http://www.avrfreaks.net/comment/796606#comment-796606			   */
	/* http://www.atmel.com/Images/doc2559.pdf							  */
	/************************************************************************/
	// If supported, trigger adc reading on TC1 channel interruption
	//adc->MR.trgsel = 2; 	// Trigger selection TC channel 2 = Timer Counter A1
	//adc->MR.trgen = AVR32_ADC_MR_TRGEN_MASK;
	adc->CHER.ch1 = 1;
	adc->CHER.ch2 = 1;
	
#ifdef INTERUTP_MODE
	adc->IER.eoc1 = 1;
	adc->IER.eoc2 = 1;
#endif
}


/************************************************************************/
/* ATMEL SOFTWARE FRAMEWORK(ASF) IMPLEMENTATIONS						*/
/************************************************************************/


/************************************************************************/
/* Fonctions importer													*/
/* move into other file ?                                               */
/************************************************************************/
void adc_start(volatile avr32_adc_t *adc) {
	/* start conversion */
	adc->cr = AVR32_ADC_START_MASK;
}

/// @brief Fonction ASF.
uint32_t gpio_enable_module_pin(uint32_t pin, uint32_t function) {
	volatile avr32_gpio_port_t *gpio_port = &AVR32_GPIO.port[pin >> 5];

	/* Enable the correct function. */
	switch (function) {
		case 0: /* A function. */
			gpio_port->pmr0c = 1 << (pin & 0x1F);
			gpio_port->pmr1c = 1 << (pin & 0x1F);
#if (AVR32_GPIO_H_VERSION >= 210)
			gpio_port->pmr2c = 1 << (pin & 0x1F);
#endif
			break;

		case 1: /* B function. */
			gpio_port->pmr0s = 1 << (pin & 0x1F);
			gpio_port->pmr1c = 1 << (pin & 0x1F);
#if (AVR32_GPIO_H_VERSION >= 210)
			gpio_port->pmr2c = 1 << (pin & 0x1F);
#endif
			break;

		case 2: /* C function. */
			gpio_port->pmr0c = 1 << (pin & 0x1F);
			gpio_port->pmr1s = 1 << (pin & 0x1F);
#if (AVR32_GPIO_H_VERSION >= 210)
			gpio_port->pmr2c = 1 << (pin & 0x1F);
#endif
			break;

		case 3: /* D function. */
			gpio_port->pmr0s = 1 << (pin & 0x1F);
			gpio_port->pmr1s = 1 << (pin & 0x1F);
#if (AVR32_GPIO_H_VERSION >= 210)
			gpio_port->pmr2c = 1 << (pin & 0x1F);
#endif
			break;

#if (AVR32_GPIO_H_VERSION >= 210)
		case 4: /* E function. */
			gpio_port->pmr0c = 1 << (pin & 0x1F);
			gpio_port->pmr1c = 1 << (pin & 0x1F);
			gpio_port->pmr2s = 1 << (pin & 0x1F);
			break;

		case 5: /* F function. */
			gpio_port->pmr0s = 1 << (pin & 0x1F);
			gpio_port->pmr1c = 1 << (pin & 0x1F);
			gpio_port->pmr2s = 1 << (pin & 0x1F);
			break;

		case 6: /* G function. */
			gpio_port->pmr0c = 1 << (pin & 0x1F);
			gpio_port->pmr1s = 1 << (pin & 0x1F);
			gpio_port->pmr2s = 1 << (pin & 0x1F);
			break;

		case 7: /* H function. */
			gpio_port->pmr0s = 1 << (pin & 0x1F);
			gpio_port->pmr1s = 1 << (pin & 0x1F);
			gpio_port->pmr2s = 1 << (pin & 0x1F);
			break;
#endif

		default:
			return GPIO_INVALID_ARGUMENT;
	}

	/* Disable GPIO control. */
	gpio_port->gperc = 1 << (pin & 0x1F);

	return GPIO_SUCCESS;
}

/// @brief Fonction ASF.
uint32_t gpio_enable_module(const gpio_map_t gpiomap, uint32_t size) {
	uint32_t status = GPIO_SUCCESS;
	uint32_t i;

	for (i = 0; i < size; i++) {
		status |= gpio_enable_module_pin(gpiomap->pin, gpiomap->function);
		gpiomap++;
	}

	return status;
}


//! Saved state of all LEDs.
static volatile CPU_INT32U LED_State = (1 << LED_COUNT) - 1;
static tLED_DESCRIPTOR LED_DESCRIPTOR[LED_COUNT] = {
#define INSERT_LED_DESCRIPTOR(LED_NO)                 \
	{                                                           \
		{LED##LED_NO##_GPIO / 32, 1 << (LED##LED_NO##_GPIO % 32)},\
		{LED##LED_NO##_PWM,	   LED##LED_NO##_PWM_FUNCTION	} \
	},
	INSERT_LED_DESCRIPTOR(0)
	INSERT_LED_DESCRIPTOR(1)
	INSERT_LED_DESCRIPTOR(2)
	INSERT_LED_DESCRIPTOR(3)
	INSERT_LED_DESCRIPTOR(4)
	INSERT_LED_DESCRIPTOR(5)
	INSERT_LED_DESCRIPTOR(6)
	INSERT_LED_DESCRIPTOR(7)
#undef INSERT_LED_DESCRIPTOR
};

void LED_Display(CPU_INT32U leds) {
    // Use the LED descriptors to get the connections of a given LED to the MCU.
    tLED_DESCRIPTOR *led_descriptor;
    volatile avr32_gpio_port_t *led_gpio_port;

	// Make sure only existing LEDs are specified.
	leds &= (1 << LED_COUNT) - 1;

	// Update the saved state of all LEDs with the requested changes.
	LED_State = leds;

    // For all LEDs...
    for (led_descriptor = &LED_DESCRIPTOR[0];
            led_descriptor < LED_DESCRIPTOR + LED_COUNT;
            led_descriptor++) {
        // Set the LED to the requested state.
        led_gpio_port = &AVR32_GPIO.port[led_descriptor->GPIO.PORT];
        if (leds & 1) {
            led_gpio_port->ovrc = led_descriptor->GPIO.PIN_MASK;
        } else {
            led_gpio_port->ovrs = led_descriptor->GPIO.PIN_MASK;
        }
        led_gpio_port->oders = led_descriptor->GPIO.PIN_MASK;
        led_gpio_port->gpers = led_descriptor->GPIO.PIN_MASK;
        leds >>= 1;
    }
}
