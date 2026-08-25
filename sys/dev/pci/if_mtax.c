/* $NetBSD$
 * MediaTek MT7922 802.11ax
 * Based on Linux mt76 driver.
 *
 * Copyright (C) 2026 Michael Jones <mike@mjones.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (C) 2020-2026 MediaTek Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of MediaTek Inc. nor the names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sys/systm.h"
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/bus_proto.h>
#include <sys/condvar.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/socket.h>

#include <machine/endian.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/firmload.h>

#include <net/if_ether.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>

#include <dev/pci/if_mtaxreg.h>
#include <dev/pci/if_mtaxvar.h>

#define ARRAY_SIZE(x) (sizeof x / sizeof x[0])
#define MTAX_MCU_TIMEOUT_MS 3000

CTASSERT(MTAX_RX_BUF_SIZE <= MCLBYTES);
CTASSERT(sizeof(struct mtax_mcu_txd) == 64);
CTASSERT(sizeof(struct mtax_mcu_rxd) == 36);

struct reg_map {
    uint32_t phys;
    uint32_t maps;
    uint32_t size;
};

static struct reg_map fixed_map[] = {
    { 0x820d0000, 0x30000, 0x10000 }, /* WF_LMAC_TOP (WF_WTBLON) */
{0x820ed000, 0x24800, 0x00800},     /* WF_LMAC_TOP BN0 (WF_MIB) */
    {0x820e4000, 0x21000, 0x00400}, /* WF_LMAC_TOP BN0 (WF_TMAC) */
    {0x820e7000, 0x21e00, 0x00200}, /* WF_LMAC_TOP BN0 (WF_DMA) */
    {0x820eb000, 0x24200, 0x00400}, /* WF_LMAC_TOP BN0 (WF_LPON) */
    {0x820e2000, 0x20800, 0x00400}, /* WF_LMAC_TOP BN0 (WF_AGG) */
    {0x820e3000, 0x20c00, 0x00400}, /* WF_LMAC_TOP BN0 (WF_ARB) */
    {0x820e5000, 0x21400, 0x00800}, /* WF_LMAC_TOP BN0 (WF_RMAC) */
    {0x00400000, 0x80000, 0x10000}, /* WF_MCU_SYSRAM */
    {0x00410000, 0x90000, 0x10000}, /* WF_MCU_SYSRAM (configure register) */
    {0x40000000, 0x70000, 0x10000}, /* WF_UMAC_SYSRAM */
    {0x54000000, 0x02000, 0x01000}, /* WFDMA PCIE0 MCU DMA0 */
    {0x55000000, 0x03000, 0x01000}, /* WFDMA PCIE0 MCU DMA1 */
    {0x58000000, 0x06000, 0x01000}, /* WFDMA PCIE1 MCU DMA0 (MEM_DMA) */
    {0x59000000, 0x07000, 0x01000}, /* WFDMA PCIE1 MCU DMA1 */
    {0x7c000000, 0xf0000, 0x10000}, /* CONN_INFRA */
    {0x7c020000, 0xd0000, 0x10000}, /* CONN_INFRA, WFDMA */
    {0x7c060000, 0xe0000, 0x10000}, /* CONN_INFRA, conn_host_csr_top */
    {0x80020000, 0xb0000, 0x10000}, /* WF_TOP_MISC_OFF */
    {0x81020000, 0xc0000, 0x10000}, /* WF_TOP_MISC_ON */
    {0x820c0000, 0x08000, 0x04000}, /* WF_UMAC_TOP (PLE) */
    {0x820c8000, 0x0c000, 0x02000}, /* WF_UMAC_TOP (PSE) */
    {0x820cc000, 0x0e000, 0x01000}, /* WF_UMAC_TOP (PP) */
    {0x820cd000, 0x0f000, 0x01000}, /* WF_MDP_TOP */
    {0x74030000, 0x10000, 0x10000}, /* PCIE_MAC_IREG */
    {0x820ce000, 0x21c00, 0x00200}, /* WF_LMAC_TOP (WF_SEC) */
    {0x820cf000, 0x22000, 0x01000}, /* WF_LMAC_TOP (WF_PF) */
    {0x820e0000, 0x20000, 0x00400}, /* WF_LMAC_TOP BN0 (WF_CFG) */
    {0x820e1000, 0x20400, 0x00200}, /* WF_LMAC_TOP BN0 (WF_TRB) */
    {0x820e9000, 0x23400, 0x00200}, /* WF_LMAC_TOP BN0 (WF_WTBLOFF) */
    {0x820ea000, 0x24000, 0x00200}, /* WF_LMAC_TOP BN0 (WF_ETBF) */
    {0x820ec000, 0x24600, 0x00200}, /* WF_LMAC_TOP BN0 (WF_INT) */
    {0x820f0000, 0xa0000, 0x00400}, /* WF_LMAC_TOP BN1 (WF_CFG) */
    {0x820f1000, 0xa0600, 0x00200}, /* WF_LMAC_TOP BN1 (WF_TRB) */
    {0x820f2000, 0xa0800, 0x00400}, /* WF_LMAC_TOP BN1 (WF_AGG) */
    {0x820f3000, 0xa0c00, 0x00400}, /* WF_LMAC_TOP BN1 (WF_ARB) */
    {0x820f4000, 0xa1000, 0x00400}, /* WF_LMAC_TOP BN1 (WF_TMAC) */
    {0x820f5000, 0xa1400, 0x00800}, /* WF_LMAC_TOP BN1 (WF_RMAC) */
    {0x820f7000, 0xa1e00, 0x00200}, /* WF_LMAC_TOP BN1 (WF_DMA) */
    {0x820f9000, 0xa3400, 0x00200}, /* WF_LMAC_TOP BN1 (WF_WTBLOFF) */
    {0x820fa000, 0xa4000, 0x00200}, /* WF_LMAC_TOP BN1 (WF_ETBF) */
    {0x820fb000, 0xa4200, 0x00400}, /* WF_LMAC_TOP BN1 (WF_LPON) */
    {0x820fc000, 0xa4600, 0x00200}, /* WF_LMAC_TOP BN1 (WF_INT) */
    {0x820fd000, 0xa4800, 0x00800}, /* WF_LMAC_TOP BN1 (WF_MIB) */
};

static uint32_t mtax_reg_map_l1(struct mtax_softc *sc, uint32_t addr);
static uint32_t mtax_reg_addr(struct mtax_softc *sc, uint32_t addr);
static void mtax_rmw(struct mtax_softc *sc, uint32_t addr, uint32_t val, uint32_t mask);
static uint32_t mtax_read(struct mtax_softc *sc, uint32_t addr);
static void mtax_write(struct mtax_softc *sc, uint32_t addr, uint32_t val);
static int mtax_mcu_fw_pmctrl(struct mtax_softc *sc);
static int mtax_mcu_drv_pmctrl(struct mtax_softc *sc);
static int mtax_dma_alloc(struct mtax_softc *sc, struct mtax_dma_info *dma,
    bus_size_t size, bus_size_t alignment);
static int mtax_tx_ring_reclaim(struct mtax_softc *sc,
    struct mtax_tx_ring *tx_ring);
static void mtax_rx_ring_process(struct mtax_softc *sc,
    struct mtax_rx_ring *rx_ring);
static void mtax_mcu_rx(struct mtax_softc *sc, struct mbuf *m);

static uint32_t
mtax_reg_map_l1(struct mtax_softc *sc, uint32_t addr)
{
	uint32_t offset = addr & 0xFFFF;
	uint32_t base = addr >> 16;

	mtax_rmw(sc, MTAX_REMAP_L1, base, 0xFFFF);
	/* use read to push write */
	mtax_read(sc, MTAX_REMAP_L1);

	return MTAX_REMAP_BASE_L1 + offset;
}

static uint32_t
mtax_reg_addr(struct mtax_softc *sc, uint32_t addr)
{
	if (addr < 0x100000)
		return addr;

	for (int i = 0; i < ARRAY_SIZE(fixed_map); i++) {
		uint32_t offset;

		if (addr < fixed_map[i].phys)
			continue;

		offset = addr - fixed_map[i].phys;
		if (offset > fixed_map[i].size)
			continue;

		return fixed_map[i].maps + offset;
	}

	if ((addr >= 0x18000000 && addr < 0x18c00000) ||
	    (addr >= 0x70000000 && addr < 0x78000000) ||
	    (addr >= 0x7c000000 && addr < 0x7c400000))
		return mtax_reg_map_l1(sc, addr);

	device_printf(sc->sc_dev, "Access to unsupported address: %08x\n", addr);
	return 0;
}

static void
mtax_rmw(struct mtax_softc *sc, uint32_t addr, uint32_t val, uint32_t mask)
{
	uint32_t bus_addr = mtax_reg_addr(sc, addr);

	uint32_t prev = bus_space_read_4(sc->sc_st, sc->sc_sh, bus_addr);

	val |= prev & ~mask;
	bus_space_write_4(sc->sc_st, sc->sc_sh, bus_addr, val);
}

static uint32_t
mtax_read(struct mtax_softc *sc, uint32_t addr)
{
	uint32_t bus_addr = mtax_reg_addr(sc, addr);
	return bus_space_read_4(sc->sc_st, sc->sc_sh, bus_addr);
}

static void
mtax_write(struct mtax_softc *sc, uint32_t addr, uint32_t val)
{
	uint32_t bus_addr = mtax_reg_addr(sc, addr);
	bus_space_write_4(sc->sc_st, sc->sc_sh, bus_addr, val);
}

static void
mtax_clear(struct mtax_softc *sc, uint32_t addr, uint32_t mask)
{
	mtax_rmw(sc, addr, 0, mask);
}

static void
mtax_set(struct mtax_softc *sc, uint32_t addr, uint32_t mask)
{
	mtax_rmw(sc, addr, mask, 0);
}

static bool
mtax_poll_msec(struct mtax_softc *sc, uint32_t addr, uint32_t mask,
	      uint32_t val, int timeout, int tick)
{
	int count, pause_ticks;
	int cur;

	KASSERT(tick > 0);
	pause_ticks = mstohz(tick);
	KASSERT(pause_ticks > 0);
	count = timeout / tick;

	do {
		cur = mtax_read(sc, addr) & mask;
		if (cur == val) return true;
		kpause("mtaxpoll", false, pause_ticks, NULL);
	} while (count-- > 0);

	return false;
}

static int
mtax_intr(void *arg)
{
	struct mtax_softc *sc = arg;
	uint32_t intr;

	intr = mtax_read(sc, MTAX_WFDMA0_HOST_INT_STA) & sc->sc_intr_mask;
	if (intr == 0)
		return 0;

	mtax_write(sc, MTAX_WFDMA0_HOST_INT_ENA, 0);
	softint_schedule(sc->sc_soft_ih);

	return 1;
}

static void
mtax_softintr(void *arg)
{
	struct mtax_softc *sc = arg;
	uint32_t intr;

	intr = mtax_read(sc, MTAX_WFDMA0_HOST_INT_STA) & sc->sc_intr_mask;
	if (intr != 0)
		mtax_write(sc, MTAX_WFDMA0_HOST_INT_STA, intr);

	if (intr & MTAX_INT_TX_DONE_FWDL) {
		mtax_tx_ring_reclaim(sc,
		    &sc->sc_tx_rings[MTAX_TX_RING_MCU_FWDL]);
	}
	if (intr & MTAX_INT_TX_DONE_MCU)
		mtax_tx_ring_reclaim(sc, &sc->sc_tx_rings[MTAX_TX_RING_MCU]);
	if (intr & MTAX_INT_RX_DONE_MCU_BOOT) {
		mtax_rx_ring_process(sc,
		    &sc->sc_rx_rings[MTAX_RX_RING_MCU_BOOT]);
	}
	if (intr & MTAX_INT_RX_DONE_MCU)
		mtax_rx_ring_process(sc, &sc->sc_rx_rings[MTAX_RX_RING_MCU]);

	mtax_write(sc, MTAX_WFDMA0_HOST_INT_ENA, sc->sc_intr_mask);
}

static int
mtax_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_MEDIATEK)
		return 0;

	if (PCI_PRODUCT(pa->pa_id) != 0x0616)
		return 0;

	return 1;
}

static int
mtax_mcu_fw_pmctrl(struct mtax_softc *sc)
{
	const size_t retry_count = 10;
	int i;

	for (i = 0; i < retry_count; i++) {
		mtax_write(sc, MTAX_CONN_ON_LPCTL, MTAX_LPCR_HOST_SET_OWN);

		if (mtax_poll_msec(sc, MTAX_CONN_ON_LPCTL, MTAX_LPCR_HOST_OWN_SYNC,
				  4, 50, 10))
			break;
	}

	if (i == retry_count) {
		device_printf(sc->sc_dev, "fw own failed");
		return -EIO;
	}
	return 0;
}

static int
mtax_mcu_drv_pmctrl(struct mtax_softc *sc)
{
	const size_t retry_count = 10;
	int i;

	for (i = 0; i < retry_count; i++) {
		mtax_write(sc, MTAX_CONN_ON_LPCTL, MTAX_LPCR_HOST_CLR_OWN);

		delay(3000);

		if (mtax_poll_msec(sc, MTAX_CONN_ON_LPCTL, MTAX_LPCR_HOST_OWN_SYNC,
				  0, 50, 10))
			break;
	}

	if (i == retry_count) {
		device_printf(sc->sc_dev, "driver own failed");
		return -EIO;
	}
	return 0;
}

static int
mtax_wfsys_reset(struct mtax_softc *sc)
{
	mtax_clear(sc, MTAX_WFSYS_RESET, MTAX_WFSYS_SW_RST_B);
	kpause("mtaxpause", false, mstohz(50), NULL);
	mtax_set(sc, MTAX_WFSYS_RESET, MTAX_WFSYS_SW_RST_B);

	if (!mtax_poll_msec(sc, MTAX_WFSYS_RESET, MTAX_WFSYS_SW_INIT_DONE,
			MTAX_WFSYS_SW_INIT_DONE, 500, 10))
		return ETIMEDOUT;

	return 0;
}

static int
mtax_wfdma_disable(struct mtax_softc *sc, bool reset)
{
	uint32_t busy_mask, disable_mask, reset_mask;

	disable_mask = MTAX_WFDMA0_GLO_CFG_TX_DMA_EN |
	    MTAX_WFDMA0_GLO_CFG_RX_DMA_EN |
	    MTAX_WFDMA0_GLO_CFG_CSR_DISP_BASE_PTR_CHAIN_EN |
	    MTAX_WFDMA0_GLO_CFG_OMIT_TX_INFO |
	    MTAX_WFDMA0_GLO_CFG_OMIT_RX_INFO |
	    MTAX_WFDMA0_GLO_CFG_OMIT_RX_INFO_PFET2;
	mtax_clear(sc, MTAX_WFDMA0_GLO_CFG, disable_mask);

	busy_mask = MTAX_WFDMA0_GLO_CFG_TX_DMA_BUSY |
	    MTAX_WFDMA0_GLO_CFG_RX_DMA_BUSY;
	if (!mtax_poll_msec(sc, MTAX_WFDMA0_GLO_CFG, busy_mask, 0, 100, 10))
		return ETIMEDOUT;

	mtax_clear(sc, MTAX_WFDMA0_GLO_CFG_EXT0,
	    MTAX_WFDMA0_CSR_TX_DMASHDL_ENABLE);
	mtax_set(sc, MTAX_DMASHDL_SW_CONTROL, MTAX_DMASHDL_DMASHDL_BYPASS);

	if (reset) {
		reset_mask = MTAX_WFDMA0_RST_DMASHDL_ALL_RST |
		    MTAX_WFDMA0_RST_LOGIC_RST;
		mtax_clear(sc, MTAX_WFDMA0_RST, reset_mask);
		mtax_set(sc, MTAX_WFDMA0_RST, reset_mask);
	}

	return 0;
}

static void
mtax_wfdma_prefetch(struct mtax_softc *sc)
{
	/* RX rings */
	mtax_write(sc, MTAX_WFDMA0_RX_RING_EXT_CTRL(0),
	    MTAX_WFDMA0_PREFETCH(0x000, 0x4));
	mtax_write(sc, MTAX_WFDMA0_RX_RING_EXT_CTRL(2),
	    MTAX_WFDMA0_PREFETCH(0x040, 0x4));
	mtax_write(sc, MTAX_WFDMA0_RX_RING_EXT_CTRL(3),
	    MTAX_WFDMA0_PREFETCH(0x080, 0x4));
	mtax_write(sc, MTAX_WFDMA0_RX_RING_EXT_CTRL(4),
	    MTAX_WFDMA0_PREFETCH(0x0c0, 0x4));
	mtax_write(sc, MTAX_WFDMA0_RX_RING_EXT_CTRL(5),
	    MTAX_WFDMA0_PREFETCH(0x100, 0x4));

	/* TX rings */
	mtax_write(sc, MTAX_WFDMA0_TX_RING_EXT_CTRL(0),
	    MTAX_WFDMA0_PREFETCH(0x140, 0x4));
	mtax_write(sc, MTAX_WFDMA0_TX_RING_EXT_CTRL(1),
	    MTAX_WFDMA0_PREFETCH(0x180, 0x4));
	mtax_write(sc, MTAX_WFDMA0_TX_RING_EXT_CTRL(2),
	    MTAX_WFDMA0_PREFETCH(0x1c0, 0x4));
	mtax_write(sc, MTAX_WFDMA0_TX_RING_EXT_CTRL(3),
	    MTAX_WFDMA0_PREFETCH(0x200, 0x4));
	mtax_write(sc, MTAX_WFDMA0_TX_RING_EXT_CTRL(4),
	    MTAX_WFDMA0_PREFETCH(0x240, 0x4));
	mtax_write(sc, MTAX_WFDMA0_TX_RING_EXT_CTRL(5),
	    MTAX_WFDMA0_PREFETCH(0x280, 0x4));
	mtax_write(sc, MTAX_WFDMA0_TX_RING_EXT_CTRL(6),
	    MTAX_WFDMA0_PREFETCH(0x2c0, 0x4));
	mtax_write(sc, MTAX_WFDMA0_TX_RING_EXT_CTRL(16),
	    MTAX_WFDMA0_PREFETCH(0x340, 0x4));
	mtax_write(sc, MTAX_WFDMA0_TX_RING_EXT_CTRL(17),
	    MTAX_WFDMA0_PREFETCH(0x380, 0x4));
}

static void
mtax_wfdma_enable(struct mtax_softc *sc)
{
	uint32_t config;

	mtax_wfdma_prefetch(sc);

	mtax_write(sc, MTAX_WFDMA0_RST_DTX_PTR, UINT32_MAX);
	mtax_write(sc, MTAX_WFDMA0_PRI_DLY_INT_CFG0, 0);

	config = MTAX_WFDMA0_GLO_CFG_TX_WB_DDONE |
	    MTAX_WFDMA0_GLO_CFG_FIFO_LITTLE_ENDIAN |
	    MTAX_WFDMA0_GLO_CFG_CLK_GAT_DIS |
	    MTAX_WFDMA0_GLO_CFG_OMIT_TX_INFO |
	    __SHIFTIN(3, MTAX_WFDMA0_GLO_CFG_DMA_SIZE) |
	    MTAX_WFDMA0_GLO_CFG_FIFO_DIS_CHECK |
	    MTAX_WFDMA0_GLO_CFG_RX_WB_DDONE |
	    MTAX_WFDMA0_GLO_CFG_CSR_DISP_BASE_PTR_CHAIN_EN |
	    MTAX_WFDMA0_GLO_CFG_OMIT_RX_INFO_PFET2;
	mtax_set(sc, MTAX_WFDMA0_GLO_CFG, config);

	mtax_set(sc, MTAX_WFDMA0_GLO_CFG,
	    MTAX_WFDMA0_GLO_CFG_TX_DMA_EN |
	    MTAX_WFDMA0_GLO_CFG_RX_DMA_EN);
	mtax_set(sc, MTAX_WFDMA_DUMMY_CR, MTAX_WFDMA_NEED_REINIT);

	sc->sc_intr_mask = MTAX_INT_RX_DONE_MCU_BOOT |
	    MTAX_INT_RX_DONE_MCU |
	    MTAX_INT_TX_DONE_FWDL |
	    MTAX_INT_TX_DONE_MCU;
	mtax_write(sc, MTAX_WFDMA0_HOST_INT_ENA, sc->sc_intr_mask);
}

static int __unused
mtax_dma_alloc(struct mtax_softc *sc, struct mtax_dma_info *dma,
    bus_size_t size, bus_size_t alignment)
{
	bus_dma_tag_t tag = sc->sc_dmat;
	int error, nsegs;

	dma->tag = tag;
	dma->map = NULL;
	dma->paddr = 0;
	dma->vaddr = NULL;
	dma->size = size;

	error = bus_dmamap_create(tag, size, 1, size, 0, BUS_DMA_WAITOK,
	    &dma->map);
	if (error)
		return error;

	error = bus_dmamem_alloc(tag, size, alignment, 0, &dma->seg, 1,
	    &nsegs, BUS_DMA_WAITOK);
	if (error)
		goto destroy;

	error = bus_dmamem_map(tag, &dma->seg, nsegs, size, &dma->vaddr,
	    BUS_DMA_WAITOK | BUS_DMA_COHERENT);
	if (error)
		goto free;

	error = bus_dmamap_load(tag, dma->map, dma->vaddr, size, NULL,
	    BUS_DMA_WAITOK);
	if (error)
		goto unmap;

	dma->paddr = dma->map->dm_segs[0].ds_addr;
	memset(dma->vaddr, 0, size);

	return 0;

unmap:
	bus_dmamem_unmap(tag, dma->vaddr, size);
	dma->vaddr = NULL;
free:
	bus_dmamem_free(tag, &dma->seg, nsegs);
destroy:
	bus_dmamap_destroy(tag, dma->map);
	dma->map = NULL;
	return error;
}

static void
mtax_dma_free(struct mtax_dma_info *dma)
{
	bus_dma_tag_t tag;

	if (dma->map == NULL) {
		memset(dma, 0, sizeof(*dma));
		return;
	}

	tag = dma->tag;
	if (dma->vaddr != NULL) {
		bus_dmamap_unload(tag, dma->map);
		bus_dmamem_unmap(tag, dma->vaddr, dma->size);
		bus_dmamem_free(tag, &dma->seg, 1);
	}
	bus_dmamap_destroy(tag, dma->map);

	memset(dma, 0, sizeof(*dma));
}

static int
mtax_ring_init(struct mtax_softc *sc, struct mtax_ring *ring, int size,
    uint32_t reg_base)
{
	bus_size_t desc_size;
	int error, i;

	ring->size = size;
	ring->reg_base = reg_base;

	desc_size = size * sizeof(*ring->desc);
	error = mtax_dma_alloc(sc, &ring->desc_dma, desc_size, PAGE_SIZE);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "could not allocate descriptor ring: %d\n", error);
		return error;
	}
	ring->desc = ring->desc_dma.vaddr;

	for (i = 0; i < size; i++)
		ring->desc[i].ctrl = htole32(MTAX_DMA_CTL_DMA_DONE);

	mtax_write(sc, ring->reg_base + MTAX_RING_CPU_IDX_OFS, 0);
	mtax_write(sc, ring->reg_base + MTAX_RING_DMA_IDX_OFS, 0);
	mtax_write(sc, ring->reg_base + MTAX_RING_SIZE_OFS, size);
	mtax_write(sc, ring->reg_base + MTAX_RING_DESC_BASE_OFS,
	    (uint32_t)ring->desc_dma.paddr);

	ring->head = 0;
	ring->published = 0;
	ring->tail = 0;

	return 0;
}

/* Synchronize the half-open descriptor range [first, last). */
static void
mtax_ring_descs_sync(struct mtax_softc *sc, struct mtax_ring *ring, int first,
    int last, int ops)
{
	int ndescs, nfirst;

	KASSERT(ring->desc_dma.map != NULL);
	KASSERT(ring->size > 0);
	KASSERT(first >= 0 && first < ring->size);
	KASSERT(last >= 0 && last < ring->size);

	ndescs = (last - first + ring->size) % ring->size;
	if (ndescs == 0)
		return;

	nfirst = MIN(ndescs, ring->size - first);
	bus_dmamap_sync(sc->sc_dmat, ring->desc_dma.map,
	    first * sizeof(*ring->desc), nfirst * sizeof(*ring->desc), ops);

	if (nfirst != ndescs) {
		bus_dmamap_sync(sc->sc_dmat, ring->desc_dma.map, 0,
		    (ndescs - nfirst) * sizeof(*ring->desc), ops);
	}
}

static void
mtax_free_ring(struct mtax_softc *sc, struct mtax_ring *ring)
{
	if (ring->desc_dma.map != NULL) {
		KASSERT(ring->size > 0);
		KASSERT(ring->tail >= 0 && ring->tail < ring->size);
		KASSERT(ring->published >= 0 && ring->published < ring->size);

		mtax_ring_descs_sync(sc, ring, ring->tail, ring->published,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		mtax_write(sc, ring->reg_base + MTAX_RING_CPU_IDX_OFS, 0);
		mtax_write(sc, ring->reg_base + MTAX_RING_DMA_IDX_OFS, 0);
		mtax_write(sc, ring->reg_base + MTAX_RING_SIZE_OFS, 0);
		mtax_write(sc, ring->reg_base + MTAX_RING_DESC_BASE_OFS, 0);
	}

	ring->desc = NULL;
	mtax_dma_free(&ring->desc_dma);
	ring->reg_base = 0;
	ring->head = 0;
	ring->published = 0;
	ring->tail = 0;
	ring->size = 0;
}

static int
mtax_tx_ring_init(struct mtax_softc *sc, enum mtax_tx_ring_idx idx, int size,
    uint32_t reg_base)
{
	struct mtax_tx_ring *tx_ring = &sc->sc_tx_rings[idx];
	int error;

	error = mtax_ring_init(sc, &tx_ring->base, size, reg_base);
	if (error)
		return error;

	KASSERT(tx_ring->entries == NULL);
	tx_ring->entries = kmem_zalloc(size * sizeof(*tx_ring->entries),
	    KM_SLEEP);

	return 0;
}

static int
mtax_tx_addbuf(struct mtax_softc *sc, struct mtax_tx_ring *tx_ring,
    struct mbuf *m)
{
	struct mtax_ring *ring = &tx_ring->base;
	struct mtax_tx_data *data;
	struct mtax_ring_desc *desc;
	bus_addr_t addr;
	bus_size_t len, max_len;
	uint32_t ctrl, info;
	bool new_map = false;
	int error, idx;

	KASSERT(m != NULL);
	KASSERT(m->m_flags & M_PKTHDR);
	KASSERT(tx_ring->entries != NULL);

	if ((ring->head + 1) % ring->size == ring->tail)
		return ENOBUFS;

	idx = ring->head;
	data = &tx_ring->entries[idx];
	desc = &ring->desc[idx];
	KASSERT(data->m == NULL);

	len = m->m_pkthdr.len;
	max_len = __SHIFTOUT_MASK(MTAX_DMA_CTL_SD_LEN0);
	if (len == 0 || len > max_len)
		return EFBIG;

	if (data->map == NULL) {
		error = bus_dmamap_create(sc->sc_dmat, max_len, 1, max_len, 0,
		    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &data->map);
		if (error)
			return error;
		new_map = true;
	}

	error = bus_dmamap_load_mbuf(sc->sc_dmat, data->map, m,
	    BUS_DMA_WRITE | BUS_DMA_NOWAIT);
	if (error)
		goto destroy_map;

	KASSERT(data->map->dm_nsegs == 1);
	addr = data->map->dm_segs[0].ds_addr;
	if (((uint64_t)addr >> 36) != 0) {
		error = EFBIG;
		goto unload_map;
	}

	info = __SHIFTIN((uint64_t)addr >> 32, MTAX_DMA_CTL_SDP0_H);
	ctrl = __SHIFTIN(len, MTAX_DMA_CTL_SD_LEN0) |
	    MTAX_DMA_CTL_LAST_SEC0;
	desc->buf0 = htole32((uint32_t)addr);
	desc->buf1 = 0;
	desc->ctrl = htole32(ctrl);
	desc->info = htole32(info);

	data->m = m;
	ring->head = (ring->head + 1) % ring->size;

	return 0;

unload_map:
	bus_dmamap_unload(sc->sc_dmat, data->map);
destroy_map:
	if (new_map) {
		bus_dmamap_destroy(sc->sc_dmat, data->map);
		data->map = NULL;
	}
	return error;
}

static void
mtax_tx_ring_kick(struct mtax_softc *sc, struct mtax_tx_ring *tx_ring)
{
	struct mtax_ring *ring = &tx_ring->base;
	struct mtax_tx_data *data;
	int first, idx;

	KASSERT(ring->published >= 0 && ring->published < ring->size);
	KASSERT(ring->head >= 0 && ring->head < ring->size);

	first = ring->published;
	if (first == ring->head)
		return;

	idx = first;
	while (idx != ring->head) {
		data = &tx_ring->entries[idx];
		KASSERT(data->m != NULL);
		KASSERT(data->map != NULL);
		bus_dmamap_sync(sc->sc_dmat, data->map, 0,
		    data->map->dm_mapsize, BUS_DMASYNC_PREWRITE);

		idx = (idx + 1) % ring->size;
	}

	mtax_ring_descs_sync(sc, ring, first, ring->head,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	mtax_write(sc, ring->reg_base + MTAX_RING_CPU_IDX_OFS, ring->head);
	ring->published = ring->head;
}

static int
mtax_tx_ring_reclaim(struct mtax_softc *sc, struct mtax_tx_ring *tx_ring)
{
	struct mtax_ring *ring = &tx_ring->base;
	struct mtax_tx_data *data;
	struct mbuf *m;
	uint32_t hw_idx;
	int active, completed, dma_idx, idx, reclaimed;

	KASSERT(tx_ring->entries != NULL);
	KASSERT(ring->size > 0);
	KASSERT(ring->tail >= 0 && ring->tail < ring->size);
	KASSERT(ring->published >= 0 && ring->published < ring->size);

	reclaimed = 0;
	while (ring->tail != ring->published) {
		hw_idx = mtax_read(sc,
		    ring->reg_base + MTAX_RING_DMA_IDX_OFS);
		if (hw_idx >= (uint32_t)ring->size) {
			aprint_error_dev(sc->sc_dev,
			    "TX ring %#x has invalid DMA index %u\n",
			    ring->reg_base, hw_idx);
			break;
		}
		dma_idx = (int)hw_idx;

		active = (ring->published - ring->tail + ring->size) %
		    ring->size;
		completed = (dma_idx - ring->tail + ring->size) %
		    ring->size;
		if (completed > active) {
			aprint_error_dev(sc->sc_dev,
			    "TX ring %#x DMA index %d is outside active range\n",
			    ring->reg_base, dma_idx);
			break;
		}
		if (completed == 0)
			break;

		mtax_ring_descs_sync(sc, ring, ring->tail, dma_idx,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		while (completed-- > 0) {
			idx = ring->tail;
			data = &tx_ring->entries[idx];
			KASSERT(data->m != NULL);
			KASSERT(data->map != NULL);

			bus_dmamap_sync(sc->sc_dmat, data->map, 0,
			    data->map->dm_mapsize, BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(sc->sc_dmat, data->map);

			m = data->m;
			data->m = NULL;
			ring->tail = (idx + 1) % ring->size;
			m_freem(m);
			reclaimed++;
		}
	}

	return reclaimed;
}

static int
mtax_rx_addbuf(struct mtax_softc *sc, struct mtax_rx_ring *rx_ring, int idx)
{
	struct mtax_ring *ring = &rx_ring->base;
	struct mtax_rx_data *data;
	struct mtax_ring_desc *desc;
	bus_addr_t addr;
	struct mbuf *m;
	uint32_t buf1, ctrl;
	bool new_map = false;
	int error;

	KASSERT(idx >= 0 && idx < ring->size);
	KASSERT(rx_ring->entries != NULL);
	data = &rx_ring->entries[idx];
	desc = &ring->desc[idx];
	KASSERT(data->m == NULL);

	m = m_gethdr(M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return ENOBUFS;
	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return ENOBUFS;
	}
	m->m_len = m->m_pkthdr.len = MTAX_RX_BUF_SIZE;

	if (data->map == NULL) {
		error = bus_dmamap_create(sc->sc_dmat, MTAX_RX_BUF_SIZE, 1,
		    MTAX_RX_BUF_SIZE, 0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
		    &data->map);
		if (error)
			goto free_mbuf;
		new_map = true;
	}

	error = bus_dmamap_load_mbuf(sc->sc_dmat, data->map, m,
	    BUS_DMA_READ | BUS_DMA_NOWAIT);
	if (error)
		goto destroy_map;

	KASSERT(data->map->dm_nsegs == 1);
	addr = data->map->dm_segs[0].ds_addr;
	if (((uint64_t)addr >> 36) != 0) {
		error = EFBIG;
		goto unload_map;
	}

	buf1 = __SHIFTIN((uint64_t)addr >> 32, MTAX_DMA_CTL_SDP0_H);
	ctrl = __SHIFTIN(MTAX_RX_BUF_SIZE, MTAX_DMA_CTL_SD_LEN0);
	desc->buf0 = htole32((uint32_t)addr);
	desc->buf1 = htole32(buf1);
	desc->ctrl = htole32(ctrl);
	desc->info = 0;

	data->m = m;
	return 0;

unload_map:
	bus_dmamap_unload(sc->sc_dmat, data->map);
destroy_map:
	if (new_map) {
		bus_dmamap_destroy(sc->sc_dmat, data->map);
		data->map = NULL;
	}
free_mbuf:
	m_freem(m);
	return error;
}

static void
mtax_rx_ring_kick(struct mtax_softc *sc, struct mtax_rx_ring *rx_ring)
{
	struct mtax_ring *ring = &rx_ring->base;
	struct mtax_rx_data *data;
	int first, idx;

	KASSERT(ring->published >= 0 && ring->published < ring->size);
	KASSERT(ring->head >= 0 && ring->head < ring->size);

	first = ring->published;
	if (first == ring->head)
		return;

	idx = first;
	while (idx != ring->head) {
		data = &rx_ring->entries[idx];
		KASSERT(data->m != NULL);
		KASSERT(data->map != NULL);
		bus_dmamap_sync(sc->sc_dmat, data->map, 0,
		    data->map->dm_mapsize, BUS_DMASYNC_PREREAD);

		idx = (idx + 1) % ring->size;
	}

	mtax_ring_descs_sync(sc, ring, first, ring->head,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	mtax_write(sc, ring->reg_base + MTAX_RING_CPU_IDX_OFS, ring->head);
	ring->published = ring->head;
}

static struct mbuf *
mtax_rx_ring_dequeue(struct mtax_softc *sc, struct mtax_rx_ring *rx_ring)
{
	struct mtax_ring *ring = &rx_ring->base;
	struct mtax_rx_data *data;
	struct mtax_ring_desc *desc;
	struct mbuf *m;
	uint32_t ctrl;
	int idx, len;

	KASSERT(rx_ring->entries != NULL);
	KASSERT(ring->size > 0);
	KASSERT(ring->tail >= 0 && ring->tail < ring->size);
	KASSERT(ring->published >= 0 && ring->published < ring->size);

	if (ring->tail == ring->published)
		return NULL;

	idx = ring->tail;
	data = &rx_ring->entries[idx];
	desc = &ring->desc[idx];
	KASSERT(data->m != NULL);
	KASSERT(data->map != NULL);

	mtax_ring_descs_sync(sc, ring, idx, (idx + 1) % ring->size,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	ctrl = le32toh(desc->ctrl);
	if ((ctrl & MTAX_DMA_CTL_DMA_DONE) == 0) {
		mtax_ring_descs_sync(sc, ring, idx, (idx + 1) % ring->size,
		    BUS_DMASYNC_PREREAD);
		return NULL;
	}

	bus_dmamap_sync(sc->sc_dmat, data->map, 0,
	    data->map->dm_mapsize, BUS_DMASYNC_POSTREAD);
	bus_dmamap_unload(sc->sc_dmat, data->map);

	len = __SHIFTOUT(ctrl, MTAX_DMA_CTL_SD_LEN0);
	m = data->m;
	m->m_len = m->m_pkthdr.len = len;
	data->m = NULL;
	ring->tail = (idx + 1) % ring->size;

	return m;
}

static int
mtax_rx_ring_fill(struct mtax_softc *sc, struct mtax_rx_ring *rx_ring)
{
	struct mtax_ring *ring = &rx_ring->base;
	int error;

	error = 0;

	while ((ring->head + 1) % ring->size != ring->tail) {
		error = mtax_rx_addbuf(sc, rx_ring, ring->head);
		if (error)
			break;

		ring->head = (ring->head + 1) % ring->size;
	}

	mtax_rx_ring_kick(sc, rx_ring);

	return error;
}

static void
mtax_rx_ring_process(struct mtax_softc *sc, struct mtax_rx_ring *rx_ring)
{
	struct mbuf *m;
	int error;

	while ((m = mtax_rx_ring_dequeue(sc, rx_ring)) != NULL)
		mtax_mcu_rx(sc, m);

	error = mtax_rx_ring_fill(sc, rx_ring);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "could not refill RX ring %#x: %d\n",
		    rx_ring->base.reg_base, error);
	}
}

static int
mtax_rx_ring_init(struct mtax_softc *sc, enum mtax_rx_ring_idx idx, int size,
    uint32_t reg_base)
{
	struct mtax_rx_ring *rx_ring = &sc->sc_rx_rings[idx];
	int error;

	error = mtax_ring_init(sc, &rx_ring->base, size, reg_base);
	if (error)
		return error;

	KASSERT(rx_ring->entries == NULL);
	rx_ring->entries = kmem_zalloc(size * sizeof(*rx_ring->entries),
	    KM_SLEEP);

	return mtax_rx_ring_fill(sc, rx_ring);
}

static void
mtax_free_tx_ring(struct mtax_softc *sc, enum mtax_tx_ring_idx idx)
{
	struct mtax_tx_ring *tx_ring = &sc->sc_tx_rings[idx];
	struct mtax_ring *ring = &tx_ring->base;
	struct mtax_tx_data *data;
	int i;

	if (tx_ring->entries != NULL) {
		KASSERT(ring->size > 0);

		i = ring->tail;
		while (i != ring->published) {
			data = &tx_ring->entries[i];
			KASSERT(data->m != NULL);
			KASSERT(data->map != NULL);
			bus_dmamap_sync(sc->sc_dmat, data->map, 0,
			    data->map->dm_mapsize, BUS_DMASYNC_POSTWRITE);
			i = (i + 1) % ring->size;
		}

		for (i = 0; i < ring->size; i++) {
			data = &tx_ring->entries[i];
			if (data->m != NULL) {
				bus_dmamap_unload(sc->sc_dmat, data->map);
				m_freem(data->m);
				data->m = NULL;
			}
			if (data->map != NULL) {
				bus_dmamap_destroy(sc->sc_dmat, data->map);
				data->map = NULL;
			}
		}

		kmem_free(tx_ring->entries,
		    ring->size * sizeof(*tx_ring->entries));
		tx_ring->entries = NULL;
	}

	mtax_free_ring(sc, ring);
}

static void
mtax_free_rx_ring(struct mtax_softc *sc, enum mtax_rx_ring_idx idx)
{
	struct mtax_rx_ring *rx_ring = &sc->sc_rx_rings[idx];
	struct mtax_ring *ring = &rx_ring->base;
	struct mtax_rx_data *data;
	int i;

	if (rx_ring->entries != NULL) {
		KASSERT(ring->size > 0);

		i = ring->tail;
		while (i != ring->published) {
			data = &rx_ring->entries[i];
			KASSERT(data->m != NULL);
			KASSERT(data->map != NULL);
			bus_dmamap_sync(sc->sc_dmat, data->map, 0,
			    data->map->dm_mapsize, BUS_DMASYNC_POSTREAD);
			i = (i + 1) % ring->size;
		}

		for (i = 0; i < ring->size; i++) {
			data = &rx_ring->entries[i];
			if (data->m != NULL) {
				bus_dmamap_unload(sc->sc_dmat, data->map);
				m_freem(data->m);
				data->m = NULL;
			}
			if (data->map != NULL) {
				bus_dmamap_destroy(sc->sc_dmat, data->map);
				data->map = NULL;
			}
		}

		kmem_free(rx_ring->entries,
		    ring->size * sizeof(*rx_ring->entries));
		rx_ring->entries = NULL;
	}

	mtax_free_ring(sc, ring);
}

static void __unused
mtax_free_rings(struct mtax_softc *sc)
{
	int i;

	for (i = MTAX_NUM_RX_RINGS - 1; i >= 0; i--)
		mtax_free_rx_ring(sc, i);
	for (i = MTAX_NUM_TX_RINGS - 1; i >= 0; i--)
		mtax_free_tx_ring(sc, i);
}

static void
mtax_pci_enable(struct mtax_softc *sc)
{
	pcireg_t reg;

	reg = pci_conf_read(sc->sc_pct, sc->sc_pcitag,
	    PCI_COMMAND_STATUS_REG);
	reg |= PCI_COMMAND_MASTER_ENABLE;
	if (pci_intr_type(sc->sc_pct, sc->sc_pihp[0]) == PCI_INTR_TYPE_INTX)
		reg &= ~PCI_COMMAND_INTERRUPT_DISABLE;
	else
		reg |= PCI_COMMAND_INTERRUPT_DISABLE;
	pci_conf_write(sc->sc_pct, sc->sc_pcitag,
	    PCI_COMMAND_STATUS_REG, reg);

	mtax_write(sc, MTAX_PCIE_MAC_INT_ENABLE, 0xff);
}

static int
mtax_init_dma(struct mtax_softc *sc)
{
	int error;

	sc->sc_intr_mask = 0;
	mtax_write(sc, MTAX_WFDMA0_HOST_INT_ENA, 0);

	error = mtax_wfdma_disable(sc, true);
	if (error)
		return error;

	error = mtax_tx_ring_init(sc, MTAX_TX_RING_MCU,
	    MTAX_TX_MCU_RING_SIZE, MTAX_TX_RING_MCU_BASE);
	if (error)
		return error;

	error = mtax_tx_ring_init(sc, MTAX_TX_RING_MCU_FWDL,
	    MTAX_TX_FWDL_RING_SIZE, MTAX_TX_RING_FWDL_BASE);
	if (error)
		return error;

	error = mtax_rx_ring_init(sc, MTAX_RX_RING_MCU_BOOT,
	    MTAX_RX_MCU_BOOT_RING_SIZE, MTAX_RX_RING_MCU_BOOT_BASE);
	if (error)
		return error;

	error = mtax_rx_ring_init(sc, MTAX_RX_RING_MCU,
	    MTAX_RX_MCU_RING_SIZE, MTAX_RX_RING_MCU_BASE);
	if (error)
		return error;

	mtax_pci_enable(sc);
	mtax_wfdma_enable(sc);

	return 0;
}

static int
mtax_mcu_msg_alloc(uint8_t cid, uint8_t seq, const void *payload,
    size_t payload_len, struct mbuf **mp)
{
	struct mtax_mcu_txd *txd;
	struct mbuf *m;
	size_t msg_len;
	uint32_t val;

	KASSERT(seq > 0 && seq <= 0xf);
	KASSERT(payload != NULL || payload_len == 0);

	*mp = NULL;
	if (payload_len > MCLBYTES - sizeof(*txd))
		return EFBIG;
	msg_len = sizeof(*txd) + payload_len;

	m = m_gethdr(M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return ENOMEM;
	if (msg_len > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			return ENOMEM;
		}
	}

	txd = mtod(m, struct mtax_mcu_txd *);
	memset(txd, 0, sizeof(*txd));
	if (payload_len != 0)
		memcpy(txd + 1, payload, payload_len);

	val = __SHIFTIN((uint32_t)msg_len, MTAX_TXD0_TX_BYTES) |
	    __SHIFTIN(MTAX_TX_TYPE_CMD, MTAX_TXD0_PKT_FMT) |
	    __SHIFTIN(MTAX_TX_MCU_PORT_RX_Q0, MTAX_TXD0_Q_IDX);
	txd->txd[0] = htole32(val);

	val = MTAX_TXD1_LONG_FORMAT |
	    __SHIFTIN(MTAX_HDR_FORMAT_CMD, MTAX_TXD1_HDR_FORMAT);
	txd->txd[1] = htole32(val);

	txd->len = htole16((uint16_t)(msg_len - sizeof(txd->txd)));
	txd->pq_id = htole16(MTAX_MCU_PQ_ID(MTAX_TX_PORT_IDX_MCU,
	    MTAX_TX_MCU_PORT_RX_Q0));
	txd->cid = cid;
	txd->pkt_type = MTAX_MCU_PKT_ID;
	txd->set_query = MTAX_MCU_Q_NA;
	txd->seq = seq;
	txd->s2d_index = MTAX_MCU_S2D_H2N;

	m->m_len = m->m_pkthdr.len = (int)msg_len;
	*mp = m;

	return 0;
}

static void
mtax_mcu_rx(struct mtax_softc *sc, struct mbuf *m)
{
	struct mtax_mcu_rxd *rxd;

	KASSERT(m != NULL);
	KASSERT(m->m_flags & M_PKTHDR);

	if (m->m_pkthdr.len < (int)sizeof(*rxd)) {
		aprint_error_dev(sc->sc_dev,
		    "short MCU message (%d bytes)\n", m->m_pkthdr.len);
		m_freem(m);
		return;
	}

	rxd = mtod(m, struct mtax_mcu_rxd *);

	mutex_enter(&sc->sc_mcu_resp_mtax);
	if (sc->sc_mcu_wait_seq != 0 &&
	    sc->sc_mcu_response == NULL &&
	    rxd->seq == sc->sc_mcu_wait_seq) {
		sc->sc_mcu_response = m;
		m = NULL;
		cv_signal(&sc->sc_mcu_cv);
	}
	mutex_exit(&sc->sc_mcu_resp_mtax);

	if (m != NULL)
		m_freem(m);
}

static uint8_t
mtax_mcu_next_seq(struct mtax_softc *sc)
{
	KASSERT(mutex_owned(&sc->sc_mcu_send_mtax));

	sc->sc_mcu_seq++;
	if (sc->sc_mcu_seq > 0xf)
		sc->sc_mcu_seq = 1;

	return sc->sc_mcu_seq;
}

static int
mtax_mcu_enqueue_legacy(struct mtax_softc *sc, uint8_t cid, uint8_t seq,
    const void *payload, size_t payload_len)
{
	struct mtax_tx_ring *ring = &sc->sc_tx_rings[MTAX_TX_RING_MCU];
	struct mbuf *m;
	int error;

	KASSERT(mutex_owned(&sc->sc_mcu_send_mtax));

	error = mtax_mcu_msg_alloc(cid, seq, payload, payload_len, &m);
	if (error)
		return error;

	error = mtax_tx_addbuf(sc, ring, m);
	if (error) {
		m_freem(m);
		return error;
	}

	mtax_tx_ring_kick(sc, ring);

	return 0;
}

static int
mtax_mcu_send_legacy(struct mtax_softc *sc, uint8_t cid,
    const void *payload, size_t payload_len)
{
	uint8_t seq;
	int error;

	mutex_enter(&sc->sc_mcu_send_mtax);
	seq = mtax_mcu_next_seq(sc);
	error = mtax_mcu_enqueue_legacy(sc, cid, seq, payload, payload_len);
	mutex_exit(&sc->sc_mcu_send_mtax);

	return error;
}

/* On success, the caller owns the mbuf returned in responsep. */
static int __unused
mtax_mcu_send_legacy_wait(struct mtax_softc *sc, uint8_t cid,
    const void *payload, size_t payload_len, struct mbuf **responsep)
{
	struct mbuf *response;
	uint8_t seq;
	int error, timeout;

	KASSERT(responsep != NULL);
	*responsep = NULL;
	timeout = mstohz(MTAX_MCU_TIMEOUT_MS);
	KASSERT(timeout > 0);

	mutex_enter(&sc->sc_mcu_send_mtax);
	seq = mtax_mcu_next_seq(sc);

	mutex_enter(&sc->sc_mcu_resp_mtax);
	KASSERT(sc->sc_mcu_wait_seq == 0);
	KASSERT(sc->sc_mcu_response == NULL);
	sc->sc_mcu_wait_seq = seq;

	error = mtax_mcu_enqueue_legacy(sc, cid, seq, payload, payload_len);
	while (error == 0 && sc->sc_mcu_response == NULL)
		error = cv_timedwait(&sc->sc_mcu_cv,
		    &sc->sc_mcu_resp_mtax, timeout);

	response = sc->sc_mcu_response;
	sc->sc_mcu_response = NULL;
	sc->sc_mcu_wait_seq = 0;
	mutex_exit(&sc->sc_mcu_resp_mtax);

	if (response != NULL) {
		*responsep = response;
		error = 0;
	} else if (error == EWOULDBLOCK) {
		error = ETIMEDOUT;
	}

	mutex_exit(&sc->sc_mcu_send_mtax);

	return error;
}

static int
mtax_mcu_restart(struct mtax_softc *sc)
{
	struct {
		uint8_t power_mode;
		uint8_t rsv[3];
	} req = {
		.power_mode = 1,
	};

	return mtax_mcu_send_legacy(sc, MTAX_MCU_CMD_NIC_POWER_CTRL,
	    &req, sizeof(req));
}

static int
mtax_init_mcu(struct mtax_softc *sc)
{
	return mtax_mcu_restart(sc);
}

static void
mtax_attach_hook(device_t self)
{
	struct mtax_softc *sc = device_private(self);
	char intrbuf[PCI_INTRSTR_LEN];
	const char *intrstr;
	int err, chipid, rev;

	sc->sc_soft_ih = softint_establish(SOFTINT_NET, mtax_softintr, sc);
	if (sc->sc_soft_ih == NULL) {
		aprint_error_dev(self, "can't establish soft interrupt\n");
		return;
	}

	intrstr = pci_intr_string(sc->sc_pct, sc->sc_pihp[0], intrbuf,
	    sizeof(intrbuf));
	sc->sc_ih = pci_intr_establish_xname(sc->sc_pct, sc->sc_pihp[0],
	    IPL_NET, mtax_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "can't establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		softint_disestablish(sc->sc_soft_ih);
		sc->sc_soft_ih = NULL;
		/* TODO: clean up on failure here and elsewhere */
		return;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	err = mtax_mcu_fw_pmctrl(sc);
	if (err)
		return;

	err = mtax_mcu_drv_pmctrl(sc);
	if (err)
		return;

	chipid = mtax_read(sc, MTAX_HW_CHIPID);
	rev = mtax_read(sc, MTAX_HW_REV);
	device_printf(sc->sc_dev, "chip ID: %04x, rev %04x\n", chipid, rev);

	err = mtax_wfsys_reset(sc);
	if (err) {
		device_printf(sc->sc_dev, "wfsys reset failed");
		return;
	}

	err = mtax_init_dma(sc);
	if (err) {
		aprint_error_dev(self, "DMA initialization failed: %d\n", err);
		return;
	}

	err = mtax_init_mcu(sc);
	if (err) {
		aprint_error_dev(self, "MCU initialization failed: %d\n", err);
		return;
	}
}

static void
mtax_attach(device_t parent, device_t self, void *aux)
{
	struct mtax_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	pcireg_t reg, memtype;
	int err;

	sc->sc_dev = self;
	sc->sc_pct = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;
	sc->sc_dmat = pa->pa_dmat;
	sc->sc_intr_mask = 0;
	mutex_init(&sc->sc_mcu_send_mtax, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_mcu_resp_mtax, MUTEX_DEFAULT, IPL_SOFTNET);
	cv_init(&sc->sc_mcu_cv, "mtaxmcu");
	sc->sc_mcu_response = NULL;
	sc->sc_mcu_seq = 0;
	sc->sc_mcu_wait_seq = 0;

	/* Disable interrupts and bus-mastering until we're ready */
	reg = pci_conf_read(sc->sc_pct, sc->sc_pcitag, PCI_COMMAND_STATUS_REG);
	reg &= ~PCI_COMMAND_MASTER_ENABLE;
	reg |= PCI_COMMAND_INTERRUPT_DISABLE;
	pci_conf_write(sc->sc_pct, sc->sc_pcitag, PCI_COMMAND_STATUS_REG, reg);

	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, PCI_MAPREG_START);
	err = pci_mapreg_map(pa, PCI_MAPREG_START, memtype, 0,
	    &sc->sc_st, &sc->sc_sh, NULL, &sc->sc_sz);
	if (err) {
		aprint_error_dev(self, "can't map mem space\n");
		return;
	}

	/* Allocate interrupt handler. */
	err = pci_intr_alloc(pa, &sc->sc_pihp, NULL, 0);
	if (err) {
		aprint_error_dev(self, "can't allocate interrupt\n");
		bus_space_unmap(sc->sc_st, sc->sc_sh, sc->sc_sz);
		return;
	}

	/* Resetting needs register polls with delays, so defer it so we
	 * don't hold up boot */
	config_mountroot(self, &mtax_attach_hook);
}

CFATTACH_DECL_NEW(mtax, sizeof(struct mtax_softc), mtax_match, mtax_attach,
	NULL, NULL);
