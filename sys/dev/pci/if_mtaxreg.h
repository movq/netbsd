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

#define MTAX_INFRA_CFG_BASE	0xfe000
#define MTAX_INFRA(ofs)		(MTAX_INFRA_CFG_BASE + (ofs))

#define MTAX_REMAP_L1		MTAX_INFRA(0x24c)
#define MTAX_REMAP_BASE_L1	0x40000

#define MTAX_HW_BOUND		0x70010020
#define MTAX_HW_CHIPID		0x70010200
#define MTAX_HW_REV		0x70010204

#define MTAX_PCIE_MAC_BASE	0x10000
#define MTAX_PCIE_MAC(ofs)	(MTAX_PCIE_MAC_BASE + (ofs))
#define MTAX_PCIE_MAC_INT_ENABLE	MTAX_PCIE_MAC(0x188)

#define MTAX_CONN_ON_LPCTL	0x7c060010
#define MTAX_LPCR_HOST_SET_OWN	__BIT(0)
#define MTAX_LPCR_HOST_CLR_OWN	__BIT(1)
#define MTAX_LPCR_HOST_OWN_SYNC	__BIT(2)

#define MTAX_WFSYS_RESET		0x18000140
#define MTAX_WFSYS_SW_RST_B	1
#define MTAX_WFSYS_SW_INIT_DONE	__BIT(4)

#define MTAX_WFDMA0_BASE		0xd4000
#define MTAX_WFDMA0(ofs)		(MTAX_WFDMA0_BASE + (ofs))

#define MTAX_WFDMA0_RST			MTAX_WFDMA0(0x100)
#define MTAX_WFDMA0_RST_LOGIC_RST	__BIT(4)
#define MTAX_WFDMA0_RST_DMASHDL_ALL_RST	__BIT(5)

#define MTAX_WFDMA0_HOST_INT_STA		MTAX_WFDMA0(0x200)
#define MTAX_INT_RX_DONE_MCU_BOOT	__BIT(0)
#define MTAX_INT_RX_DONE_MCU		__BIT(22)
#define MTAX_INT_TX_DONE_FWDL		__BIT(26)
#define MTAX_INT_TX_DONE_MCU		__BIT(27)

#define MTAX_WFDMA0_HOST_INT_ENA	MTAX_WFDMA0(0x204)

#define MTAX_WFDMA0_GLO_CFG				MTAX_WFDMA0(0x208)
#define MTAX_WFDMA0_GLO_CFG_TX_DMA_EN			__BIT(0)
#define MTAX_WFDMA0_GLO_CFG_TX_DMA_BUSY			__BIT(1)
#define MTAX_WFDMA0_GLO_CFG_RX_DMA_EN			__BIT(2)
#define MTAX_WFDMA0_GLO_CFG_RX_DMA_BUSY			__BIT(3)
#define MTAX_WFDMA0_GLO_CFG_DMA_SIZE			__BITS(5, 4)
#define MTAX_WFDMA0_GLO_CFG_TX_WB_DDONE			__BIT(6)
#define MTAX_WFDMA0_GLO_CFG_FIFO_DIS_CHECK		__BIT(11)
#define MTAX_WFDMA0_GLO_CFG_FIFO_LITTLE_ENDIAN		__BIT(12)
#define MTAX_WFDMA0_GLO_CFG_RX_WB_DDONE			__BIT(13)
#define MTAX_WFDMA0_GLO_CFG_CSR_DISP_BASE_PTR_CHAIN_EN	__BIT(15)
#define MTAX_WFDMA0_GLO_CFG_OMIT_RX_INFO_PFET2		__BIT(21)
#define MTAX_WFDMA0_GLO_CFG_OMIT_RX_INFO			__BIT(27)
#define MTAX_WFDMA0_GLO_CFG_OMIT_TX_INFO			__BIT(28)
#define MTAX_WFDMA0_GLO_CFG_CLK_GAT_DIS			__BIT(30)

#define MTAX_WFDMA0_RST_DTX_PTR		MTAX_WFDMA0(0x20c)

#define MTAX_WFDMA0_GLO_CFG_EXT0			MTAX_WFDMA0(0x2b0)
#define MTAX_WFDMA0_CSR_TX_DMASHDL_ENABLE	__BIT(6)

#define MTAX_WFDMA0_PRI_DLY_INT_CFG0	MTAX_WFDMA0(0x2f0)

#define MTAX_WFDMA0_TX_RING_EXT_CTRL_BASE	MTAX_WFDMA0(0x600)
#define MTAX_WFDMA0_TX_RING_EXT_CTRL(idx)	\
	(MTAX_WFDMA0_TX_RING_EXT_CTRL_BASE + (idx) * 4)

#define MTAX_WFDMA0_RX_RING_EXT_CTRL_BASE	MTAX_WFDMA0(0x680)
#define MTAX_WFDMA0_RX_RING_EXT_CTRL(idx)	\
	(MTAX_WFDMA0_RX_RING_EXT_CTRL_BASE + (idx) * 4)

#define MTAX_WFDMA0_EXT_CTRL_MAX_CNT	__BITS(7, 0)
#define MTAX_WFDMA0_EXT_CTRL_BASE_PTR	__BITS(31, 16)
#define MTAX_WFDMA0_PREFETCH(base, depth)				\
	(__SHIFTIN((base), MTAX_WFDMA0_EXT_CTRL_BASE_PTR) |	\
	 __SHIFTIN((depth), MTAX_WFDMA0_EXT_CTRL_MAX_CNT))

#define MTAX_MCU_WPDMA0_BASE	0x54000000
#define MTAX_MCU_WPDMA0(ofs)	(MTAX_MCU_WPDMA0_BASE + (ofs))
#define MTAX_WFDMA_DUMMY_CR	MTAX_MCU_WPDMA0(0x120)
#define MTAX_WFDMA_NEED_REINIT	__BIT(1)

#define MTAX_DMA_SHDL_BASE		0x7c026000
#define MTAX_DMA_SHDL(ofs)		(MTAX_DMA_SHDL_BASE + (ofs))
#define MTAX_DMASHDL_SW_CONTROL		MTAX_DMA_SHDL(0x004)
#define MTAX_DMASHDL_DMASHDL_BYPASS	__BIT(28)

#define MTAX_RING_REG_SIZE	0x10
#define MTAX_RING_DESC_BASE_OFS	0x0
#define MTAX_RING_SIZE_OFS	0x4
#define MTAX_RING_CPU_IDX_OFS	0x8
#define MTAX_RING_DMA_IDX_OFS	0xc

#define MTAX_TX_RING_BASE	MTAX_WFDMA0(0x300)
#define MTAX_TX_RING_FWDL_BASE	(MTAX_TX_RING_BASE + 16 * MTAX_RING_REG_SIZE)
#define MTAX_TX_RING_MCU_BASE	(MTAX_TX_RING_BASE + 17 * MTAX_RING_REG_SIZE)

#define MTAX_RX_RING_BASE		MTAX_WFDMA0(0x500)
#define MTAX_RX_RING_MCU_BOOT_BASE	(MTAX_RX_RING_BASE + 0 * MTAX_RING_REG_SIZE)
#define MTAX_RX_RING_MCU_BASE		(MTAX_RX_RING_BASE + 4 * MTAX_RING_REG_SIZE)

#define MTAX_DMA_CTL_SDP0_H	__BITS(3, 0)
#define MTAX_DMA_CTL_SD_LEN0	__BITS(29, 16)
#define MTAX_DMA_CTL_LAST_SEC0	__BIT(30)
#define MTAX_DMA_CTL_DMA_DONE	__BIT(31)

#define MTAX_TXD0_TX_BYTES	__BITS(15, 0)
#define MTAX_TXD0_PKT_FMT	__BITS(24, 23)
#define MTAX_TXD0_Q_IDX		__BITS(31, 25)
#define MTAX_TXD1_HDR_FORMAT	__BITS(17, 16)
#define MTAX_TXD1_LONG_FORMAT	__BIT(31)

#define MTAX_HDR_FORMAT_CMD	1
#define MTAX_TX_TYPE_CMD		2
#define MTAX_TX_MCU_PORT_RX_Q0	0x20
#define MTAX_TX_PORT_IDX_MCU	1

#define MTAX_MCU_PQ_ID(port, queue)	(((port) << 15) | ((queue) << 10))
#define MTAX_MCU_PKT_ID			0xa0
#define MTAX_MCU_Q_NA			3
#define MTAX_MCU_S2D_H2N			0

#define MTAX_MCU_CMD_NIC_POWER_CTRL	0x04

#define MTAX_TX_MCU_RING_SIZE		256
#define MTAX_TX_FWDL_RING_SIZE		128
#define MTAX_RX_MCU_BOOT_RING_SIZE	8
#define MTAX_RX_MCU_RING_SIZE		512
#define MTAX_RX_BUF_SIZE			2048

struct mtax_mcu_txd {
	uint32_t txd[8];

	uint16_t len;
	uint16_t pq_id;

	uint8_t cid;
	uint8_t pkt_type;
	uint8_t set_query;
	uint8_t seq;

	uint8_t uc_d2b0_rev;
	uint8_t ext_cid;
	uint8_t s2d_index;
	uint8_t ext_cid_ack;

	uint32_t rsv[5];
} __packed __aligned(4);

struct mtax_mcu_rxd {
	uint32_t rxd[6];

	uint16_t len;
	uint16_t pkt_type_id;

	uint8_t eid;
	uint8_t seq;
	uint8_t option;
	uint8_t rsv;

	uint8_t ext_eid;
	uint8_t rsv1[2];
	uint8_t s2d_index;

	uint8_t tlv[];
} __packed __aligned(4);
