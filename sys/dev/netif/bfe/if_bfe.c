/*
 * Copyright (c) 2003 Stuart Walsh<stu@ipng.org.uk>
 * and Duncan Barclay<dmlb@dmlb.org>
 * Modifications for FreeBSD-stable by Edwin Groothuis
 * <edwin at mavetju.org
 * < http://lists.freebsd.org/mailman/listinfo/freebsd-bugs>>
 */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS 'AS IS' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/dev/bfe/if_bfe.c 1.4.4.7 2004/03/02 08:41:33 julian Exp  v
 * $DragonFly: src/sys/dev/netif/bfe/if_bfe.c,v 1.31 2007/07/14 08:01:24 sephe Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/bus.h>
#include <sys/rman.h>
#include <sys/thread2.h>

#include <net/if.h>
#include <net/ifq_var.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#include <net/bpf.h>

#include <net/if_types.h>
#include <net/vlan/if_vlan_var.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <bus/pci/pcireg.h>
#include <bus/pci/pcivar.h>
#include <bus/pci/pcidevs.h>

#include <dev/netif/mii_layer/mii.h>
#include <dev/netif/mii_layer/miivar.h>

#include "if_bfereg.h"

MODULE_DEPEND(bfe, pci, 1, 1, 1);
MODULE_DEPEND(bfe, miibus, 1, 1, 1);

/* "controller miibus0" required.  See GENERIC if you get errors here. */
#include "miibus_if.h"

#define BFE_DEVDESC_MAX		64	/* Maximum device description length */

static struct bfe_type bfe_devs[] = {
	{ PCI_VENDOR_BROADCOM, PCI_PRODUCT_BROADCOM_BCM4401,
	    "Broadcom BCM4401 Fast Ethernet" },
	{ PCI_VENDOR_BROADCOM, PCI_PRODUCT_BROADCOM_BCM4401B0,
	    "Broadcom BCM4401-B0 Fast Ethernet" },
	{ 0, 0, NULL }
};

static int	bfe_probe(device_t);
static int	bfe_attach(device_t);
static int	bfe_detach(device_t);
static void	bfe_intr(void *);
static void	bfe_start(struct ifnet *);
static int	bfe_ioctl(struct ifnet *, u_long, caddr_t, struct ucred *);
static void	bfe_init(void *);
static void	bfe_stop(struct bfe_softc *);
static void	bfe_watchdog(struct ifnet *);
static void	bfe_shutdown(device_t);
static void	bfe_tick(void *);
static void	bfe_txeof(struct bfe_softc *);
static void	bfe_rxeof(struct bfe_softc *);
static void	bfe_set_rx_mode(struct bfe_softc *);
static int	bfe_list_rx_init(struct bfe_softc *);
static int	bfe_list_newbuf(struct bfe_softc *, int, struct mbuf*);
static void	bfe_rx_ring_free(struct bfe_softc *);

static void	bfe_pci_setup(struct bfe_softc *, uint32_t);
static int	bfe_ifmedia_upd(struct ifnet *);
static void	bfe_ifmedia_sts(struct ifnet *, struct ifmediareq *);
static int	bfe_miibus_readreg(device_t, int, int);
static int	bfe_miibus_writereg(device_t, int, int, int);
static void	bfe_miibus_statchg(device_t);
static int	bfe_wait_bit(struct bfe_softc *, uint32_t, uint32_t,
			     u_long, const int);
static void	bfe_get_config(struct bfe_softc *sc);
static void	bfe_read_eeprom(struct bfe_softc *, uint8_t *);
static void	bfe_stats_update(struct bfe_softc *);
static void	bfe_clear_stats	(struct bfe_softc *);
static int	bfe_readphy(struct bfe_softc *, uint32_t, uint32_t*);
static int	bfe_writephy(struct bfe_softc *, uint32_t, uint32_t);
static int	bfe_resetphy(struct bfe_softc *);
static int	bfe_setupphy(struct bfe_softc *);
static void	bfe_chip_reset(struct bfe_softc *);
static void	bfe_chip_halt(struct bfe_softc *);
static void	bfe_core_reset(struct bfe_softc *);
static void	bfe_core_disable(struct bfe_softc *);
static int	bfe_dma_alloc(device_t);
static void	bfe_dma_free(struct bfe_softc *);
static void	bfe_dma_map_desc(void *, bus_dma_segment_t *, int, int);
static void	bfe_dma_map(void *, bus_dma_segment_t *, int, int);
static void	bfe_cam_write(struct bfe_softc *, u_char *, int);

static device_method_t bfe_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		bfe_probe),
	DEVMETHOD(device_attach,	bfe_attach),
	DEVMETHOD(device_detach,	bfe_detach),
	DEVMETHOD(device_shutdown,	bfe_shutdown),

	/* bus interface */
	DEVMETHOD(bus_print_child,	bus_generic_print_child),
	DEVMETHOD(bus_driver_added,	bus_generic_driver_added),

	/* MII interface */
	DEVMETHOD(miibus_readreg,	bfe_miibus_readreg),
	DEVMETHOD(miibus_writereg,	bfe_miibus_writereg),
	DEVMETHOD(miibus_statchg,	bfe_miibus_statchg),

	{ 0, 0 }
};

static driver_t bfe_driver = {
	"bfe",
	bfe_methods,
	sizeof(struct bfe_softc)
};

static devclass_t bfe_devclass;

DRIVER_MODULE(bfe, pci, bfe_driver, bfe_devclass, 0, 0);
DRIVER_MODULE(miibus, bfe, miibus_driver, miibus_devclass, 0, 0);

/*
 * Probe for a Broadcom 4401 chip. 
 */
static int
bfe_probe(device_t dev)
{
	struct bfe_type *t;
	uint16_t vendor, product;

	vendor = pci_get_vendor(dev);
	product = pci_get_device(dev);

	for (t = bfe_devs; t->bfe_name != NULL; t++) {
		if (vendor == t->bfe_vid && product == t->bfe_did) {
			device_set_desc(dev, t->bfe_name);
			return(0);
		}
	}

	return(ENXIO);
}

static int
bfe_dma_alloc(device_t dev)
{
	struct bfe_softc *sc = device_get_softc(dev);
	int error, i, tx_pos, rx_pos;

	/*
	 * parent tag.  Apparently the chip cannot handle any DMA address
	 * greater than 1GB.
	 */
	error = bus_dma_tag_create(NULL,          /* parent */
			4096, 0,                  /* alignment, boundary */
			0x3FFFFFFF,               /* lowaddr */
			BUS_SPACE_MAXADDR,        /* highaddr */
			NULL, NULL,               /* filter, filterarg */
			MAXBSIZE,                 /* maxsize */
			BUS_SPACE_UNRESTRICTED,   /* num of segments */
			BUS_SPACE_MAXSIZE_32BIT,  /* max segment size */
			0,                        /* flags */
			&sc->bfe_parent_tag);
	if (error) {
		device_printf(dev, "could not allocate parent dma tag\n");
		return(error);
	}

	/* tag for TX ring */
	error = bus_dma_tag_create(sc->bfe_parent_tag, 4096, 0,
				   BUS_SPACE_MAXADDR, BUS_SPACE_MAXADDR,
				   NULL, NULL,
				   BFE_TX_LIST_SIZE, 1,
				   BUS_SPACE_MAXSIZE_32BIT,
				   0, &sc->bfe_tx_tag);
	if (error) {
		device_printf(dev, "could not allocate dma tag for TX list\n");
		return(error);
	}

	/* tag for RX ring */
	error = bus_dma_tag_create(sc->bfe_parent_tag, 4096, 0,
				   BUS_SPACE_MAXADDR, BUS_SPACE_MAXADDR,
				   NULL, NULL,
				   BFE_RX_LIST_SIZE, 1,
				   BUS_SPACE_MAXSIZE_32BIT,
				   0, &sc->bfe_rx_tag);
	if (error) {
		device_printf(dev, "could not allocate dma tag for RX list\n");
		return(error);
	}

	/* tag for mbufs */
	error = bus_dma_tag_create(sc->bfe_parent_tag, ETHER_ALIGN, 0,
				   BUS_SPACE_MAXADDR, BUS_SPACE_MAXADDR,
				   NULL, NULL,
				   MCLBYTES, 1, BUS_SPACE_MAXSIZE_32BIT,
				   BUS_DMA_ALLOCNOW, &sc->bfe_tag);
	if (error) {
		device_printf(dev, "could not allocate dma tag for mbufs\n");
		return(error);
	}

	rx_pos = tx_pos = 0;

	/* pre allocate dmamaps for RX list */
	for (i = 0; i < BFE_RX_LIST_CNT; i++) {
		error = bus_dmamap_create(sc->bfe_tag, 0,
					  &sc->bfe_rx_ring[i].bfe_map);
		if (error) {
			rx_pos = i;
			device_printf(dev, "cannot create DMA map for RX\n");
			goto ring_fail;
		}
	}
	rx_pos = BFE_RX_LIST_CNT;

	/* pre allocate dmamaps for TX list */
	for (i = 0; i < BFE_TX_LIST_CNT; i++) {
		error = bus_dmamap_create(sc->bfe_tag, 0,
					  &sc->bfe_tx_ring[i].bfe_map);
		if (error) {
			tx_pos = i;
			device_printf(dev, "cannot create DMA map for TX\n");
			goto ring_fail;
		}
	}

	/* Alloc dma for rx ring */
	error = bus_dmamem_alloc(sc->bfe_rx_tag, (void *)&sc->bfe_rx_list,
				 BUS_DMA_WAITOK | BUS_DMA_ZERO,
				 &sc->bfe_rx_map);
	if (error) {
		device_printf(dev, "cannot allocate DMA mem for RX\n");
		return(error);
	}

	error = bus_dmamap_load(sc->bfe_rx_tag, sc->bfe_rx_map,
				sc->bfe_rx_list, sizeof(struct bfe_desc),
				bfe_dma_map, &sc->bfe_rx_dma, BUS_DMA_WAITOK);
	if (error) {
		device_printf(dev, "cannot load DMA map for RX\n");
		return(error);
	}

	bus_dmamap_sync(sc->bfe_rx_tag, sc->bfe_rx_map, BUS_DMASYNC_PREWRITE);

	/* Alloc dma for tx ring */
	error = bus_dmamem_alloc(sc->bfe_tx_tag, (void *)&sc->bfe_tx_list,
				 BUS_DMA_WAITOK | BUS_DMA_ZERO,
				 &sc->bfe_tx_map);
	if (error) {
		device_printf(dev, "cannot allocate DMA mem for TX\n");
		return(error);
	}

	error = bus_dmamap_load(sc->bfe_tx_tag, sc->bfe_tx_map,
				sc->bfe_tx_list, sizeof(struct bfe_desc),
				bfe_dma_map, &sc->bfe_tx_dma, BUS_DMA_WAITOK);
	if (error) {
		device_printf(dev, "cannot load DMA map for TX\n");
		return(error);
	}

	bus_dmamap_sync(sc->bfe_tx_tag, sc->bfe_tx_map, BUS_DMASYNC_PREWRITE);

	return(0);

ring_fail:
	for (i = 0; i < rx_pos; ++i)
		bus_dmamap_destroy(sc->bfe_tag, sc->bfe_rx_ring[i].bfe_map);
	for (i = 0; i < tx_pos; ++i)
		bus_dmamap_destroy(sc->bfe_tag, sc->bfe_tx_ring[i].bfe_map);

	bus_dma_tag_destroy(sc->bfe_tag);
	sc->bfe_tag = NULL;
	return error;
}

static int
bfe_attach(device_t dev)
{
	struct ifnet *ifp;
	struct bfe_softc *sc;
	int error = 0, rid;

	sc = device_get_softc(dev);

	sc->bfe_dev = dev;
	callout_init(&sc->bfe_stat_timer);

	/*
	 * Handle power management nonsense.
	 */
	if (pci_get_powerstate(dev) != PCI_POWERSTATE_D0) {
		uint32_t membase, irq;

		/* Save important PCI config data. */
		membase = pci_read_config(dev, BFE_PCI_MEMLO, 4);
		irq = pci_read_config(dev, BFE_PCI_INTLINE, 4);

		/* Reset the power state. */
		device_printf(dev, "chip is in D%d power mode"
			      " -- setting to D0\n", pci_get_powerstate(dev));

		pci_set_powerstate(dev, PCI_POWERSTATE_D0);

		/* Restore PCI config data. */
		pci_write_config(dev, BFE_PCI_MEMLO, membase, 4);
		pci_write_config(dev, BFE_PCI_INTLINE, irq, 4);
	}

	/*
	 * Map control/status registers.
	 */
	pci_enable_busmaster(dev);

	rid = BFE_PCI_MEMLO;
	sc->bfe_res = bus_alloc_resource_any(dev, SYS_RES_MEMORY, &rid,
	    RF_ACTIVE);
	if (sc->bfe_res == NULL) {
		device_printf(dev, "couldn't map memory\n");
		return ENXIO;
	}

	sc->bfe_btag = rman_get_bustag(sc->bfe_res);
	sc->bfe_bhandle = rman_get_bushandle(sc->bfe_res);

	/* Allocate interrupt */
	rid = 0;

	sc->bfe_irq = bus_alloc_resource_any(dev, SYS_RES_IRQ, &rid,
	    RF_SHAREABLE | RF_ACTIVE);
	if (sc->bfe_irq == NULL) {
		device_printf(dev, "couldn't map interrupt\n");
		error = ENXIO;
		goto fail;
	}

	error = bfe_dma_alloc(dev);
	if (error != 0) {
		device_printf(dev, "failed to allocate DMA resources\n");
		goto fail;
	}

	/* Set up ifnet structure */
	ifp = &sc->arpcom.ac_if;
	ifp->if_softc = sc;
	if_initname(ifp, device_get_name(dev), device_get_unit(dev));
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = bfe_ioctl;
	ifp->if_start = bfe_start;
	ifp->if_watchdog = bfe_watchdog;
	ifp->if_init = bfe_init;
	ifp->if_mtu = ETHERMTU;
	ifp->if_baudrate = 100000000;
	ifp->if_capabilities |= IFCAP_VLAN_MTU;
	ifp->if_capenable |= IFCAP_VLAN_MTU;
	ifp->if_hdrlen = sizeof(struct ether_vlan_header);
	ifq_set_maxlen(&ifp->if_snd, BFE_TX_QLEN);
	ifq_set_ready(&ifp->if_snd);

	bfe_get_config(sc);

	/* Reset the chip and turn on the PHY */
	bfe_chip_reset(sc);

	if (mii_phy_probe(dev, &sc->bfe_miibus,
				bfe_ifmedia_upd, bfe_ifmedia_sts)) {
		device_printf(dev, "MII without any PHY!\n");
		error = ENXIO;
		goto fail;
	}

	ether_ifattach(ifp, sc->arpcom.ac_enaddr, NULL);

	/*
	 * Hook interrupt last to avoid having to lock softc
	 */
	error = bus_setup_intr(dev, sc->bfe_irq, INTR_NETSAFE,
			       bfe_intr, sc, &sc->bfe_intrhand, 
			       sc->arpcom.ac_if.if_serializer);

	if (error) {
		ether_ifdetach(ifp);
		device_printf(dev, "couldn't set up irq\n");
		goto fail;
	}
	return 0;
fail:
	bfe_detach(dev);
	return(error);
}

static int
bfe_detach(device_t dev)
{
	struct bfe_softc *sc = device_get_softc(dev);
	struct ifnet *ifp = &sc->arpcom.ac_if;

	if (device_is_attached(dev)) {
		lwkt_serialize_enter(ifp->if_serializer);
		bfe_stop(sc);
		bfe_chip_reset(sc);
		bus_teardown_intr(dev, sc->bfe_irq, sc->bfe_intrhand);
		lwkt_serialize_exit(ifp->if_serializer);

		ether_ifdetach(ifp);
	}
	if (sc->bfe_miibus != NULL)
		device_delete_child(dev, sc->bfe_miibus);
	bus_generic_detach(dev);

	if (sc->bfe_irq != NULL)
		bus_release_resource(dev, SYS_RES_IRQ, 0, sc->bfe_irq);

	if (sc->bfe_res != NULL) {
		bus_release_resource(dev, SYS_RES_MEMORY, BFE_PCI_MEMLO,
				     sc->bfe_res);
	}
	bfe_dma_free(sc);

	return(0);
}

/*
 * Stop all chip I/O so that the kernel's probe routines don't
 * get confused by errant DMAs when rebooting.
 */
static void
bfe_shutdown(device_t dev)
{
	struct bfe_softc *sc = device_get_softc(dev);
	struct ifnet *ifp = &sc->arpcom.ac_if;

	lwkt_serialize_enter(ifp->if_serializer);
	bfe_stop(sc); 
	lwkt_serialize_exit(ifp->if_serializer);
}

static int
bfe_miibus_readreg(device_t dev, int phy, int reg)
{
	struct bfe_softc *sc;
	uint32_t ret;

	sc = device_get_softc(dev);
	if (phy != sc->bfe_phyaddr)
		return(0);
	bfe_readphy(sc, reg, &ret);

	return(ret);
}

static int
bfe_miibus_writereg(device_t dev, int phy, int reg, int val)
{
	struct bfe_softc *sc;

	sc = device_get_softc(dev);
	if (phy != sc->bfe_phyaddr)
		return(0);
	bfe_writephy(sc, reg, val); 

	return(0);
}

static void
bfe_miibus_statchg(device_t dev)
{
	return;
}

static void
bfe_tx_ring_free(struct bfe_softc *sc)
{
	int i;
    
	for (i = 0; i < BFE_TX_LIST_CNT; i++) {
		bus_dmamap_unload(sc->bfe_tag,
				  sc->bfe_tx_ring[i].bfe_map);
		if (sc->bfe_tx_ring[i].bfe_mbuf != NULL) {
			m_freem(sc->bfe_tx_ring[i].bfe_mbuf);
			sc->bfe_tx_ring[i].bfe_mbuf = NULL;
		}
	}
	bzero(sc->bfe_tx_list, BFE_TX_LIST_SIZE);
	bus_dmamap_sync(sc->bfe_tx_tag, sc->bfe_tx_map, BUS_DMASYNC_PREWRITE);
}

static void
bfe_rx_ring_free(struct bfe_softc *sc)
{
	int i;

	for (i = 0; i < BFE_RX_LIST_CNT; i++) {
		if (sc->bfe_rx_ring[i].bfe_mbuf != NULL) {
			bus_dmamap_unload(sc->bfe_tag,
					  sc->bfe_rx_ring[i].bfe_map);
			m_freem(sc->bfe_rx_ring[i].bfe_mbuf);
			sc->bfe_rx_ring[i].bfe_mbuf = NULL;
		}
	}
	bzero(sc->bfe_rx_list, BFE_RX_LIST_SIZE);
	bus_dmamap_sync(sc->bfe_rx_tag, sc->bfe_rx_map, BUS_DMASYNC_PREWRITE);
}

static int 
bfe_list_rx_init(struct bfe_softc *sc)
{
	int i;

	for (i = 0; i < BFE_RX_LIST_CNT; i++)
		if (bfe_list_newbuf(sc, i, NULL) == ENOBUFS) 
			return(ENOBUFS);

	bus_dmamap_sync(sc->bfe_rx_tag, sc->bfe_rx_map, BUS_DMASYNC_PREWRITE);
	CSR_WRITE_4(sc, BFE_DMARX_PTR, (i * sizeof(struct bfe_desc)));

	sc->bfe_rx_cons = 0;

	return(0);
}

static int
bfe_list_newbuf(struct bfe_softc *sc, int c, struct mbuf *m)
{
	struct bfe_rxheader *rx_header;
	struct bfe_desc *d;
	struct bfe_data *r;
	uint32_t ctrl;

	if ((c < 0) || (c >= BFE_RX_LIST_CNT))
		return(EINVAL);

	if (m == NULL) {
		m = m_getcl(MB_DONTWAIT, MT_DATA, M_PKTHDR);
		if (m == NULL)
			return(ENOBUFS);
		m->m_len = m->m_pkthdr.len = MCLBYTES;
	}
	else
		m->m_data = m->m_ext.ext_buf;

	rx_header = mtod(m, struct bfe_rxheader *);
	rx_header->len = 0;
	rx_header->flags = 0;

	/* Map the mbuf into DMA */
	sc->bfe_rx_cnt = c;
	d = &sc->bfe_rx_list[c];
	r = &sc->bfe_rx_ring[c];
	/* XXX error? */
	bus_dmamap_load(sc->bfe_tag, r->bfe_map, mtod(m, void *),
			MCLBYTES, bfe_dma_map_desc, d, BUS_DMA_NOWAIT);
	bus_dmamap_sync(sc->bfe_tag, r->bfe_map, BUS_DMASYNC_PREWRITE);

	ctrl = ETHER_MAX_LEN + 32;

	if(c == BFE_RX_LIST_CNT - 1)
		ctrl |= BFE_DESC_EOT;

	d->bfe_ctrl = ctrl;
	r->bfe_mbuf = m;
	bus_dmamap_sync(sc->bfe_rx_tag, sc->bfe_rx_map, BUS_DMASYNC_PREWRITE);
	return(0);
}

static void
bfe_get_config(struct bfe_softc *sc)
{
	uint8_t eeprom[128];

	bfe_read_eeprom(sc, eeprom);

	sc->arpcom.ac_enaddr[0] = eeprom[79];
	sc->arpcom.ac_enaddr[1] = eeprom[78];
	sc->arpcom.ac_enaddr[2] = eeprom[81];
	sc->arpcom.ac_enaddr[3] = eeprom[80];
	sc->arpcom.ac_enaddr[4] = eeprom[83];
	sc->arpcom.ac_enaddr[5] = eeprom[82];

	sc->bfe_phyaddr = eeprom[90] & 0x1f;
	sc->bfe_mdc_port = (eeprom[90] >> 14) & 0x1;

	sc->bfe_core_unit = 0; 
	sc->bfe_dma_offset = BFE_PCI_DMA;
}

static void
bfe_pci_setup(struct bfe_softc *sc, uint32_t cores)
{
	uint32_t bar_orig, pci_rev, val;

	bar_orig = pci_read_config(sc->bfe_dev, BFE_BAR0_WIN, 4);
	pci_write_config(sc->bfe_dev, BFE_BAR0_WIN, BFE_REG_PCI, 4);
	pci_rev = CSR_READ_4(sc, BFE_SBIDHIGH) & BFE_RC_MASK;

	val = CSR_READ_4(sc, BFE_SBINTVEC);
	val |= cores;
	CSR_WRITE_4(sc, BFE_SBINTVEC, val);

	val = CSR_READ_4(sc, BFE_SSB_PCI_TRANS_2);
	val |= BFE_SSB_PCI_PREF | BFE_SSB_PCI_BURST;
	CSR_WRITE_4(sc, BFE_SSB_PCI_TRANS_2, val);

	pci_write_config(sc->bfe_dev, BFE_BAR0_WIN, bar_orig, 4);
}

static void 
bfe_clear_stats(struct bfe_softc *sc)
{
	u_long reg;

	CSR_WRITE_4(sc, BFE_MIB_CTRL, BFE_MIB_CLR_ON_READ);
	for (reg = BFE_TX_GOOD_O; reg <= BFE_TX_PAUSE; reg += 4)
		CSR_READ_4(sc, reg);
	for (reg = BFE_RX_GOOD_O; reg <= BFE_RX_NPAUSE; reg += 4)
		CSR_READ_4(sc, reg);
}

static int 
bfe_resetphy(struct bfe_softc *sc)
{
	uint32_t val;

	bfe_writephy(sc, 0, BMCR_RESET);
	DELAY(100);
	bfe_readphy(sc, 0, &val);
	if (val & BMCR_RESET) {
		if_printf(&sc->arpcom.ac_if,
			  "PHY Reset would not complete.\n");
		return(ENXIO);
	}
	return(0);
}

static void
bfe_chip_halt(struct bfe_softc *sc)
{
	/* disable interrupts - not that it actually does..*/
	CSR_WRITE_4(sc, BFE_IMASK, 0);
	CSR_READ_4(sc, BFE_IMASK);

	CSR_WRITE_4(sc, BFE_ENET_CTRL, BFE_ENET_DISABLE);
	bfe_wait_bit(sc, BFE_ENET_CTRL, BFE_ENET_DISABLE, 200, 1);

	CSR_WRITE_4(sc, BFE_DMARX_CTRL, 0);
	CSR_WRITE_4(sc, BFE_DMATX_CTRL, 0);
	DELAY(10);
}

static void
bfe_chip_reset(struct bfe_softc *sc)
{
	uint32_t val;    

	/* Set the interrupt vector for the enet core */
	bfe_pci_setup(sc, BFE_INTVEC_ENET0);

	/* is core up? */
	val = CSR_READ_4(sc, BFE_SBTMSLOW) & (BFE_RESET | BFE_REJECT | BFE_CLOCK);
	if (val == BFE_CLOCK) {
		/* It is, so shut it down */
		CSR_WRITE_4(sc, BFE_RCV_LAZY, 0);
		CSR_WRITE_4(sc, BFE_ENET_CTRL, BFE_ENET_DISABLE);
		bfe_wait_bit(sc, BFE_ENET_CTRL, BFE_ENET_DISABLE, 100, 1);
		CSR_WRITE_4(sc, BFE_DMATX_CTRL, 0);
		sc->bfe_tx_cnt = sc->bfe_tx_prod = sc->bfe_tx_cons = 0;
		if (CSR_READ_4(sc, BFE_DMARX_STAT) & BFE_STAT_EMASK) 
			bfe_wait_bit(sc, BFE_DMARX_STAT, BFE_STAT_SIDLE, 100, 0);
		CSR_WRITE_4(sc, BFE_DMARX_CTRL, 0);
		sc->bfe_rx_prod = sc->bfe_rx_cons = 0;
	}

	bfe_core_reset(sc);
	bfe_clear_stats(sc);

	/*
	 * We want the phy registers to be accessible even when
	 * the driver is "downed" so initialize MDC preamble, frequency,
	 * and whether internal or external phy here.
	 */

	/* 4402 has 62.5Mhz SB clock and internal phy */
	CSR_WRITE_4(sc, BFE_MDIO_CTRL, 0x8d);

	/* Internal or external PHY? */
	val = CSR_READ_4(sc, BFE_DEVCTRL);
	if (!(val & BFE_IPP)) 
		CSR_WRITE_4(sc, BFE_ENET_CTRL, BFE_ENET_EPSEL);
	else if (CSR_READ_4(sc, BFE_DEVCTRL) & BFE_EPR) {
		BFE_AND(sc, BFE_DEVCTRL, ~BFE_EPR);
		DELAY(100);
	}

	/* Enable CRC32 generation and set proper LED modes */
	BFE_OR(sc, BFE_MAC_CTRL, BFE_CTRL_CRC32_ENAB | BFE_CTRL_LED);

	/* Reset or clear powerdown control bit  */
	BFE_AND(sc, BFE_MAC_CTRL, ~BFE_CTRL_PDOWN);

	CSR_WRITE_4(sc, BFE_RCV_LAZY, ((1 << BFE_LAZY_FC_SHIFT) & 
				BFE_LAZY_FC_MASK));

	/* 
	 * We don't want lazy interrupts, so just send them at the end of a
	 * frame, please 
	 */
	BFE_OR(sc, BFE_RCV_LAZY, 0);

	/* Set max lengths, accounting for VLAN tags */
	CSR_WRITE_4(sc, BFE_RXMAXLEN, ETHER_MAX_LEN+32);
	CSR_WRITE_4(sc, BFE_TXMAXLEN, ETHER_MAX_LEN+32);

	/* Set watermark XXX - magic */
	CSR_WRITE_4(sc, BFE_TX_WMARK, 56);

	/* 
	 * Initialise DMA channels - not forgetting dma addresses need to be
	 * added to BFE_PCI_DMA 
	 */
	CSR_WRITE_4(sc, BFE_DMATX_CTRL, BFE_TX_CTRL_ENABLE);
	CSR_WRITE_4(sc, BFE_DMATX_ADDR, sc->bfe_tx_dma + BFE_PCI_DMA);

	CSR_WRITE_4(sc, BFE_DMARX_CTRL, (BFE_RX_OFFSET << BFE_RX_CTRL_ROSHIFT) | 
			BFE_RX_CTRL_ENABLE);
	CSR_WRITE_4(sc, BFE_DMARX_ADDR, sc->bfe_rx_dma + BFE_PCI_DMA);

	bfe_resetphy(sc);
	bfe_setupphy(sc);
}

static void
bfe_core_disable(struct bfe_softc *sc)
{
	if ((CSR_READ_4(sc, BFE_SBTMSLOW)) & BFE_RESET)
		return;

	/* 
	 * Set reject, wait for it set, then wait for the core to stop being busy
	 * Then set reset and reject and enable the clocks
	 */
	CSR_WRITE_4(sc, BFE_SBTMSLOW, (BFE_REJECT | BFE_CLOCK));
	bfe_wait_bit(sc, BFE_SBTMSLOW, BFE_REJECT, 1000, 0);
	bfe_wait_bit(sc, BFE_SBTMSHIGH, BFE_BUSY, 1000, 1);
	CSR_WRITE_4(sc, BFE_SBTMSLOW, (BFE_FGC | BFE_CLOCK | BFE_REJECT |
				BFE_RESET));
	CSR_READ_4(sc, BFE_SBTMSLOW);
	DELAY(10);
	/* Leave reset and reject set */
	CSR_WRITE_4(sc, BFE_SBTMSLOW, (BFE_REJECT | BFE_RESET));
	DELAY(10);
}

static void
bfe_core_reset(struct bfe_softc *sc)
{
	uint32_t val;

	/* Disable the core */
	bfe_core_disable(sc);

	/* and bring it back up */
	CSR_WRITE_4(sc, BFE_SBTMSLOW, (BFE_RESET | BFE_CLOCK | BFE_FGC));
	CSR_READ_4(sc, BFE_SBTMSLOW);
	DELAY(10);

	/* Chip bug, clear SERR, IB and TO if they are set. */
	if (CSR_READ_4(sc, BFE_SBTMSHIGH) & BFE_SERR)
		CSR_WRITE_4(sc, BFE_SBTMSHIGH, 0);
	val = CSR_READ_4(sc, BFE_SBIMSTATE);
	if (val & (BFE_IBE | BFE_TO))
		CSR_WRITE_4(sc, BFE_SBIMSTATE, val & ~(BFE_IBE | BFE_TO));

	/* Clear reset and allow it to move through the core */
	CSR_WRITE_4(sc, BFE_SBTMSLOW, (BFE_CLOCK | BFE_FGC));
	CSR_READ_4(sc, BFE_SBTMSLOW);
	DELAY(10);

	/* Leave the clock set */
	CSR_WRITE_4(sc, BFE_SBTMSLOW, BFE_CLOCK);
	CSR_READ_4(sc, BFE_SBTMSLOW);
	DELAY(10);
}

static void 
bfe_cam_write(struct bfe_softc *sc, u_char *data, int index)
{
	uint32_t val;

	val  = ((uint32_t) data[2]) << 24;
	val |= ((uint32_t) data[3]) << 16;
	val |= ((uint32_t) data[4]) <<  8;
	val |= ((uint32_t) data[5]);
	CSR_WRITE_4(sc, BFE_CAM_DATA_LO, val);
	val = (BFE_CAM_HI_VALID |
			(((uint32_t) data[0]) << 8) |
			(((uint32_t) data[1])));
	CSR_WRITE_4(sc, BFE_CAM_DATA_HI, val);
	CSR_WRITE_4(sc, BFE_CAM_CTRL, (BFE_CAM_WRITE |
		    ((uint32_t)index << BFE_CAM_INDEX_SHIFT)));
	bfe_wait_bit(sc, BFE_CAM_CTRL, BFE_CAM_BUSY, 10000, 1);
}

static void 
bfe_set_rx_mode(struct bfe_softc *sc)
{
	struct ifnet *ifp = &sc->arpcom.ac_if;
 	struct ifmultiaddr  *ifma;
	uint32_t val;
	int i = 0;

	val = CSR_READ_4(sc, BFE_RXCONF);

	if (ifp->if_flags & IFF_PROMISC)
		val |= BFE_RXCONF_PROMISC;
	else
		val &= ~BFE_RXCONF_PROMISC;

	if (ifp->if_flags & IFF_BROADCAST)
		val &= ~BFE_RXCONF_DBCAST;
	else
		val |= BFE_RXCONF_DBCAST;


	CSR_WRITE_4(sc, BFE_CAM_CTRL, 0);
	bfe_cam_write(sc, sc->arpcom.ac_enaddr, i++);

 	if (ifp->if_flags & IFF_ALLMULTI) {
 		val |= BFE_RXCONF_ALLMULTI;
 	} else {
 		val &= ~BFE_RXCONF_ALLMULTI;
		LIST_FOREACH(ifma, &ifp->if_multiaddrs, ifma_link) {
 			if (ifma->ifma_addr->sa_family != AF_LINK)
 				continue;
 			bfe_cam_write(sc,
 			    LLADDR((struct sockaddr_dl *)ifma->ifma_addr), i++);
 		}
 	}

	CSR_WRITE_4(sc, BFE_RXCONF, val);
	BFE_OR(sc, BFE_CAM_CTRL, BFE_CAM_ENABLE);
}

static void
bfe_dma_map(void *arg, bus_dma_segment_t *segs, int nseg, int error)
{
	uint32_t *ptr;

	ptr = arg;
	*ptr = segs->ds_addr;
}

static void
bfe_dma_map_desc(void *arg, bus_dma_segment_t *segs, int nseg, int error)
{
	struct bfe_desc *d;

	d = arg;
	/* The chip needs all addresses to be added to BFE_PCI_DMA */
	d->bfe_addr = segs->ds_addr + BFE_PCI_DMA;
}

static void
bfe_dma_free(struct bfe_softc *sc)
{
	if (sc->bfe_tx_tag != NULL) {
		bus_dmamap_unload(sc->bfe_tx_tag, sc->bfe_tx_map);
		if (sc->bfe_tx_list != NULL) {
			bus_dmamem_free(sc->bfe_tx_tag, sc->bfe_tx_list,
					sc->bfe_tx_map);
			sc->bfe_tx_list = NULL;
		}
		bus_dma_tag_destroy(sc->bfe_tx_tag);
		sc->bfe_tx_tag = NULL;
	}

	if (sc->bfe_rx_tag != NULL) {
		bus_dmamap_unload(sc->bfe_rx_tag, sc->bfe_rx_map);
		if (sc->bfe_rx_list != NULL) {
			bus_dmamem_free(sc->bfe_rx_tag, sc->bfe_rx_list,
					sc->bfe_rx_map);
			sc->bfe_rx_list = NULL;
		}
		bus_dma_tag_destroy(sc->bfe_rx_tag);
		sc->bfe_rx_tag = NULL;
	}

	if (sc->bfe_tag != NULL) {
		int i;

		for (i = 0; i < BFE_TX_LIST_CNT; i++) {
			bus_dmamap_destroy(sc->bfe_tag,
					   sc->bfe_tx_ring[i].bfe_map);
		}
		for (i = 0; i < BFE_RX_LIST_CNT; i++) {
			bus_dmamap_destroy(sc->bfe_tag,
					   sc->bfe_rx_ring[i].bfe_map);
		}

		bus_dma_tag_destroy(sc->bfe_tag);
		sc->bfe_tag = NULL;
	}

	if (sc->bfe_parent_tag != NULL) {
		bus_dma_tag_destroy(sc->bfe_parent_tag);
		sc->bfe_parent_tag = NULL;
	}
}

static void
bfe_read_eeprom(struct bfe_softc *sc, uint8_t *data)
{
	long i;
	uint16_t *ptr = (uint16_t *)data;

	for (i = 0; i < 128; i += 2)
		ptr[i/2] = CSR_READ_4(sc, 4096 + i);
}

static int
bfe_wait_bit(struct bfe_softc *sc, uint32_t reg, uint32_t bit, 
	     u_long timeout, const int clear)
{
	u_long i;

	for (i = 0; i < timeout; i++) {
		uint32_t val = CSR_READ_4(sc, reg);

		if (clear && !(val & bit))
			break;
		if (!clear && (val & bit))
			break;
		DELAY(10);
	}
	if (i == timeout) {
		if_printf(&sc->arpcom.ac_if,
			  "BUG!  Timeout waiting for bit %08x of register "
			  "%x to %s.\n", bit, reg, 
			  (clear ? "clear" : "set"));
		return -1;
	}
	return 0;
}

static int
bfe_readphy(struct bfe_softc *sc, uint32_t reg, uint32_t *val)
{
	int err; 

	/* Clear MII ISR */
	CSR_WRITE_4(sc, BFE_EMAC_ISTAT, BFE_EMAC_INT_MII);
	CSR_WRITE_4(sc, BFE_MDIO_DATA, (BFE_MDIO_SB_START |
				(BFE_MDIO_OP_READ << BFE_MDIO_OP_SHIFT) |
				(sc->bfe_phyaddr << BFE_MDIO_PMD_SHIFT) |
				(reg << BFE_MDIO_RA_SHIFT) |
				(BFE_MDIO_TA_VALID << BFE_MDIO_TA_SHIFT)));
	err = bfe_wait_bit(sc, BFE_EMAC_ISTAT, BFE_EMAC_INT_MII, 100, 0);
	*val = CSR_READ_4(sc, BFE_MDIO_DATA) & BFE_MDIO_DATA_DATA;
	return(err);
}

static int
bfe_writephy(struct bfe_softc *sc, uint32_t reg, uint32_t val)
{
	int status;

	CSR_WRITE_4(sc, BFE_EMAC_ISTAT, BFE_EMAC_INT_MII);
	CSR_WRITE_4(sc, BFE_MDIO_DATA, (BFE_MDIO_SB_START |
				(BFE_MDIO_OP_WRITE << BFE_MDIO_OP_SHIFT) |
				(sc->bfe_phyaddr << BFE_MDIO_PMD_SHIFT) |
				(reg << BFE_MDIO_RA_SHIFT) |
				(BFE_MDIO_TA_VALID << BFE_MDIO_TA_SHIFT) |
				(val & BFE_MDIO_DATA_DATA)));
	status = bfe_wait_bit(sc, BFE_EMAC_ISTAT, BFE_EMAC_INT_MII, 100, 0);

	return status;
}

/* 
 * XXX - I think this is handled by the PHY driver, but it can't hurt to do it
 * twice
 */
static int
bfe_setupphy(struct bfe_softc *sc)
{
	uint32_t val;
	
	/* Enable activity LED */
	bfe_readphy(sc, 26, &val);
	bfe_writephy(sc, 26, val & 0x7fff); 
	bfe_readphy(sc, 26, &val);

	/* Enable traffic meter LED mode */
	bfe_readphy(sc, 27, &val);
	bfe_writephy(sc, 27, val | (1 << 6));

	return(0);
}

static void 
bfe_stats_update(struct bfe_softc *sc)
{
	u_long reg;
	uint32_t *val;

	val = &sc->bfe_hwstats.tx_good_octets;
	for (reg = BFE_TX_GOOD_O; reg <= BFE_TX_PAUSE; reg += 4)
		*val++ += CSR_READ_4(sc, reg);
	val = &sc->bfe_hwstats.rx_good_octets;
	for (reg = BFE_RX_GOOD_O; reg <= BFE_RX_NPAUSE; reg += 4)
		*val++ += CSR_READ_4(sc, reg);
}

static void
bfe_txeof(struct bfe_softc *sc)
{
	struct ifnet *ifp = &sc->arpcom.ac_if;
	uint32_t i, chipidx;

	chipidx = CSR_READ_4(sc, BFE_DMATX_STAT) & BFE_STAT_CDMASK;
	chipidx /= sizeof(struct bfe_desc);

	i = sc->bfe_tx_cons;
	/* Go through the mbufs and free those that have been transmitted */
	while (i != chipidx) {
		struct bfe_data *r = &sc->bfe_tx_ring[i];

		bus_dmamap_unload(sc->bfe_tag, r->bfe_map);
		if (r->bfe_mbuf != NULL) {
			ifp->if_opackets++;
			m_freem(r->bfe_mbuf);
			r->bfe_mbuf = NULL;
		}
		sc->bfe_tx_cnt--;
		BFE_INC(i, BFE_TX_LIST_CNT);
	}

	if (i != sc->bfe_tx_cons) {
		/* we freed up some mbufs */
		sc->bfe_tx_cons = i;
		ifp->if_flags &= ~IFF_OACTIVE;
	}
	if (sc->bfe_tx_cnt == 0)
		ifp->if_timer = 0;
	else
		ifp->if_timer = 5;
}

/* Pass a received packet up the stack */
static void
bfe_rxeof(struct bfe_softc *sc)
{
	struct ifnet *ifp = &sc->arpcom.ac_if;
	struct mbuf *m;
	struct bfe_rxheader *rxheader;
	struct bfe_data *r;
	uint32_t cons, status, current, len, flags;

	cons = sc->bfe_rx_cons;
	status = CSR_READ_4(sc, BFE_DMARX_STAT);
	current = (status & BFE_STAT_CDMASK) / sizeof(struct bfe_desc);

	while (current != cons) {
		r = &sc->bfe_rx_ring[cons];
		m = r->bfe_mbuf;
		rxheader = mtod(m, struct bfe_rxheader*);
		bus_dmamap_sync(sc->bfe_tag, r->bfe_map, BUS_DMASYNC_POSTREAD);
		len = rxheader->len;
		r->bfe_mbuf = NULL;

		bus_dmamap_unload(sc->bfe_tag, r->bfe_map);
		flags = rxheader->flags;

		len -= ETHER_CRC_LEN;

		/* flag an error and try again */
		if ((len > ETHER_MAX_LEN+32) || (flags & BFE_RX_FLAG_ERRORS)) {
			ifp->if_ierrors++;
			if (flags & BFE_RX_FLAG_SERR)
				ifp->if_collisions++;
			bfe_list_newbuf(sc, cons, m);
			BFE_INC(cons, BFE_RX_LIST_CNT);
			continue;
		}

		/* Go past the rx header */
		if (bfe_list_newbuf(sc, cons, NULL) != 0) {
			bfe_list_newbuf(sc, cons, m);
			BFE_INC(cons, BFE_RX_LIST_CNT);
			ifp->if_ierrors++;
			continue;
		}

		m_adj(m, BFE_RX_OFFSET);
		m->m_len = m->m_pkthdr.len = len;

		ifp->if_ipackets++;
		m->m_pkthdr.rcvif = ifp;

		ifp->if_input(ifp, m);
		BFE_INC(cons, BFE_RX_LIST_CNT);
	}
	sc->bfe_rx_cons = cons;
}

static void
bfe_intr(void *xsc)
{
	struct bfe_softc *sc = xsc;
	struct ifnet *ifp = &sc->arpcom.ac_if;
	uint32_t istat, imask, flag;

	istat = CSR_READ_4(sc, BFE_ISTAT);
	imask = CSR_READ_4(sc, BFE_IMASK);

	/* 
	 * Defer unsolicited interrupts - This is necessary because setting the
	 * chips interrupt mask register to 0 doesn't actually stop the
	 * interrupts
	 */
	istat &= imask;
	CSR_WRITE_4(sc, BFE_ISTAT, istat);
	CSR_READ_4(sc, BFE_ISTAT);

	/* not expecting this interrupt, disregard it */
	if (istat == 0) {
		return;
	}

	if (istat & BFE_ISTAT_ERRORS) {
		flag = CSR_READ_4(sc, BFE_DMATX_STAT);
		if (flag & BFE_STAT_EMASK)
			ifp->if_oerrors++;

		flag = CSR_READ_4(sc, BFE_DMARX_STAT);
		if (flag & BFE_RX_FLAG_ERRORS)
			ifp->if_ierrors++;

		ifp->if_flags &= ~IFF_RUNNING;
		bfe_init(sc);
	}

	/* A packet was received */
	if (istat & BFE_ISTAT_RX)
		bfe_rxeof(sc);

	/* A packet was sent */
	if (istat & BFE_ISTAT_TX)
		bfe_txeof(sc);

	/* We have packets pending, fire them out */ 
	if ((ifp->if_flags & IFF_RUNNING) && !ifq_is_empty(&ifp->if_snd))
		bfe_start(ifp);
}

static int
bfe_encap(struct bfe_softc *sc, struct mbuf **m_head, uint32_t *txidx)
{
	struct bfe_desc *d = NULL;
	struct bfe_data *r = NULL;
	struct mbuf *m;
	uint32_t frag, cur, cnt = 0;
	int error, chainlen = 0;

	KKASSERT(BFE_TX_LIST_CNT >= (2 + sc->bfe_tx_cnt));

	/*
	 * Count the number of frags in this chain to see if
	 * we need to m_defrag.  Since the descriptor list is shared
	 * by all packets, we'll m_defrag long chains so that they
	 * do not use up the entire list, even if they would fit.
	 */
	for (m = *m_head; m != NULL; m = m->m_next)
		chainlen++;

	if (chainlen > (BFE_TX_LIST_CNT / 4) ||
	    BFE_TX_LIST_CNT < (2 + chainlen + sc->bfe_tx_cnt)) {
		m = m_defrag(*m_head, MB_DONTWAIT);
		if (m == NULL) {
			m_freem(*m_head);
			return (ENOBUFS);
		}
		*m_head = m;
	}

	/*
	 * Start packing the mbufs in this chain into
	 * the fragment pointers. Stop when we run out
	 * of fragments or hit the end of the mbuf chain.
	 */
	cur = frag = *txidx;
	cnt = 0;

	for (m = *m_head; m != NULL; m = m->m_next) {
		if (m->m_len != 0) {
			KKASSERT(BFE_TX_LIST_CNT >= (2 + sc->bfe_tx_cnt + cnt));

			d = &sc->bfe_tx_list[cur];
			r = &sc->bfe_tx_ring[cur];
			d->bfe_ctrl = BFE_DESC_LEN & m->m_len;
			/* always intterupt on completion */
			d->bfe_ctrl |= BFE_DESC_IOC;
			if (cnt == 0) {
				/* Set start of frame */
				d->bfe_ctrl |= BFE_DESC_SOF;
			}
			if (cur == BFE_TX_LIST_CNT - 1) {
				/*
				 * Tell the chip to wrap to the start of the
				 * descriptor list
				 */
				d->bfe_ctrl |= BFE_DESC_EOT;
			}

			error = bus_dmamap_load(sc->bfe_tag, r->bfe_map,
						mtod(m, void *), m->m_len,
						bfe_dma_map_desc, d,
						BUS_DMA_NOWAIT);
			if (error) {
				/* XXX This should be a fatal error. */
				if_printf(&sc->arpcom.ac_if,
					  "%s bus_dmamap_load failed: %d",
					  __func__, error);
				m_freem(*m_head);
				return (ENOBUFS);
			}

			bus_dmamap_sync(sc->bfe_tag, r->bfe_map,
					BUS_DMASYNC_PREWRITE);

			frag = cur;
			BFE_INC(cur, BFE_TX_LIST_CNT);
			cnt++;
		}
	}

	sc->bfe_tx_list[frag].bfe_ctrl |= BFE_DESC_EOF;
	sc->bfe_tx_ring[frag].bfe_mbuf = *m_head;
	bus_dmamap_sync(sc->bfe_tx_tag, sc->bfe_tx_map, BUS_DMASYNC_PREWRITE);

	*txidx = cur;
	sc->bfe_tx_cnt += cnt;
	return(0);
}

/*
 * Set up to transmit a packet
 */
static void
bfe_start(struct ifnet *ifp)
{
	struct bfe_softc *sc = ifp->if_softc;
	struct mbuf *m_head = NULL;
	int idx, need_trans;

	/* 
	 * Not much point trying to send if the link is down
	 * or we have nothing to send.
	 */
	if (!sc->bfe_link)
		return;

	if (ifp->if_flags & IFF_OACTIVE)
		return;

	idx = sc->bfe_tx_prod;

	need_trans = 0;
	while (sc->bfe_tx_ring[idx].bfe_mbuf == NULL) {
		if (BFE_TX_LIST_CNT < (2 + sc->bfe_tx_cnt)) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		m_head = ifq_dequeue(&ifp->if_snd, NULL);
		if (m_head == NULL)
			break;

		/* 
		 * Pack the data into the tx ring.  If we don't have
		 * enough room, let the chip drain the ring.
		 */
		if (bfe_encap(sc, &m_head, &idx)) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}
		need_trans = 1;

		/*
		 * If there's a BPF listener, bounce a copy of this frame
		 * to him.
		 */
		BPF_MTAP(ifp, m_head);
	}

	if (!need_trans)
		return;

	sc->bfe_tx_prod = idx;
	/* Transmit - twice due to apparent hardware bug */
	CSR_WRITE_4(sc, BFE_DMATX_PTR, idx * sizeof(struct bfe_desc));
	CSR_WRITE_4(sc, BFE_DMATX_PTR, idx * sizeof(struct bfe_desc));

	/*
	 * Set a timeout in case the chip goes out to lunch.
	 */
	ifp->if_timer = 5;
}

static void
bfe_init(void *xsc)
{
	struct bfe_softc *sc = (struct bfe_softc*)xsc;
	struct ifnet *ifp = &sc->arpcom.ac_if;

	if (ifp->if_flags & IFF_RUNNING)
		return;

	bfe_stop(sc);
	bfe_chip_reset(sc);

	if (bfe_list_rx_init(sc) == ENOBUFS) {
		if_printf(ifp, "bfe_init failed. "
			  " Not enough memory for list buffers\n");
		bfe_stop(sc);
		return;
	}

	bfe_set_rx_mode(sc);

	/* Enable the chip and core */
	BFE_OR(sc, BFE_ENET_CTRL, BFE_ENET_ENABLE);
	/* Enable interrupts */
	CSR_WRITE_4(sc, BFE_IMASK, BFE_IMASK_DEF);

	bfe_ifmedia_upd(ifp);
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	callout_reset(&sc->bfe_stat_timer, hz, bfe_tick, sc);
}

/*
 * Set media options.
 */
static int
bfe_ifmedia_upd(struct ifnet *ifp)
{
	struct bfe_softc *sc = ifp->if_softc;
	struct mii_data *mii;

	mii = device_get_softc(sc->bfe_miibus);
	sc->bfe_link = 0;
	if (mii->mii_instance) {
		struct mii_softc *miisc;
		for (miisc = LIST_FIRST(&mii->mii_phys); miisc != NULL;
				miisc = LIST_NEXT(miisc, mii_list))
			mii_phy_reset(miisc);
	}
	mii_mediachg(mii);

	bfe_setupphy(sc);

	return(0);
}

/*
 * Report current media status.
 */
static void
bfe_ifmedia_sts(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct bfe_softc *sc = ifp->if_softc;
	struct mii_data *mii;

	mii = device_get_softc(sc->bfe_miibus);
	mii_pollstat(mii);
	ifmr->ifm_active = mii->mii_media_active;
	ifmr->ifm_status = mii->mii_media_status;
}

static int
bfe_ioctl(struct ifnet *ifp, u_long command, caddr_t data, struct ucred *cr)
{
	struct bfe_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *) data;
	struct mii_data *mii;
	int error = 0;

	switch (command) {
		case SIOCSIFFLAGS:
			if (ifp->if_flags & IFF_UP)
				if (ifp->if_flags & IFF_RUNNING)
					bfe_set_rx_mode(sc);
				else
					bfe_init(sc);
			else if (ifp->if_flags & IFF_RUNNING)
				bfe_stop(sc);
			break;
		case SIOCADDMULTI:
		case SIOCDELMULTI:
			if (ifp->if_flags & IFF_RUNNING)
				bfe_set_rx_mode(sc);
			break;
		case SIOCGIFMEDIA:
		case SIOCSIFMEDIA:
			mii = device_get_softc(sc->bfe_miibus);
			error = ifmedia_ioctl(ifp, ifr, &mii->mii_media,
					      command);
			break;
		default:
			error = ether_ioctl(ifp, command, data);
			break;
	}
	return error;
}

static void
bfe_watchdog(struct ifnet *ifp)
{
	struct bfe_softc *sc = ifp->if_softc;

	if_printf(ifp, "watchdog timeout -- resetting\n");

	ifp->if_flags &= ~IFF_RUNNING;
	bfe_init(sc);

	ifp->if_oerrors++;
}

static void
bfe_tick(void *xsc)
{
	struct bfe_softc *sc = xsc;
	struct mii_data *mii;
	struct ifnet *ifp = &sc->arpcom.ac_if;

	mii = device_get_softc(sc->bfe_miibus);

	lwkt_serialize_enter(ifp->if_serializer);

	bfe_stats_update(sc);
	callout_reset(&sc->bfe_stat_timer, hz, bfe_tick, sc);

	if (sc->bfe_link == NULL) {
		mii_tick(mii);
		if (!sc->bfe_link && mii->mii_media_status & IFM_ACTIVE &&
		    IFM_SUBTYPE(mii->mii_media_active) != IFM_NONE)  {
			sc->bfe_link++;
		}
		if (!sc->bfe_link)
			sc->bfe_link++;
	}
	lwkt_serialize_exit(ifp->if_serializer);
}

/*
 * Stop the adapter and free any mbufs allocated to the
 * RX and TX lists.
 */
static void
bfe_stop(struct bfe_softc *sc)
{
	struct ifnet *ifp = &sc->arpcom.ac_if;

	callout_stop(&sc->bfe_stat_timer);

	bfe_chip_halt(sc);
	bfe_tx_ring_free(sc);
	bfe_rx_ring_free(sc);

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}
