/*!
 * \file fifo.h
 *
 * \brief Abstraction des variables du programme
 * \author
 *	Max Moreau \n
 *	Charly Simon\n
 *	Benoit Rudelle 
 * \date 2015-03-08 18:55:23
 */


#ifndef FIFO_H_
#define FIFO_H_

/** Masque pour le buffer de light dans la fifo */
#define MSK_BUF_LIGHT	0x0000FFFF
/** Masque pour le buffer de pot dans la fifo */
#define MSK_BUF_POT		0xFFFF0000
/** Nombre de bit de decalage pour acceder au buffer pot dans la fifo */
#define OFFSET_POT		16


/************************************************************************
* Fonction de pile (FIFO)
* NB : Cette pile est sommaire et ne gere que 4 elements de 8bit max
* push = ajoute a la pile
*	Penser a check sur 8 bit avant insertion sinon curruption des autres donner
* pop = retire le premier element (plus vieux)
*	Ne supprime pas vraiment l'element de la pile, (le push le fera ulterieurement)
*	Vs devez check nb_buf afin de savoir si donner valid ou pas.
* http://fr.cppreference.com/w/cpp/language/operator_precedence
* Pile tres sensible, l'entre doit etre check sur 8bit avant de push
************************************************************************/

/**
* Macro de pile (FIFO)\n
* NB : Cette pile est sommaire et ne gere que 4 elements de 8bit max\n
* Push ajoute a la pile.
* - On pousse toute notre stack
* - On additionne que si l'on est pas au max
* - Cette pile est pour le buffer light
*/
#define push_light(fifo,val,nbuf) \
	fifo = (( ( ((fifo&MSK_BUF_LIGHT) << 8) | (val&0xFF) )&MSK_BUF_LIGHT) | (fifo&MSK_BUF_POT)); \
	nbuf +=((nbuf&0x2)!=0x2)
/**
* Macro de pile (FIFO)\n
* NB : Cette pile est sommaire et ne gere que 4 elements de 8bit max\n
* Pop retire le plus vieux element de la pile
*/
#define pop_light(fifo,nbuf) \
	((fifo>> ((nbuf-=(nbuf!=0)) <<3) )&0xFF)
/**
* Affichage du buffer light
*/
#define get_light(fifo) \
	(fifo&MSK_BUF_LIGHT)

/**
* Macro de pile (FIFO)\n
* NB : Cette pile est sommaire et ne gere que 4 elements de 8bit max\n
* Push2 ajoute a la pile.\n
* On pousse toute notre stack\n
* On additionne que si l'on est pas au max\n
*/
#define push_pot(fifo,val,nbuf) \
	fifo = (( ( ((fifo&MSK_BUF_POT) << 8) | ((val&0xFF)<< OFFSET_POT) )&MSK_BUF_POT) | (fifo&MSK_BUF_LIGHT))  ; \
	nbuf +=((nbuf&0x2)!=0x2)
/**
* Macro de pile (FIFO)\n
* NB : Cette pile est sommaire et ne gere que 4 elements de 8bit max\n
* Pop retire le plus vieux element de la pile\n
*/
#define pop_pot(fifo,nbuf) \
	(( (fifo&MSK_BUF_POT)>> (((nbuf-=(nbuf!=0))<<3) + OFFSET_POT) )&0xFF)
/**
* Affichage du buffer pot
*/
#define get_pot(fifo) \
	((fifo&MSK_BUF_POT)>>OFFSET_POT)

/**
* Structure du registre d'etat de notre programme
*/
struct reg_state {
	volatile int adc_changed : 1;				/*! True when Auto_Vitesse */
	volatile int speed_changed : 1;				/*! True when PB0 */
	volatile int speed_up : 1;					/*! *2 acquisition mode */
	volatile int pb1_pressed : 1;				/*! True when PB1 */
	int acq_prev : 1;							/*! True when received ('s' or 'x' from uart) */
	volatile int acq_start : 1;					/*! acquisition start or stop mode */
	int toogle : 1;								/*! switch UART send */
	volatile U8 u8LedMap;						/*! bit de chaque led */
	volatile U8 char_recu;						/*! char recu par uart	*/
	volatile uint nb_buf : 2;					/*! counter buffer light	*/
	volatile uint nb_buf2 : 2;					/*! counter buffer pot	*/
	volatile uint labo_mode : 2;				/*! Mode 0=lab normal, Mode 1=optimal acq, Mode 2=ethernet */
	// 1 bit libre
};

/**
* Structure du registre des valeur de conversion d'acd
* Recupere les valeur de conversion et verifie 
* si l'on est pas en depassement d'acd
*/
struct reg_acd_val {
	volatile uint acq_prise_pot : 3;			/*!	Nombre d'acquisition prise pour pot (avant nouveau acd_start) */
	volatile uint acq_prise_light : 3;			/*! Nombre d'acquisition prise pour light (avant nouveau acd_start) */
	volatile uint acq_started : 3;				/*! Nombre d'acd_start dans intr_temps (utiliser pour check depassement ADC) */
	volatile uint acq_finished : 3;				/*! Nombre d'acd_finit dans intr_temps (utiliser pour check depassement ADC) */
	//12
	volatile uint i : 4;						/*! Compteur de temporisation pour les clock */
	//16 libre
};

/**
* Structure du registre de counter de l'acd
* Permet de verifier si l'on transmet assez vite
*/
struct reg_acd_ok {
	volatile U16 acq_prise;						/*! Nombre d'acd_prise dans intr_temps (utiliser pour auto calc adc_fhz) */
	volatile U16 acq_tranmise;					/*! Nombre d'acd_transmise dans intr_temps (utiliser pour auto calc adc_fhz)  */
};

/**
* Structure du registre des frequence de l'acd
* Sert a augmenter ou diminuer la frequence d'acquisition
*/
struct reg_acd_freq {
	volatile U16 adc_fhz;						/*! Frequence du adc_start */
	U16 adc_prev;								/*! Frequence du adc_start precedente (pour eviter trop de changement) */
};
//struct reg5 {	volatile U16 buflight, bufpot; }	// fifo

#endif /* FIFO_H_ */
