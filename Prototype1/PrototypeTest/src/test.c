/*!
 * \file test.c
 *
 * \brief Fonction de tests, verifications des differents elements
 * \author
 *	Max Moreau \n
 *	Charly Simon\n
 *	Benoit Rudelle 
 * \date 2015-03-08 18:34:02
 */ 

#include <asf.h>
#include <stdio.h>
#include "compiler.h"
#include "board.h"
#include "config.h"
#include "test.h"
#include "fifo.h"

#define push_light_(val) push_light(buf,val,nb_buf)
#define pop_light_() pop_light(buf,nb_buf)
#define get_light_() get_light(buf)

#define push_pot_(val) push_pot(buf,val,nb_buf2)
#define pop_pot_() pop_pot(buf,nb_buf2)
#define get_pot_() get_pot(buf)

/** Definition du nombre de charactere max dans notre string a transmettre */
#define MAXLINE	100

#ifdef RUN_TEST 

/**
 * Fonction de verification du comportement de notre fifo
 */  
void test_fifo(U32 buf, U8 nb_buf, U8 nb_buf2)
{
	char string[MAXLINE];
	U8 valtmp=0;
	buf=0;
	nb_buf=0;
	
	string[MAXLINE-1]='\0';
	buf=0;
	print_dbg("\n\r Test Fifo\n");
	
	print_dbg("\r Push light\n");
	print_dbg("\r push 1\n");
	push_light_(1);
	sprintf(string,"\r buffer=%x bufpot=%hu buflight=%hu nb_buf=%d\n",buf,get_pot_(),get_light_(),nb_buf);
	print_dbg(string);

	print_dbg("\r push 3\n");
	push_light_(3);
	sprintf(string,"\r buffer=%x bufpot=%hu buflight=%hu nb_buf=%d\n",buf,get_pot_(),get_light_(),nb_buf);
	print_dbg(string);

	print_dbg("\r push 4\n");
	push_light_(4);
	sprintf(string,"\r buffer=%x bufpot=%hu buflight=%hu nb_buf=%d\n",buf,get_pot_(),get_light_(),nb_buf);
	print_dbg(string);

	print_dbg("\r push 156\n");
	push_light_(156);
	sprintf(string,"\r buffer=%x bufpot=%hu buflight=%hu nb_buf=%d\n",buf,get_pot_(),get_light_(),nb_buf);
	print_dbg(string);
	


	print_dbg("\r Push pot\n");
	print_dbg("\r push 5\n");
	push_pot_(5);
	sprintf(string,"\r buffer=%x bufpot=%hu buflight=%hu nb_buf2=%d\n",buf,get_pot_(),get_light_(),nb_buf2);
	print_dbg(string);

	print_dbg("\r push 9\n");
	push_pot_(9);
	sprintf(string,"\r buffer=%x bufpot=%hu buflight=%hu nb_buf2=%d\n",buf,get_pot_(),get_light_(),nb_buf2);
	print_dbg(string);

	print_dbg("\r push 30\n");
	push_pot_(30);
	sprintf(string,"\r buffer=%x bufpot=%hu buflight=%hu nb_buf2=%d\n",buf,get_pot_(),get_light_(),nb_buf2);
	print_dbg(string);

	print_dbg("\r push 75\n");
	push_pot_(75);
	sprintf(string,"\r buffer=%x bufpot=%hu buflight=%hu nb_buf2=%d\n",buf,get_pot_(),get_light_(),nb_buf2);
	print_dbg(string);

	print_dbg("\r push light 8\n");
	push_light_(8);
	sprintf(string,"\r buffer=%x bufpot=%hu buflight=%hu nb_buf=%d\n",buf,get_pot_(),get_light_(),nb_buf);
	print_dbg(string);
	
	print_dbg("\n\r pop light\n");
	valtmp=pop_light_();
	sprintf(string,"\r val=%d buffer=%x bufpot=%hu buflight=%hu nb_buf=%d\n",valtmp,buf,get_pot_(),get_light_(),nb_buf);
	print_dbg(string);
	valtmp=pop_light_();
	sprintf(string,"\r val=%d buffer=%x bufpot=%hu buflight=%hu nb_buf=%d\n",valtmp,buf,get_pot_(),get_light_(),nb_buf);
	print_dbg(string);
	valtmp=pop_light_();
	sprintf(string,"\r val=%d buffer=%x bufpot=%hu buflight=%hu nb_buf=%d\n",valtmp,buf,get_pot_(),get_light_(),nb_buf);
	print_dbg(string);
	valtmp=pop_light_();
	sprintf(string,"\r val=%d buffer=%x bufpot=%hu buflight=%hu nb_buf=%d\n",valtmp,buf,get_pot_(),get_light_(),nb_buf);
	print_dbg(string);
	
	print_dbg("\n\r pop pot\n");
	valtmp=pop_pot_();
	sprintf(string,"\r val=%d buffer=%x bufpot=%hu buflight=%hu nb_buf2=%d\n",valtmp,buf,get_pot_(),get_light_(),nb_buf2);
	print_dbg(string);
	valtmp=pop_pot_();
	sprintf(string,"\r val=%d buffer=%x bufpot=%hu buflight=%hu nb_buf2=%d\n",valtmp,buf,get_pot_(),get_light_(),nb_buf2);
	print_dbg(string);
	valtmp=pop_pot_();
	sprintf(string,"\r val=%d buffer=%x bufpot=%hu buflight=%hu nb_buf2=%d\n",valtmp,buf,get_pot_(),get_light_(),nb_buf2);
	print_dbg(string);
	valtmp=pop_pot_();
	sprintf(string,"\r val=%d buffer=%x bufpot=%hu buflight=%hu nb_buf2=%d\n",valtmp,buf,get_pot_(),get_light_(),nb_buf2);
	print_dbg(string);

	print_dbg("\r push light 15\n");
	push_light_(15);
	sprintf(string,"\r buffer=%x bufpot=%hu buflight=%hu nb_buf=%d\n",buf,get_pot_(),get_light_(),nb_buf);
	print_dbg(string);

	print_dbg("\r push pot 8\n");
	push_pot_(8);
	sprintf(string,"\r buffer=%x bufpot=%hu buflight=%hu nb_buf=%d\n",buf,get_pot_(),get_light_(),nb_buf);
	print_dbg(string);
	
	buf=0;
	nb_buf=0;
	print_dbg("\n\r push WTF suivit de pop\n");
	print_dbg("\r push 12\n");
	push_light_(12);
	sprintf(string,"\r buffer=%x bufpot=%hu buflight=%hu nb_buf=%d\n",buf,get_pot_(),get_light_(),nb_buf);
	print_dbg(string);

	print_dbg("\r push 65535 0xFFFF \n");
	push_light_(65535);
	sprintf(string,"\r buffer=%x bufpot=%hu buflight=%hu nb_buf=%d\n",buf,get_pot_(),get_light_(),nb_buf);
	print_dbg(string);

	print_dbg("\r pop\n");
	valtmp=pop_light_();
	sprintf(string,"\r val=%d  buffer=%x val=%d nb_buf=%d\n",valtmp,buf,valtmp,nb_buf);
	print_dbg(string);
		
	buf=0;
	nb_buf=0;
	print_dbg("\n\r push suivit de 2 pop WTF\n");
	print_dbg("\r push 2\n");
	push_light_(2);
	sprintf(string,"\r buffer=%x bufpot=%hu buflight=%hu nb_buf=%d\n",buf,get_pot_(),get_light_(),nb_buf);
	print_dbg(string);

	print_dbg("\r pop\n");
	valtmp=pop_light_();
	sprintf(string,"\r val=%d buffer=%x bufpot=%hu buflight=%hu nb_buf=%d\n",valtmp,buf,get_pot_(),get_light_(),nb_buf);
	print_dbg(string);

	print_dbg("\r pop\n");
	valtmp=pop_light_();
	sprintf(string,"\r val=%d buffer=%x val=%d nb_buf=%d\n",valtmp,buf,valtmp,nb_buf);
	print_dbg(string);
	
	//clean up the shit we did
	buf=0;
	nb_buf=0;
	nb_buf2=0;
}

/**
 * Fonction d'affichage de la taille de nos different registre
 */
void sizeof_regs(void){
	
	char string[MAXLINE];

	string[MAXLINE-1]='\0';
	print_dbg("\r Sizeof\n");

	sprintf(string,"\r sizeof(reg)=%d\n",sizeof(struct reg_state));
	print_dbg(string);
	sprintf(string,"\r sizeof(reg2)=%d\n",sizeof(struct reg_acd_val));
	print_dbg(string);
	sprintf(string,"\r sizeof(reg3)=%d\n",sizeof(struct reg_acd_ok));
	print_dbg(string);
	sprintf(string,"\r sizeof(reg4)=%d\n",sizeof(struct reg_acd_freq));
	print_dbg(string);
}

/**
 * Simple test de verification que notre switch de mode s'effectue correctement
 * (util pour debug)
 */
void test_switchmode(void){
	struct reg testmode; //on use la meme struct que main pour test
	U8 i;
	char string[MAXLINE];
	
	
	for(i=0; i<16; i++){
		testmode.labo_mode++;
		switch(testmode.labo_mode){
			case 0:
				sprintf(string,"\r shouldbe=0 labomode=%d\n",testmode.labo_mode);
				print_dbg(string);
				break;
			case 1:
				sprintf(string,"\r shouldbe=1  labomode=%d\n",testmode.labo_mode);
				print_dbg(string);
			break;
			case 2:
				//smthing
				sprintf(string,"\r shouldbe=2  labomode=%d\n",testmode.labo_mode);
				print_dbg(string);
			break;
			case 3:
			sprintf(string,"\r shouldbe=3  labomode=%d\n",testmode.labo_mode);
			print_dbg(string);
			break;
		}
	}
}

#endif
