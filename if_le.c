/*	$NetBSD: if_le.c,v 1.36 1995/10/07 09:19:13 mycroft Exp $	*/

/*-
 * Copyright (c) 1995 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)if_le.c	8.2 (Berkeley) 11/16/93
 */

#define DEBUG_STARTUP 0

#include "am7990_adds.h"

#include "add_types.h"

#include "aix_io.h"

#include <sys/i386/mmu386.h>
#include <sys/i386/intr86.h>

/* #include "bpfilter.h" */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
/* #include <sys/device.h> */

#include <stdio.h>

#include <net/if.h>

#ifdef INET
#include <netinet/in.h>
/* #include <netinet/if_ether.h> */
#endif

/* #include <vm/vm.h> */
#include <sys/vm.h>
#include <sys/vmalloc.h>

/*
#include <machine/cpu.h>
#include <machine/pio.h>
*/
/*
#include "isa.h"
#include "pci.h"
*/

#if NISA > 0
/*
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>
#include <i386/isa/isa_machdep.h>
*/
#endif

#if NPCI > 0
/*
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
*/
#endif

#include <dev/isa/if_levar.h>
#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

char *card_type[] = {"unknown", "BICC Isolan", "NE2100", "DEPCA", "PCnet-ISA", "PCnet-PCI"};

/* rak: we don't have the netbsd configuration infrastructure, just 
hardcode for our purposes */

#define NUM_INTERFACES 1

struct le_softc hardcoded_le_softc[NUM_INTERFACES];

#define LE_SOFTC(unit) (&hardcoded_le_softc[(unit)])

struct isa_attach_args hardcoded_isa_attach_args[NUM_INTERFACES];

#define ISA_ATTACH_ARGS(unit) (&hardcoded_isa_attach_args[(unit)])

int hardcoded_le_busy[NUM_INTERFACES];

#define LE_BUSY(unit) hardcoded_le_busy[(unit)]

/*
#define	LE_SOFTC(unit)	lecd.cd_devs[unit]
Too long:
#define	LE_DELAY(x) delayticks(1);
*/
#define LE_DELAY(x) delayloop(x);
/* rak: in netbsd this is delay(x) 
        and the param is microsec. */

int leprobe __P((/* struct device *,*/ void *, void *));
int depca_probe __P((struct le_softc *, struct isa_attach_args *));
int ne2100_probe __P((struct le_softc *, struct isa_attach_args *));
int bicc_probe __P((struct le_softc *, struct isa_attach_args *));
int lance_probe __P((struct le_softc *));
void leattach __P((/* struct device *, */ struct device *, void *));

int leintr __P(());
int leintredge __P(());
void leshutdown __P((void *));

/*
struct cfdriver lecd = {
	NULL, "le", leprobe, leattach, DV_IFNET, sizeof(struct le_softc)
};
*/

integrate void
lewrcsr(sc, port, val)
	struct le_softc *sc;
	u_int16_t port, val;
{
	/*printf("leaix: write CSR%d: 0x%04x\n", port, val);*/
	/*outw(sc->sc_rap, port);*/
	ioout(sc->sc_rap, port);
	/*outw(sc->sc_rdp, val);*/
	ioout(sc->sc_rdp, val);
}

integrate u_int16_t
lerdcsr(sc, port)
	struct le_softc *sc;
	u_int16_t port;
{
	u_int16_t val;

	/*outw(sc->sc_rap, port);*/
        ioout(sc->sc_rap, port);
	/*val = inw(sc->sc_rdp);*/
	val = ioin(sc->sc_rdp);
	/*printf("leaix: read CSR%d: 0x%04x\n", port, val);*/
	return (val);
}

int
leprobe(/* parent, */ match, aux)
	/* struct device *parent; */
	void *match, *aux;
{
	struct le_softc *sc = match;
/*	extern struct cfdriver isacd, pcicd; */

#if NISA > 0
	if (1 /* parent->dv_cfdata->cf_driver == &isacd */) {
		struct isa_attach_args *ia = aux;

/*		if (bicc_probe(sc, ia))
			return (1); */
		if (ne2100_probe(sc, ia))
			return (1);
/*		if (depca_probe(sc, ia))
			return (1); */
	}
#endif

#if NPCI > 0
	if (parent->dv_cfdata->cf_driver == &pcicd) {
		struct pci_attach_args *pa = aux;

		if (pa->pa_id == 0x20001022)
			return (1);
	}
#endif

	return (0);
}

#if NISA > 0
int
depca_probe(sc, ia)
	struct le_softc *sc;
        struct isa_attach_args *ia;
{
        int iobase = ia->ia_iobase, port;
	/* u_long rom_sum; */
	/* u_char x; */
	int i;

	sc->sc_rap = iobase + DEPCA_RAP;
	sc->sc_rdp = iobase + DEPCA_RDP;
	sc->sc_card = DEPCA;

	if (lance_probe(sc) == 0)
		return 0;

	outb(iobase + DEPCA_CSR, DEPCA_CSR_DUM);

	printf("extract mac address\n");

	/*
	 * Extract the physical MAC address from the ROM.
	 *
	 * The address PROM is 32 bytes wide, and we access it through
	 * a single I/O port.  On each read, it rotates to the next
	 * position.  We find the ethernet address by looking for a
	 * particular sequence of bytes (0xff, 0x00, 0x55, 0xaa, 0xff,
	 * 0x00, 0x55, 0xaa), and then reading the next 8 bytes (the
	 * ethernet address and a checksum).
	 *
	 * It appears that the PROM can be at one of two locations, so
	 * we just try both.
	 */
	port = iobase + DEPCA_ADP;
	for (i = 0; i < 32; i++)
		if (ioinb(port) == 0xff && ioinb(port) == 0x00 &&
		    ioinb(port) == 0x55 && ioinb(port) == 0xaa &&
		    ioinb(port) == 0xff && ioinb(port) == 0x00 &&
		    ioinb(port) == 0x55 && ioinb(port) == 0xaa)
			goto found;
	port = DEPCA_ADP + 1;
	for (i = 0; i < 32; i++)
		if (ioinb(port) == 0xff && ioinb(port) == 0x00 &&
		    ioinb(port) == 0x55 && ioinb(port) == 0xaa &&
		    ioinb(port) == 0xff && ioinb(port) == 0x00 &&
		    ioinb(port) == 0x55 && ioinb(port) == 0xaa)
			goto found;
	printf("%s: address not found\n", sc->sc_dev.dv_xname);
	return 0;

found:
	for (i = 0; i < sizeof(sc->sc_arpcom.ac_enaddr); i++)
		sc->sc_arpcom.ac_enaddr[i] = ioinb(port);

#if 0
	sum =
	    (sc->sc_arpcom.ac_enaddr[0] <<  2) +
	    (sc->sc_arpcom.ac_enaddr[1] << 10) +
	    (sc->sc_arpcom.ac_enaddr[2] <<  1) +
	    (sc->sc_arpcom.ac_enaddr[3] <<  9) +
	    (sc->sc_arpcom.ac_enaddr[4] <<  0) +
	    (sc->sc_arpcom.ac_enaddr[5] <<  8);
	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);

	rom_sum = inb(port);
	rom_sum |= inb(port) << 8;

	if (sum != rom_sum) {
		printf("%s: checksum mismatch; calculated %04x != read %04x",
		    sc->sc_dev.dv_xname, sum, rom_sum);
		return 0;
	}
#endif

	outb(iobase + DEPCA_CSR, DEPCA_CSR_NORMAL);

	ia->ia_iosize = 16;
	ia->ia_drq = DRQUNK;
	return 1;
}

int
ne2100_probe(sc, ia)
	struct le_softc *sc;
	struct isa_attach_args *ia;
{
	int iobase = ia->ia_iobase;
	int i;

	sc->sc_rap = iobase + NE2100_RAP;
	sc->sc_rdp = iobase + NE2100_RDP;
#if DEBUG_STARTUP
	printf("using io addresses RAP 0x%04x RDP 0x%04x\n", sc->sc_rap, 
		sc->sc_rdp);
#endif
	sc->sc_card = NE2100;

	if (lance_probe(sc) == 0)
		return 0;

	/*
	 * Extract the physical MAC address from the ROM.
	 */
	for (i = 0; i < sizeof(sc->sc_arpcom.ac_enaddr); i++)
		sc->sc_arpcom.ac_enaddr[i] = ioinb(iobase + i);

	ia->ia_iosize = 24;
	return 1;
}

int
bicc_probe(sc, ia)
	struct le_softc *sc;
	struct isa_attach_args *ia;
{
	int iobase = ia->ia_iobase;
	int i;

	printf("bicc_probe\n");

	sc->sc_rap = iobase + BICC_RAP;
	sc->sc_rdp = iobase + BICC_RDP;
	sc->sc_card = BICC;

	if (lance_probe(sc) == 0)
		return 0;

	/*
	 * Extract the physical MAC address from the ROM.
	 */
	for (i = 0; i < sizeof(sc->sc_arpcom.ac_enaddr); i++)
		sc->sc_arpcom.ac_enaddr[i] = ioinb(iobase + i * 2);

	ia->ia_iosize = 16;
	return 1;
}

/*
 * Determine which chip is present on the card.
 */
int
lance_probe(sc)
	struct le_softc *sc;
{
	/* Stop the LANCE chip and put it in a known state. */
#if DEBUG_STARTUP
	printf("lance_probe\n");
	printf("stop the lance chip and reset\n");
#endif
	lewrcsr(sc, LE_CSR0, LE_C0_STOP);
	LE_DELAY(100); 

#if DEBUG_STARTUP
	printf("verify stop\n");
#endif
	if (lerdcsr(sc, LE_CSR0) != LE_C0_STOP)
		return 0;

	lewrcsr(sc, LE_CSR3, sc->sc_conf3);
	return 1;
}
#endif

/* leattach was ifzeroed out but I think I did that */
void
leattach(/*parent,*/ self, aux)
	struct device /* *parent, */ *self;
	void *aux;
{
	struct le_softc *sc = (void *)self;
	/* extern struct cfdriver isacd, pcicd; */

#if NPCI > 0
	if (1 /*parent->dv_cfdata->cf_driver == &pcicd*/) {
		struct pci_attach_args *pa = aux;
		int iobase;

		if (pa->pa_id == 0x20001022) {
			int i;

			if (pci_map_io(pa->pa_tag, 0x10, &iobase))
				return;

			sc->sc_rap = iobase + NE2100_RAP;
			sc->sc_rdp = iobase + NE2100_RDP;
			sc->sc_card = PCnet_PCI;

			/*
			 * Extract the physical MAC address from the ROM.
			 */
			for (i = 0; i < sizeof(sc->sc_arpcom.ac_enaddr); i++)
				sc->sc_arpcom.ac_enaddr[i] = inb(iobase + i);
		}
	}
#endif

#if NISA > 0
	if (sc->sc_card == DEPCA) {
		struct isa_attach_args *ia = aux;
		u_char *mem, val;
		int i;

                caddr_t vaddr = NULL;

		printf("leaix: card is DEPCA\n");

                mem = sc->sc_mem = MAPIN(vaddr, ia->ia_maddr, ia->ia_msize);

		val = 0xff;
		for (;;) {
			for (i = 0; i < ia->ia_msize; i++)
				mem[i] = val;
			for (i = 0; i < ia->ia_msize; i++)
				if (mem[i] != val) {
					printf("%s: failed to clear memory\n",
					    sc->sc_dev.dv_xname);
					return;
				}
			if (val == 0x00)
				break;
			val -= 0x55;
		}

		sc->sc_conf3 = LE_C3_ACON;
		sc->sc_addr = 0;
		sc->sc_memsize = ia->ia_msize;
	} else
#endif
	{
#if DEBUG_STARTUP
		printf("leaix: card is non-DEPCA\n");
#endif
		/* sc->sc_mem = malloc(16384, M_DEVBUF, M_NOWAIT); */
		sc->sc_mem = kmemalloc(16384, MA_PAGE | MA_LONGTERM);
		if (sc->sc_mem == 0) {
			printf("%s: couldn't allocate memory for card\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		bzero(sc->sc_mem, 16384);

		sc->sc_conf3 = 0;
		/* rak: kvtop: convert a kernel virtual address 
		   to a physical address -- this has a corresponding 
	   	   function here*/
		/* sc->sc_addr = KVTOP(sc->sc_mem); */
		sc->sc_addr = kvtophys(sc->sc_mem);
		sc->sc_memsize = 16384;
	}

	sc->sc_copytodesc = copytobuf_contig;
	sc->sc_copyfromdesc = copyfrombuf_contig;
	sc->sc_copytobuf = copytobuf_contig;
	sc->sc_copyfrombuf = copyfrombuf_contig;
	sc->sc_zerobuf = zerobuf_contig;

	/*sc->sc_arpcom.ac_if.if_name = lecd.cd_name; */
	sc->sc_arpcom.ac_if.if_name = "eth";
	leconfig(sc);

	printf("%s: type %s\n", sc->sc_dev.dv_xname, card_type[sc->sc_card]);

#if NISA > 0
	if (1 /* parent->dv_cfdata->cf_driver == &isacd */) {
		struct isa_attach_args *ia = aux;

		/* rak: I don't see any guidance about setup  
                   for device-mastered DMA */
		/*
		if (ia->ia_drq != DRQUNK)
			isa_dmacascade(ia->ia_drq);
		*/

		/* rak: on netbsd this is 
		isa_intr_establish(ic, irq, type, level, handler, arg) */
		/* sc->sc_ih = isa_intr_establish(ia->ia_irq, ISA_IST_EDGE,
		    ISA_IPL_NET, leintredge, sc); */
#if DEBUG_STARTUP
		printf("leaix: setting up leintredge on irq %d\n", ia->ia_irq);
#endif
		intrattach(leintredge, ia->ia_irq, SPL_IMP); 
#if DEBUG_STARTUP
		printf("leaix: done\n");
#endif
	}
#endif

#if NPCI > 0
	if (parent->dv_cfdata->cf_driver == &pcicd) {
		struct pci_attach_args *pa = aux;

		pci_conf_write(pa->pa_tag, PCI_COMMAND_STATUS_REG,
		    pci_conf_read(pa->pa_tag, PCI_COMMAND_STATUS_REG) |
		    PCI_COMMAND_MASTER_ENABLE);

		/* sc->sc_ih = pci_map_int(pa->pa_tag, PCI_IPL_NET, leintr, sc); */
		intrattach(leintredge, ia->ia_irq, SPL_IMP); 
	}
#endif

	/* FIXME: but hookup leshutdown to our reset function */
	/* sc->sc_sh = shutdownhook_establish(leshutdown, sc); */
}

void
leshutdown(arg)
	void *arg;
{
	struct le_softc *sc = arg;

	lestop(sc);
}

#if NISA > 0
/*
 * Controller interrupt.
 */
int leintredge()
{

	if (leintr() == 0)
		return (0);
	for (;;)
		if (leintr() == 0)
			return (1);
}
#endif

/*
 * Routines for accessing the transmit and receive buffers.
 */

void
copytobuf_contig(sc, from, boff, len)
	struct le_softc *sc;
	caddr_t from;
	int boff, len;
{
	volatile caddr_t buf = sc->sc_mem;

	/*
	 * Just call bcopy() to do the work.
	 */
	bcopy(from, buf + boff, len);
}

void
copyfrombuf_contig(sc, to, boff, len)
	struct le_softc *sc;
	caddr_t to;
	int boff, len;
{
	volatile caddr_t buf = sc->sc_mem;

	/*
	 * Just call bcopy() to do the work.
	 */
	bcopy(buf + boff, to, len);
}

void
zerobuf_contig(sc, boff, len)
	struct le_softc *sc;
	int boff, len;
{
	volatile caddr_t buf = sc->sc_mem;

	/*
	 * Just call bzero() to do the work.
	 */
	bzero(buf + boff, len);
}

#include <dev/ic/am7990.c>
