/* $NetBSD$
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

struct mtx_dma_info {
	bus_dma_tag_t tag;
	bus_dmamap_t map;
	bus_dma_segment_t seg;
	bus_addr_t paddr;
	void *vaddr;
	bus_size_t size;
};

enum mtx_rx_ring_idx : uint8_t {
	MTX_RX_RING_MCU_BOOT,
	MTX_RX_RING_MCU,
	MTX_RX_RING_MAIN,
	MTX_NUM_RX_RINGS,
};

enum mtx_tx_ring_idx : uint8_t {
	MTX_TX_RING_BAND0,
	MTX_TX_RING_MCU,
	MTX_TX_RING_MCU_FWDL,
	MTX_NUM_TX_RINGS,
};

struct mtx_ring_desc {
	uint32_t buf0;
	uint32_t ctrl;
	uint32_t buf1;
	uint32_t info;
} __packed __aligned(4);

struct mtx_rx_data {
	struct mbuf *m;
	bus_dmamap_t map;
};

struct mtx_tx_data {
	struct mbuf *m;
	bus_dmamap_t map;
};

struct mtx_ring {
	uint32_t reg_base;

	struct mtx_dma_info desc_dma;
	struct mtx_ring_desc *desc;

	int head;		/* next descriptor to prepare */
	int published;		/* last value written to CPU_IDX */
	int tail;		/* next descriptor to reclaim */
	int size;
};

struct mtx_tx_ring {
	struct mtx_ring base;

	struct mtx_tx_data *entries;
};

struct mtx_rx_ring {
	struct mtx_ring base;

	struct mtx_rx_data *entries;
};

struct mtx_softc {
	device_t sc_dev;

	struct ethercom	sc_ec;
	struct ieee80211com sc_ic;
	int (*sc_newstate)(struct ieee80211com *, enum ieee80211_state, int);

	bus_space_tag_t	sc_st;
	bus_space_handle_t sc_sh;
	pci_intr_handle_t *sc_pihp;

	bus_size_t sc_sz;
	bus_dma_tag_t sc_dmat;
	pci_chipset_tag_t sc_pct;
	pcitag_t sc_pcitag;
	void *sc_ih;
	void *sc_soft_ih;
	uint32_t sc_intr_mask;

	kmutex_t sc_mcu_send_mtx;
	kmutex_t sc_mcu_resp_mtx;
	kcondvar_t sc_mcu_cv;
	struct mbuf *sc_mcu_response;
	uint8_t sc_mcu_seq;
	uint8_t sc_mcu_wait_seq;

	struct mtx_rx_ring sc_rx_rings[MTX_NUM_RX_RINGS];
	struct mtx_tx_ring sc_tx_rings[MTX_NUM_TX_RINGS];
};
