/*	$NetBSD: if_levar.h,v 1.3 1995/10/07 09:19:16 mycroft Exp $	*/

/*
 * LANCE Ethernet driver header file
 *
 * Copyright (c) 1994, 1995 Charles M. Hannum.  All rights reserved.
 *
 * Copyright (C) 1993, Paul Richards. This software may be used, modified,
 *   copied, distributed, and sold, in both source and binary form provided
 *   that the above copyright and these terms are retained. Under no
 *   circumstances is the author responsible for the proper functioning
 *   of this software, nor does the author assume any responsibility
 *   for damages incurred with its use.
 */

#include "../../../am7990_adds.h"
#include "add_types.h"
#include "if_levar_adds.h"

/* Board types */
#define	BICC		1
#define	BICC_RDP	0xc
#define	BICC_RAP	0xe
#define BICC_BDP	0x16 /* FIXME */

#define	NE2100		2
#define	PCnet_ISA	4
#define	PCnet_PCI	5
#define	NE2100_RDP	0x10
#define	NE2100_RAP	0x12
#define NE2100_BDP	0x16

#define	DEPCA		3
#define	DEPCA_CSR	0x0
#define	DEPCA_CSR_SHE		0x80	/* Shared memory enabled */
#define	DEPCA_CSR_SWAP32	0x40	/* Byte swapped */
#define	DEPCA_CSR_DUM		0x08	/* rev E compatibility */
#define	DEPCA_CSR_IM		0x04	/* Interrupt masked */
#define	DEPCA_CSR_IEN		0x02	/* Interrupt enabled */
#define	DEPCA_CSR_NORMAL \
	(DEPCA_CSR_SHE | DEPCA_CSR_DUM | DEPCA_CSR_IEN)
#define	DEPCA_RDP	0x4
#define	DEPCA_RAP	0x6
#define	DEPCA_ADP	0xc
#define DEPCA_BDP	0x16 /* FIXME */

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * arpcom.ac_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
struct le_softc {
	struct	device sc_dev;		/* base structure */
	struct	arpcom sc_arpcom;	/* Ethernet common part */

	void	(*sc_copytodesc)();	/* Copy to descriptor */
	void	(*sc_copyfromdesc)();	/* Copy from descriptor */

	void	(*sc_copytobuf)();	/* Copy to buffer */
	void	(*sc_copyfrombuf)();	/* Copy from buffer */
	void	(*sc_zerobuf)();	/* and Zero bytes in buffer */

	u_int16_t sc_conf3;		/* CSR3 value */

	void	*sc_mem;		/* base address of RAM -- CPU's view */
	u_long	sc_addr;		/* base address of RAM -- LANCE's view */
	u_long	sc_memsize;		/* size of RAM */

	int	sc_nrbuf;		/* number of receive buffers */
	int	sc_ntbuf;		/* number of transmit buffers */
	int	sc_last_rd;
	int	sc_first_td, sc_last_td, sc_no_td;

	int	sc_initaddr;
	int	sc_rmdaddr;
	int	sc_tmdaddr;
	int	sc_rbufaddr;
	int	sc_tbufaddr;

#ifdef LEDEBUG
	int	sc_debug;
#endif

	void	*sc_ih;
	void	*sc_sh;
	int	sc_card;
	
	int	sc_rap, sc_rdp, sc_bdp;		/* LANCE registers */

	unsigned int sc_chip_id;
	int sc_have_mii_support;
};

/** These are right out of newer NetBSD/OpenBSD lance/pcn code */

/*
 * Chip ID (CSR88 IDL, CSR89 IDU) values for various AMD PCnet parts.
 */
#define	CHIPID_MANFID(x)	(((x) >> 1) & 0x3ff)
#define	CHIPID_PARTID(x)	(((x) >> 12) & 0xffff)
#define	CHIPID_VER(x)		(((x) >> 28) & 0x7)

#define	PARTID_Am79c960		0x0003
#define	PARTID_Am79c961		0x2260
#define	PARTID_Am79c961A	0x2261
#define	PARTID_Am79c965		0x2430	/* yes, these... */
#define	PARTID_Am79c970		0x2430	/* ...are the same */
#define	PARTID_Am79c970A	0x2621
#define	PARTID_Am79c971		0x2623
#define	PARTID_Am79c972		0x2624
#define	PARTID_Am79c973		0x2625
#define	PARTID_Am79c978		0x2626
#define	PARTID_Am79c975		0x2627
#define	PARTID_Am79c976		0x2628
