/*
 * iux_guide.h
 * 	- guide document for iux coding and design
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Dec 26, 2010
 *
 */

/*
 * abbreviation
 *
 * 		iux		: ipx ui
 * 		cmm		: communication manager
 * 		scm		: service manager
 * 		uxm		: ux manager (viewing service, event input)
 * 		vsm		: video surface manager
 * 		icf		: iux config
 * 		ifn		: iux function
 * 		vsc		: videosystem controller
 *
 */ 		


/*
 * concept of module
 *
 * +---------------------------------------------+
 * |                                             |
 * |               service                       |
 * |                                             |
 * |     +---------------------------------+     |
 * |     |                                 |     |
 * |     |         module                  |     |
 * |     |                                 |     |
 * |     |     +---------------------+     |     |
 * |     |     |                     |     |     |
 * |     |     |                     |     |     |
 * |     |     |   tool				 |     |     |
 * |     |     |     or              |     |     |
 * |     |     |   other module      |     |     |
 * |     |     |                     |     |     |
 * |     |     +---------------------+     |     |
 * |     |                                 |     |
 * |     |                                 |     |
 * |     |                                 |     |
 * |     +---------------------------------+     |
 * |                                             |
 * |                                             |
 * |                                             |
 * +---------------------------------------------+
 *
 */


/*
 * available concepts on each level
 *
 * 		- service 	: depends on scenario, buyer, model or others
 * 		
 * 		- module 	: light depends on scenario 
 *
 *		- tool		: not dependent any other module or concept
 *					: run itself well.
 */					
 

/*
 * hierarchy of a prefix in filename
 *
 * iux -+- ix_	: just for support module, widget, useful functions or tools,
 * 		|		  not dependent on viwer, or other services
 * 		|			ex) ix_queue.c
 *      |
 *      +- vw_	: just for each viewer
 *      |			ex) vw_vwnd.c
 *      |
 *      +- scm_	: scm's sub-modules and scm itself
 *      |			ex) scm_notify.c	--> sub file
 *      |			ex) scm.c			--> main file
 *      |
 *      +- vsm_	: vsm's sub-modules and vsm itself
 *
 *
 * one service or module can be composed of 2 type of file.
 * 		- one is main file, The other is sub file.
 * 		- the main file exist only one.
 * 		- but, sub file can be multiple.
 * 		- all public interfaces are in main file.
 * 		- sub files can be had private functions.
 *
 */


      
/*
 * conventions
 *
 * 		- functions : pefix_verb()
 * 			
 * 					  	ex) scm_init();
 * 						 	scm_cleanup();
 *
 * 					: prefix_verb_XXXX()
 *
 * 						ex) cmm_get_message();
 *
 *
 * 					: prefix_is_XXXX()
 *
 * 						ex) icf_is_opend();
 *
 *
 *		- some function names are inherited from legacy NF system api
 *			ex)
 *				 scm_filesystem_is_online()
 *
 *		  it is for decreasing a bug in process of porting from SNF to IPX
 */


/*
 * rulse
 *
 * 		- use keyword 'static' all internal variables and funtions
 * 		- don't use malloc. Insteadi, use the imalloc, ifree
 * 		- don't call a public function of the service module in service thread
 *
 *
 */
