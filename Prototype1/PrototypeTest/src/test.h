
/*!
 * \file test.h
 *
 * \brief Header des fonctions de test disponibles
 * \author
 *	Max Moreau \n
 *	Charly Simon\n
 *	Benoit Rudelle 
 * \date 2015-03-08 18:34:25
 */ 
 

#ifndef TEST_H_
#define TEST_H_

#ifdef RUN_TEST
	/** Fonction de verification du comportement de notre fifo */
	void test_fifo(U32 buf, U8 nb_buf, U8 nb_buf2);
	/** Fonction d'affichage de la taille de nos different registre */
	void sizeof_regs(void);
	/** Test d'incrementation du mode */
	void test_switchmode(void);
#endif


#endif /* TEST_H_ */
