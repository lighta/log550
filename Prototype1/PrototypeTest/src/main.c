/*!
*\file main.c                                                        
*                                                                             
* \brief Programme du prototype 1
* \author
*	Max Moreau \n
*	Charly Simon\n
*	Benoit Rudelle   
* \version 1.0 
* \date 7 mars 2015  \n           
* Composant FRAMEWORK a ajouter:  (GPIO et INTC deja inclu par defaut) \n     
*	Generic board support  (INTC)\n  
*	ADC - Analog to Digital Converter (driver)\n
*	GPIO - General-Purpose Input/Ouput (driver)\n
*	TC - Timer counter    (driver) \n
*	PM - Power Manager    (driver)\n
*	USART - Universal Synchronous Asynchronous Receiver Transmitter\n
*	USART Debug strings 
* Le UC3A possede 3 TIMER-COUNTER identique, identifie par channel 0-1-2.  \n   
* Ces TC sont de 16bits, donc il compte des cycles de 0x000 a RC choisi      \n 
* Lorsque le compte atteint RC, il genere une interrupt qui toggle un LED.  \n  
* Le TC compte a la vitesse FPBA/32 choisie (source TIMER_CLOCK4)           \n  
*********************************************************************************/

#include <asf.h>
#include <stdio.h>
#include "compiler.h"
#include "adc.h"
#include "gpio.h"
/*****************************************************************************************************************************/
/* http://elk.informatik.fh-augsburg.de/pub/rtlabor/ti-versuche/avr32/1.5.0-AT32UC3/DRIVERS/ADC/EXAMPLE/DOC/html/a00007.html */
/*****************************************************************************************************************************/
#include "board.h"
#include "config.h"
#include "fifo.h"
#include "test.h"


/**
*	Constantes globals  \n 
*	Configuration du peripherique TC0 
*  - .wavsel  = triguer automatique 
*  - .tcclks  = clock diviser par 8
*/
static const tc_waveform_opt_t WAVEFORM_OPT_TC0 =
{
	.channel  = TC_CHANNEL_0,                        // Channel selection.

	.bswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOB.
	.beevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOB.
	.bcpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOB.
	.bcpb     = TC_EVT_EFFECT_NOOP,                // RB compare effect on TIOB.

	.aswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOA.
	.aeevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOA.
	.acpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOA: toggle.
	.acpa     = TC_EVT_EFFECT_NOOP,                // RA compare effect on TIOA: toggle
	.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,// Waveform selection: Up mode with automatic trigger(reset) on RC compare.
	.enetrg   = FALSE,                             // External event trigger enable.
	.eevt     = 0,                                 // External event selection.
	.eevtedg  = TC_SEL_NO_EDGE,                    // External event edge selection.
	.cpcdis   = FALSE,                             // Counter disable when RC compare.
	.cpcstop  = FALSE,                             // Counter clock stopped with RC compare.

	.burst    = FALSE,                             // Burst signal selection.
	.clki     = FALSE,                             // Clock inversion.
	.tcclks   = TC_CLOCK_SOURCE_TC4                // Internal source clock 3, connected to fPBA / 8.
};
/** 
*	Constantes globals  \n 
*	Configuration du peripherique TC1 
*  - .wavsel  = triguer automatique
*  - .tcclks  = clock diviser par 8
*/
static const tc_waveform_opt_t WAVEFORM_OPT_TC1 =
{
	.channel  = TC_CHANNEL_1,						// Channel selection.

	.bswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOB.
	.beevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOB.
	.bcpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOB.
	.bcpb     = TC_EVT_EFFECT_NOOP,                // RB compare effect on TIOB.

	.aswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOA.
	.aeevt    = TC_EVT_EFFECT_NOOP,				   // External event effect on TIOA.
	.acpc     = TC_EVT_EFFECT_TOGGLE,              // RC compare effect on TIOA: toggle.
	.acpa     = TC_EVT_EFFECT_NOOP,                // RA compare effect on TIOA: toggle
	.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,// Waveform selection: Up mode with automatic trigger(reset) on RC compare.
	.enetrg   = FALSE,                             // External event trigger enable.
	.eevt     = 0,                                 // External event selection.
	.eevtedg  = TC_SEL_NO_EDGE,                    // External event edge selection.
	.cpcdis   = FALSE,                             // Counter disable when RC compare.
	.cpcstop  = FALSE,                             // Counter clock stopped with RC compare.

	.burst    = FALSE,                             // Burst signal selection.
	.clki     = FALSE,                             // Clock inversion.
	.tcclks   = TC_CLOCK_SOURCE_TC2                // Internal source clock 3, connected to fPBA / 8.
	// Internal source clock 4, connected to fPBA / 2.
};

/** 
*	Constantes globals  \n 
*	Configuration du choix d'interuption pour TC_INTERRUPT0 
* Activation d'interruption sur RC
*/
static const tc_interrupt_t TC_INTERRUPT0 =
{
	.etrgs = 0,
	.ldrbs = 0,
	.ldras = 0,
	.cpcs  = 1,
	.cpbs  = 0,
	.cpas  = 0,
	.lovrs = 0,
	.covfs = 0
};
/** 
*	Constantes globals  \n 
*	Configuration du choix d'interuption pour TC_INTERRUPT0 
*  Activation d'interruption sur RC
*/
static const tc_interrupt_t TC_INTERRUPT1 =
{
	.etrgs = 0,
	.ldrbs = 0,
	.ldras = 0,
	.cpcs  = 1,
	.cpbs  = 0,
	.cpas  = 0,
	.lovrs = 0,
	.covfs = 0
};


/** 
*	constantes globals  \n 
*	Mappage des pins pour les fonctions de USART 
*/
static const gpio_map_t USART_GPIO_MAP =
{
	{AVR32_USART1_RXD_0_0_PIN, AVR32_USART1_RXD_0_0_FUNCTION},
	{AVR32_USART1_TXD_0_0_PIN, AVR32_USART1_TXD_0_0_FUNCTION}
};
/** 
*	constantes globals  \n 
*	Mappage des pins pour les fonctions de l'ADC 
*/
static const gpio_map_t ADC_GPIO_MAP =
{
	{ADC_LIGHT_PIN, ADC_LIGHT_FUNCTION},
	{ADC_POTENTIOMETER_PIN, ADC_POTENTIOMETER_FUNCTION}
};	


/*****************************************************************************************************************************/
/* http://elk.informatik.fh-augsburg.de/pub/rtlabor/ti-versuche/avr32/1.5.0-AT32UC3/DRIVERS/ADC/EXAMPLE/DOC/html/a00007.html */
/*****************************************************************************************************************************/


//globals vars
volatile avr32_adc_t *adc = &AVR32_ADC;		/*! ADC registers address */
volatile avr32_tc_t *tc = TC;				/*! Timer Counter registers address */

struct reg_state reg_state;			/*! Registre des etats */
struct reg_acd_val reg_acd_val;		/*! Registre des valeurs de fin de conversion (ACD) */
struct reg_acd_ok reg_acd_ok; 		/*! Registre des verification UART/ACD */
struct reg_acd_freq reg_acd_freq; 	/*! Registre d'auto frequence ACD */

// http://stackoverflow.com/questions/10341706/how-can-i-bind-a-variable-to-an-unused-avr-i-o-register-using-avr-gcc
//register U32 pilefifo asm("r3");
volatile U32 pilefifo;				/*! Pile fifo pour nos valeur d'ACD */

/** Redirect vers push_light avec nos variables */
#define push_light_(val) push_light(pilefifo,val,reg_state.nb_buf)
/** Redirect vers pop_light avec nos variables */
#define pop_light_() pop_light(pilefifo,reg_state.nb_buf)
/** Redirect vers get_light avec nos variables */
#define get_light_() get_light(pilefifo)

/** Redirect vers push_pot avec nos variables */
#define push_pot_(val) push_pot(pilefifo,val,reg_state.nb_buf2)
/** Redirect vers pop_pot avec nos variables */
#define pop_pot_() pop_pot(pilefifo,reg_state.nb_buf2)
/** Redirect vers get_pot avec nos variables */
#define get_pot_() get_pot(pilefifo)


//early function declaration
void init_gpio(void);
void init_adc(void);

/**
* Routine d'interutpion du Timer counter TC0\n 
* Clignottement des LEDs (LED0 et LED1)
* Reset des LED_BI (errors) si active
*/
__attribute__((__interrupt__))
static void IRQ_TC0(void)
{
	tc_read_sr(TC, TC_CHANNEL_0); // La lecture du registre SR efface le fanion de l'interruption.
	reg_state.u8LedMap ^= LED_MONO0_GREEN|(LED_MONO1_GREEN&(reg_state.acq_start<<1))|(LED_MONO2_GREEN&((reg_state.labo_mode==2)<<2)); //on suppose stop=0
	
	//if((++i & 0x08)==0)
	//u8LedMap &= ~(((++i & 0x08)==0)*(LED_BI0_RED|LED_BI1_RED)); //raz des led rouge
	++reg_acd_val.i; 
	if((reg_acd_val.i & 0x3)==0){ //1s
		reg_state.u8LedMap &= ~(0xF0); //on eteint les bi et mono4
	}
}

/**
* Routine d'interutpion du Timer counter TC2  
* - Lit le registre de l'ADC 
* - Synchronisation des remises a zero pour cohesion des comparaisons
*/
__attribute__((__interrupt__))
static void IRQ_TC1(void)
{
	tc_read_sr(TC, TC_CHANNEL_1); // La lecture du registre SR efface le fanion de l'interruption.
	
	//Avant de lancer une nouvelle demande d,acquisition on verifie qu'on est synchro
	if(reg_acd_val.acq_prise_pot + reg_acd_val.acq_prise_light >= 2){
		if(abs((reg_acd_val.acq_started) - reg_acd_val.acq_finished) > 2 ){
			reg_state.u8LedMap |= LED_BI0_RED;	// LED5 rouge
			if(reg_state.labo_mode && reg_acd_freq.adc_fhz > ADC_DECR+ADC_SEUIL){ //on reduit la freq si l adc ne suit pas
					reg_acd_freq.adc_fhz -=ADC_DECR ;
					reg_state.adc_changed=TRUE;
			}
		}
		reg_acd_val.acq_started = 0;
		reg_acd_val.acq_finished = 0;
		 	
		if(abs(reg_acd_val.acq_prise_pot - reg_acd_val.acq_prise_light) > 2) //
			reg_state.u8LedMap |= LED_BI0_GREEN;	// LED5 vert
		 	
		reg_acd_val.acq_prise_pot = 0;
		reg_acd_val.acq_prise_light = 0;
	}
	
	adc->CR.start = 1;
	++reg_acd_val.acq_started;
	
	//synchro des remise a zero pour cohesion des comparaison
	//ici parce que TC1 et ADC au meme niveau INT_LVL0
	//adc_started et adc_finished doivent etre egalement au meme INT_LVL0 pour comparaison
	// on remet ses variable apres plusisuer demarage pour avoir une vue plus globale	 
	if((reg_acd_val.i & 0x3)==0){
		reg_acd_ok.acq_prise = 0;
		reg_acd_ok.acq_tranmise = 0;
	}
}

/**
* Routine d'interutpion du bouton PB0 \n 
* Active/Desactive le doublement de vitesse d'acquisition
*/
__attribute__((__interrupt__))
static void IRQ_PB0(void)
{
	//gpio_clear_pin_interrupt_flag(GPIO_BTN_0);
	// is the same function in doc32119.
	AVR32_GPIO.port[GPIO_PB0_PORT].ifrc = 1<<GPIO_PB0_PIN; //read
	reg_state.u8LedMap ^= LED_MONO2_GREEN; // (LED2)
	reg_state.speed_up ^= 1; //toggle speed (0:1)
	reg_state.speed_changed = TRUE;
}

/**
* Routine d'interutpion du bouton PB1 \n 
* Active la demande de changement de mode
*/
__attribute__((__interrupt__)) 
static void IRQ_PB1(void) 
{
	AVR32_GPIO.port[GPIO_PB1_PORT].ifrc = 1<<GPIO_PB1_PIN; //read
	reg_state.pb1_pressed = TRUE;
}

/**
* Routine d'interutpion du bouton PB2 \n
* Start ou Stop l'acquisition
*/
__attribute__((__interrupt__))
static void IRQ_JSUP(void)
{
	AVR32_GPIO.port[GPIO_JS_UP_PORT].ifrc = 1<<GPIO_JS_UP_PIN; //read
	reg_state.acq_start ^= 1;
}

/**
* Routine d'attente.
*/
__attribute__((__interrupt__))
static void IRQ_sleep(void)
{
	U32 i=0;
	while(++i);
}


/**
* Routine d'interuption de l'USART\n.
*  2 sources peuvent lancer cette interruption 
*   -   bit RXRDY : Ce bit se leve sur RECEPTION D'UN CARACTERE (provenant du PC),\n
*                  et redescend lorqu'on lit le caractere recu (acces lecture dans RHR)
*   -   bit TXRDY : Ce bit se leve lorsqu'un transmission (vers le PC se termine,\n
*                  et demeure lever tant que le transmetteur est disponible.  \n
* .csr : chk TX ou RX dispo\n
* .thr : buffer TX\n
* .ier : demande IRQ par TX\n
* .idr : terminer transmission\n
* AVR32_USART_CSR_TXRDY_MASK = 9 bit max\n
*/
__attribute__((__interrupt__))
static void IRQ_UART(void)
{
	// Si cette interruption est lancee par une reception (bit RXRDY=1)
	if (AVR32_USART1.csr & (AVR32_USART_CSR_RXRDY_MASK)) {
		//Lire le char recu dans registre RHR, et le stocker dans un 32bit
		reg_state.char_recu = (AVR32_USART1.rhr & AVR32_USART_RHR_RXCHR_MASK)&0x7B; 
		//Eliminer la source de l'IRQ, bit RXRDY (automatiquement mis a zero a la lecture de RHR)
		//(les seul char qui nous interesse sont 'x' et 's', ont peut donc masquer et eliminer les autres)
		switch (reg_state.char_recu) {
			case CH_START: reg_state.acq_start=START; break; //start : demarrer aquisation a partir de MAINTENANT tu commence a prendre les data
			case CH_STOP: reg_state.acq_start=STOP; break; //stop : arreter aquisition reinit du ADC
			default : break;
		}
	}
	else { // Donc cette l'interruption est lancee par une fin de transmission, bit TXRDY=1
		if(reg_state.nb_buf || reg_state.nb_buf2){
			reg_state.toogle^=1;
			if(AVR32_USART1.CSR.txrdy == 0) { //com non fini				
				reg_state.u8LedMap |= LED_BI1_RED; // LED 6 rouge : UART non pret
			}
			else if(reg_state.nb_buf && reg_state.toogle==0) // if(AVR32_USART1.CSR.txrdy) //pret a ecrire dedans 
			{
				//pop();
				AVR32_USART1.thr = pop_light_() & AVR32_USART_THR_TXCHR_MASK; // on renvoi le char //mask useless ?
				++reg_acd_ok.acq_tranmise;
			}
			else if(reg_state.nb_buf2 && reg_state.toogle){
				//pop2();
				AVR32_USART1.thr = pop_pot_() & AVR32_USART_THR_TXCHR_MASK; // on renvoi le char //mask useless ?
				++reg_acd_ok.acq_tranmise;
			}		
		}
		else {   //On a plus rien a transmettre fin des interuption !
			AVR32_USART1.idr = AVR32_USART_IDR_TXRDY_MASK; //fin de tx
		}
	}
}

/**
* Routine d'interuption de fin de conversion de l'ADC\n
* Routine pour mode normal
*/
__attribute__((__interrupt__))
static void IRQ_ADC_endconv(void)
{
	if(reg_state.nb_buf>UART_WMAX_NORM || reg_state.nb_buf2>UART_WMAX_NORM){ //on a pas depop assez vite
		reg_state.u8LedMap |= LED_BI1_RED;
	}
	
	//adc_interrup;
	if(adc->SR.eoc2) // Light channel 2
	{
		/*light interrupt*/
		push_pot_( (((adc->cdr2 >> 2) & 0xFF) | 0x01) );
		AVR32_USART1.IER.txrdy= 1;	
		++reg_acd_val.acq_prise_light;
	}
	
	if(adc->SR.eoc1) // potentiometer channel 1
	{
		/*potentiometer interrupt*/
		push_light_( ((adc->cdr1 >>2) & 0xFE) );
		AVR32_USART1.IER.txrdy= 1;
		++reg_acd_ok.acq_prise;
	}
	++reg_acd_val.acq_finished;
}

/**
* IRQ pour ADC en fin d'une conversion\n 
* Similaire a IRQ_ADC_endconv mais on change la frequence de l'ADC pour optimiser l'acquisition\n 
* Limiter par l'envoie (UART) ou du nombres d'acquisition, on recalcule automatiquement ca frequence
* Strategie Employe :
	- On Baisse la vitesse si :
		- On a un element de chaque dans notre buffer
		- La baisse ne fera pas un underflow de l'adc_freq (loop par le bas)
	- On augmente si :
		- On peut (haha)
		- Baudrate est inferieur a: baudrate maximum diviser par 20=(10 bit transmis * 2 (nombre de conversion))
		- si on depose pas asser vite
	- Si un changement est fait on marque l'adc_change (notification), pour que le main change le RC
	- NB : Toutes les frequences ne correspondes pas a un RC exacte, mais on ne tiendra pas compte de l'approximation
* 
*/
__attribute__((__interrupt__))
static void IRQ_ADC_endconv2(void)
{	
	//verifie si depassement d UART, et calc new freq
//	if(reg_state.adc_changed==FALSE){
		U16 diff = abs(reg_acd_ok.acq_prise - reg_acd_ok.acq_tranmise);
		if(reg_state.nb_buf>UART_WMAX_OPTI || reg_state.nb_buf2>UART_WMAX_OPTI) {
			if(reg_acd_freq.adc_fhz > (diff+ADC_DECR+ADC_SEUIL) ){ //on a pas depop assez vite
				reg_state.u8LedMap |= LED_BI1_RED;
				reg_acd_freq.adc_fhz -= diff+ADC_DECR ;
				reg_state.adc_changed=TRUE;
			}
		}
		else if(BAUDRATE_MAX/20 > (reg_acd_freq.adc_fhz+diff+ADC_INCR+ADC_SEUIL) ) {
			reg_acd_freq.adc_fhz +=diff+ADC_INCR;
			reg_state.u8LedMap |= LED_BI1_GREEN;
			reg_state.adc_changed=TRUE;
		}
//	}
		
	//adc_interrup;
	if( adc->sr & (1 << ADC_LIGHT_CHANNEL)) // Light channel 2
	{
		/*light interrupt*/
		push_pot_( (((adc->cdr2 >> 2) & 0xFF) | 0x01) );
		AVR32_USART1.IER.txrdy= 1;
		++reg_acd_val.acq_prise_light;
	}
	
	if( adc->sr & (1 << ADC_POTENTIOMETER_CHANNEL)) // potentiometer channel 1 //adc->SR.eoc1
	{
		/*potentiometer interrupt*/
		push_light_(( adc->cdr1 >>2) & 0xFE);
		AVR32_USART1.IER.txrdy= 1;
		++reg_acd_val.acq_prise_pot;
		
	}
 	++reg_acd_val.acq_finished;
 	++reg_acd_ok.acq_prise;	
}

/**
 * Routine d'interruption de fin d'ACD pour l'envoi par ethernet
 */ 
__attribute__((__interrupt__))
static void IRQ_ADC_endconv3(void)
{
	//verifie si depassement d UART, et calc new freq
	//	if(reg_state.adc_changed==FALSE){
	U16 diff = abs(reg_acd_ok.acq_prise - reg_acd_ok.acq_tranmise);
	if(reg_state.nb_buf>UART_WMAX_OPTI || reg_state.nb_buf2>UART_WMAX_OPTI) {
		if(reg_acd_freq.adc_fhz > (diff+ADC_DECR+ADC_SEUIL) ){ //on a pas depop assez vite
			reg_state.u8LedMap |= LED_BI1_RED;
			reg_acd_freq.adc_fhz -= diff+ADC_DECR ;
			reg_state.adc_changed=TRUE;
		}
	}
	else if( ((ETH_MAX << 3)/20) > (reg_acd_freq.adc_fhz+diff+ADC_INCR+ADC_SEUIL) ) { //ETH_MAX est en octect
		reg_acd_freq.adc_fhz +=diff+ADC_INCR;
		reg_state.u8LedMap |= LED_BI1_GREEN;
		reg_state.adc_changed=TRUE;
	}
	//	}
	
	//adc_interrup;
	if( adc->sr & (1 << ADC_LIGHT_CHANNEL)) // Light channel 2
	{
		/*light interrupt*/
		push_pot_( (((adc->cdr2 >> 2) & 0xFF) | 0x01) );
		// ETH.rdy !
		++reg_acd_val.acq_prise_light;
	}
	
	if( adc->sr & (1 << ADC_POTENTIOMETER_CHANNEL)) // potentiometer channel 1 //adc->SR.eoc1
	{
		/*potentiometer interrupt*/
		push_light_(( adc->cdr1 >>2) & 0xFE);
		// ETH.rdy !
		++reg_acd_val.acq_prise_pot;
		
	}
	++reg_acd_val.acq_finished;
	++reg_acd_ok.acq_prise;
}

/************************************************************************
 *                         configure ADC                                *
/************************************************************************/
/**
* Initialisation de l'ADC
* - Lower the ADC clock to match the ADC characteristics
* - Set Sample/Hold time to max so that the ADC capacitor
* - Set Startup to max so that the ADC capacitor
* - Enable light and potentometer
*/
void init_adc(void)
{
	 reg_acd_freq.adc_fhz = reg_acd_freq.adc_prev = 2000; 
	 reg_acd_val.acq_prise_pot = reg_acd_val.acq_prise_light = 0;
	 reg_acd_val.acq_started = reg_acd_val.acq_finished = 0;
	 reg_acd_ok.acq_prise = reg_acd_ok.acq_tranmise = 0;
	
	// Lower the ADC clock to match the ADC characteristics (because we configured
	// the CPU clock to 12MHz, and the ADC clock characteristics are usually lower;
	// cf. the ADC Characteristic section in the datasheet).
	AVR32_ADC.mr |= 0x1 << AVR32_ADC_MR_PRESCAL_OFFSET;
	
	/* Set Sample/Hold time to max so that the ADC capacitor should be
	 * loaded entirely */
	adc->mr |= 0xF << AVR32_ADC_SHTIM_OFFSET;
	/* Set Startup to max so that the ADC capacitor should be loaded
	 * entirely */
	adc->mr |= 0x1F << AVR32_ADC_STARTUP_OFFSET;
	
	/************************************************************************/
	/* http://www.avrfreaks.net/comment/796606#comment-796606               */
	/* http://www.atmel.com/Images/doc2559.pdf                              */
	/************************************************************************/
	// If supported, trigger adc reading on TC1 channel interruption
	//adc->MR.trgsel = 2; 	// Trigger selection TC channel 2 = Timer Counter A1
	//adc->MR.trgen = AVR32_ADC_MR_TRGEN_MASK;
	adc->CHER.ch1 = 1;
	adc->CHER.ch2 = 1;
	//adc->cher = 0xFF; //enable all channel for test
	//enabled EOC interrupt on channel
// 	adc->ier |= 0x01<<ADC_LIGHT_CHANNEL;
// 	adc->ier |= 0x01<<ADC_POTENTIOMETER_CHANNEL;
	
	adc->IER.eoc1 = 1;
	adc->IER.eoc2 = 1;
	//adc->IER.ovre0 = 1;
 	//adc->IER.ovre1 = 1;
 	//adc->IER.govre = 1;
}

/** 
 * Initialisation du GPIO
 * - Activation des boutons PB0 et PB1
 * - active les pin utiliser par USART
 */
void init_gpio(void)
{
	//registre des leds
	//// Init GPIO51 (PB19)	
	//AVR32_GPIO.port[1].oders = 1 << (19 & 0x1F); // The GPIO output driver is enabled for that pin.
	//AVR32_GPIO.port[1].gpers = 1 << (19 & 0x1F); // The GPIO module controls that pin.
	//// Init GPIO54 (PB22)	
	//AVR32_GPIO.port[1].oders = 1 << (22 & 0x1F); // The GPIO output driver is enabled for that pin.
	//AVR32_GPIO.port[1].gpers = 1 << (22 & 0x1F); // The GPIO module controls that pin.

	/***********************************************************************
	* Toggle sur pression PB0		
	* doc32058.pdf p.186
	* gpio.c : gpio_enable_pin_glitch_filter(uint32_t pin)
	* GPIO88=PB0 bit control can be found in gpio port 2 (88/32=>2), bit 24 (88%32=24). 
	* GPIO85=PB1 bit control can be found in gpio port 2 (85/32=>2), bit 21 (85%32=21).
	***********************************************************************/
	// Enable the glitch filter
	AVR32_GPIO.port[GPIO_PB0_PORT].gfers = 1 << (GPIO_PB0_PIN & 0x1F);
	AVR32_GPIO.port[GPIO_PB1_PORT].gfers = 1 << (GPIO_PB1_PIN & 0x1F);
//	AVR32_GPIO.port[GPIO_PB2_PORT].gfers = 1 << (GPIO_PB2_PIN & 0x1F);
 	AVR32_GPIO.port[GPIO_JS_UP_PORT].gfers = 1 << (GPIO_JS_UP_PIN & 0x1F);
// 	AVR32_GPIO.port[GPIO_JS_DOWN_PORT].gfers = 1 << (GPIO_JS_DOWN_PIN & 0x1F);

	/***********************************************************************
	* Configure the edge detector on pin change on GPIO88. We use imr0c (c like 'clear') to clear the specified bit.
	* gpio.c : uint32_t gpio_configure_edge_detector(uint32_t pin, uint32_t mode)
	* imr0c =
	* imr0s = raising edge, (1=desactive/0=actvive la pression)
	* imr1c =
	* imr1s = falling edge, (1=desactive/0=active la relache)                                                                   
	***********************************************************************/
	//init fait par defaut
	//AVR32_GPIO.port[GPIO88_PX16_PORT].imr1c = 0;
	//AVR32_GPIO.port[GPIO88_PX16_PORT].imr0s = 0;
	//AVR32_GPIO.port[GPIO88_PX16_PORT].imr1s = 0;

	AVR32_GPIO.port[GPIO_PB0_PORT].imr0c = 1 << (GPIO_PB0_PIN & 0x1F);
	AVR32_GPIO.port[GPIO_PB1_PORT].imr0c = 1 << (GPIO_PB1_PIN & 0x1F);
//	AVR32_GPIO.port[GPIO_PB2_PORT].imr0c = 1 << (GPIO_PB2_PIN & 0x1F);
 	AVR32_GPIO.port[GPIO_JS_UP_PORT].imr0c = 1 << (GPIO_JS_UP_PIN & 0x1F);
// 	AVR32_GPIO.port[GPIO_JS_DOWN_PORT].imr0c = 1 << (GPIO_JS_DOWN_PIN & 0x1F);

	//desactive la relache
	AVR32_GPIO.port[GPIO_PB0_PORT].imr1s = 1 << (GPIO_PB0_PIN & 0x1F); //falling edge
	AVR32_GPIO.port[GPIO_PB1_PORT].imr1s = 1 << (GPIO_PB1_PIN & 0x1F);
//	AVR32_GPIO.port[GPIO_PB2_PORT].imr1s = 1 << (GPIO_PB2_PIN & 0x1F);
 	AVR32_GPIO.port[GPIO_JS_UP_PORT].imr1s = 1 << (GPIO_JS_UP_PIN & 0x1F);
// 	AVR32_GPIO.port[GPIO_JS_DOWN_PORT].imr1s = 1 << (GPIO_JS_DOWN_PIN & 0x1F)
	
	//enable interupt
	AVR32_GPIO.port[GPIO_PB0_PORT].iers = 1 << (GPIO_PB0_PIN & 0x1F); // Enable interrupt on GPIO88
	AVR32_GPIO.port[GPIO_PB1_PORT].iers = 1 << (GPIO_PB1_PIN & 0x1F);
//	AVR32_GPIO.port[GPIO_PB2_PORT].iers = 1 << (GPIO_PB2_PIN & 0x1F);
	AVR32_GPIO.port[GPIO_JS_UP_PORT].iers = 1 << (GPIO_JS_UP_PIN & 0x1F);
	//AVR32_GPIO.port[GPIO_JS_DOWN_PORT].iers = 1 << (GPIO_JS_DOWN_PIN & 0x1F);

	// Assigner les pins du GPIO a etre utiliser par le USART1.
	gpio_enable_module(USART_GPIO_MAP,sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));
	// Assigner les pins du GPIO a etre utiliser par le ADC
	gpio_enable_module(ADC_GPIO_MAP, sizeof(ADC_GPIO_MAP) / sizeof(ADC_GPIO_MAP[0]));
}

int main(void)
{
	/************************************************************************/
	/* Configuration                                                        */
	/************************************************************************/
	/*! \brief Main function:
	*  - Configure the CPU to run at 12MHz
	*  - Configure GPIO for led, USART, ADC
	*  - Configure ADC
	*  - Configure RS baudraute  
	*  - Register the TC interrupt (GCC only)
	*  - Configure Timer TC0=led, TC1=adc start
	*  - while(1) { upd_acd_timer; start_stop_acd; showled; chkmode;  }
	*/

	/* Au reset, le microcontroleur opere sur un crystal interne a 115200Hz. */
	/* Nous allons le configurer pour utiliser un crystal externe, F_BOARD, a 12Mhz. */
	pcl_switch_to_osc(PCL_OSC0, F_BOARD, OSC0_STARTUP);
	
	// initialisation des variables globales;
	reg_state.adc_changed = FALSE; //autorise le premier recalc
	reg_state.speed_changed = 0;
	reg_state.speed_up = 0;
	reg_state.pb1_pressed = 0;
	reg_state.acq_prev = reg_state.acq_start = 0;
	reg_state.labo_mode = 0;
	reg_state.u8LedMap = 0;
	reg_state.char_recu = 0;
	reg_state.nb_buf = reg_state.nb_buf2 = 0;
	// Init all I/Os
	init_gpio();
	init_adc();

	// Initialise le USART1 en mode seriel RS232, a F_BOARD=12MHz.
	init_dbg_rs232(F_BOARD); //def baudrate = 57600
	
	/************************************************************************/
	/*                     Configure interrupt                              */
	/************************************************************************/
	// Desactive les interrupts le temps de la config
	Disable_global_interrupt(); 
		INTC_init_interrupts();     // Initialise les vecteurs d'interrupt
		// Enregistrement de la nouvelle IRQ du TIMER0 au Interrupt Controller .
		INTC_register_interrupt(&IRQ_TC0, TC_IRQ_0, AVR32_INTC_INT3);	//leds
		INTC_register_interrupt(&IRQ_TC1, TC_IRQ_1, AVR32_INTC_INT0);	//adc
		// IRQ du TIMER1 est mapper a ADC directement
		INTC_register_interrupt(&IRQ_PB0, (AVR32_GPIO_IRQ_0+AVR32_PIN_PX16/8), AVR32_INTC_INT2); // 88/8
		INTC_register_interrupt(&IRQ_PB1, (AVR32_GPIO_IRQ_0+AVR32_PIN_PX19/8), AVR32_INTC_INT2);
//		INTC_register_interrupt(&IRQ_PB2, (AVR32_GPIO_IRQ_0+AVR32_PIN_PX22/8), AVR32_INTC_INT2); //conflict with pb1
 		INTC_register_interrupt(&IRQ_JSUP, (AVR32_GPIO_IRQ_0+AVR32_PIN_PA26/8), AVR32_INTC_INT2);
// 		INTC_register_interrupt(&IRQ_JSDW, (AVR32_GPIO_IRQ_0+AVR32_PIN_PA27/8), AVR32_INTC_INT2);
		//// Enregister le USART interrupt handler au INTC, level INT0
		INTC_register_interrupt(&IRQ_UART, AVR32_USART1_IRQ, AVR32_INTC_INT1);
		//blabla adc
		INTC_register_interrupt(&IRQ_ADC_endconv, AVR32_ADC_IRQ, AVR32_INTC_INT0);
	Enable_global_interrupt();
	// Active les interrupts
	
	//init timer0
	tc_init_waveform(tc, &WAVEFORM_OPT_TC0);     // Initialize the timer/counter waveform.
	//init timer 0 (led)
	tc_write_rc(tc, TC_CHANNEL_0, FCHZ_C0 / (FREQ_LED << 1));
	tc_configure_interrupts(tc, TC_CHANNEL_0, &TC_INTERRUPT0);
	//init timer1 (acd start)
	tc_init_waveform(tc, &WAVEFORM_OPT_TC1);
	tc_write_rc(tc, TC_CHANNEL_1, FCHZ_C1 / reg_acd_freq.adc_fhz);
	/*tc_write_ra(tc, TC_CHANNEL_1, (FPBA / 128) / (frequence_acq << 1));*/
	tc_configure_interrupts(tc, TC_CHANNEL_1, &TC_INTERRUPT1);
	
	// Start the timer/counter.
	tc_start(tc, TC_CHANNEL_0);           // And start the timer/counter.

#ifdef RUN_TEST
	sizeof_regs();
	test_fifo(pilefifo, reg_state.nb_buf, reg_state.nb_buf2);
	test_switchmode();
//	test_led();
#endif

	print_dbg("\n\rTaper un caractere sur le clavier du PC avec TeraTerminal...\n\n");
	// Activer la source d'interrution du UART en reception (RXRDY)
	AVR32_USART1.ier = AVR32_USART_IER_RXRDY_MASK;
	/************************************************************************/
	/* Boucle infinie de traitement                                         */
	/************************************************************************/
	while(1)
	{	//mettre les appel par ordre d'importance par le haut, (dans le cas d'interrupt)
		
		//change acquisition timer
		if(reg_state.adc_changed+reg_state.speed_changed){ //avoid long reset
			#define CHANGE_TC1() \
				reg_acd_freq.adc_prev = reg_acd_freq.adc_fhz; \
				tc_write_rc(tc, TC_CHANNEL_1, FCHZ_C1 / reg_acd_freq.adc_fhz )
			if(reg_state.adc_changed){
				if(abs(reg_acd_freq.adc_fhz-reg_acd_freq.adc_prev) > ADC_THRESHOLD ){
					CHANGE_TC1(); // Set RC value.
				}
				reg_state.adc_changed=FALSE;
			} else { //donc speed_changed
				reg_acd_freq.adc_fhz = (reg_state.speed_up!=FALSE)? (reg_acd_freq.adc_fhz * 2):(reg_acd_freq.adc_fhz / 2);
				CHANGE_TC1(); // Set RC value.
				reg_state.speed_changed=FALSE;
			}
			#undef CHANGE_TC1 	
		}
		
		if(reg_state.acq_start==STOP){ //stop : arreter aquisition reinit du ADC
			if(reg_state.acq_prev != reg_state.acq_start){ //avoid reset
				reg_state.acq_prev=reg_state.acq_start;
				reg_state.u8LedMap &= ~LED_MONO1_GREEN;
				tc_stop(tc, TC_CHANNEL_1);
			}
		} else { //start : demarrer aquisation a partir de MAINTENANT tu commence a prendre les data
			if(reg_state.acq_prev != reg_state.acq_start){
				reg_state.acq_prev=reg_state.acq_start;
				reg_state.u8LedMap |= (reg_state.u8LedMap << 1)&0x3; //on copie la valeur de led0 au start pour synchro
				tc_start(tc, TC_CHANNEL_1);            // And start the timer/counter.
			} //adc conversion is done
		}
		
		if(reg_state.pb1_pressed!=FALSE){		
			#define RAZ() \
				reg_state.acq_prev = reg_state.acq_start = STOP; \
				reg_state.adc_changed = FALSE; \
				reg_state.speed_changed = FALSE; \
				reg_state.speed_up = FALSE; \
				reg_acd_freq.adc_prev = reg_acd_freq.adc_fhz = 2000; \
				reg_state.u8LedMap = 0; \
				tc_write_rc(tc, TC_CHANNEL_1, FCHZ_C1 / reg_acd_freq.adc_fhz )
			
			++reg_state.labo_mode; // [0-4], 0=normal, 1=opti serial, 2=ethernet, 3=idle
			tc_stop(tc, TC_CHANNEL_1);
			Disable_global_interrupt();
			RAZ(); //raz lab
			switch(reg_state.labo_mode){
				case 0:
					init_dbg_rs232(F_BOARD); //def baudrate = 57600
					INTC_register_interrupt(&IRQ_ADC_endconv, AVR32_ADC_IRQ, AVR32_INTC_INT0);
				break;
				case 1:
					init_dbg_rs232_ex(BAUDRATE_MAX, F_BOARD); //baudrate max = 115200
					INTC_register_interrupt(&IRQ_ADC_endconv2, AVR32_ADC_IRQ, AVR32_INTC_INT0);
					reg_state.u8LedMap |= LED_MONO2_GREEN; //allume led mono 2
				break;
				case 2:
					// init_mac()
					INTC_register_interrupt(&IRQ_ADC_endconv3, AVR32_ADC_IRQ, AVR32_INTC_INT0);
					//allume led mono 2 blink
				break;
				case 3:
					INTC_register_interrupt(&IRQ_sleep, AVR32_ADC_IRQ, AVR32_INTC_INT3);
					reg_state.u8LedMap = 0xFF; //allume toute les leds
				break;
			}
			Enable_global_interrupt();
			AVR32_USART1.ier = AVR32_USART_IER_RXRDY_MASK;
			reg_state.pb1_pressed = FALSE;
			#undef RAZ
		}
		
		LED_Display(reg_state.u8LedMap); //affichage des leds
	}
	return 0;
}
