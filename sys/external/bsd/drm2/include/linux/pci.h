/*	$NetBSD: pci.h,v 1.58 2024/05/19 17:36:08 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LINUX_PCI_H_
#define _LINUX_PCI_H_

#ifdef _KERNEL_OPT
#include "acpica.h"
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cdefs.h>
#include <sys/kmem.h>
#include <sys/systm.h>

#include <machine/limits.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/agpvar.h>
#include <dev/pci/ppbvar.h>

#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/kernel.h>

struct acpi_devnode;
struct pci_driver;
struct pci_dev;
struct pci_bus;
struct pci_saved_state;

struct pci_device_id {
	uint32_t	vendor;
	uint32_t	device;
	uint32_t	subvendor;
	uint32_t	subdevice;
	uint32_t	class;
	uint32_t	class_mask;
	unsigned long	driver_data;
};

#define	PCI_DEVICE(VENDOR, DEVICE)					      \
	.vendor = (VENDOR),						      \
	.device = (DEVICE),						      \
	.subvendor = PCI_ANY_ID,						      \
	.subdevice = PCI_ANY_ID

#define	PCI_ANY_ID		(~0)

#define	PCI_BASE_CLASS_DISPLAY	PCI_CLASS_DISPLAY

#define	PCI_CLASS_DISPLAY_VGA						\
	((PCI_CLASS_DISPLAY << 8) | PCI_SUBCLASS_DISPLAY_VGA)
CTASSERT(PCI_CLASS_DISPLAY_VGA == 0x0300);

#define	PCI_CLASS_DISPLAY_OTHER						\
	((PCI_CLASS_DISPLAY << 8) | PCI_SUBCLASS_DISPLAY_MISC)
CTASSERT(PCI_CLASS_DISPLAY_OTHER == 0x0380);

#define	PCI_CLASS_DISPLAY_3D						\
	((PCI_CLASS_DISPLAY << 8) | PCI_SUBCLASS_DISPLAY_3D)
CTASSERT(PCI_CLASS_DISPLAY_3D == 0x0302);

#define	PCI_CLASS_ACCELERATOR_PROCESSING	(PCI_CLASS_ACCEL << 8)

#define	PCI_CLASS_BRIDGE_ISA						\
	((PCI_CLASS_BRIDGE << 8) | PCI_SUBCLASS_BRIDGE_ISA)
CTASSERT(PCI_CLASS_BRIDGE_ISA == 0x0601);

/* XXX This is getting silly...  */
#define	PCI_VENDOR_ID_APPLE	PCI_VENDOR_APPLE
#define	PCI_VENDOR_ID_AMD	PCI_VENDOR_AMD
#define	PCI_VENDOR_ID_ASUSTEK	PCI_VENDOR_ASUSTEK
#define	PCI_VENDOR_ID_ATI	PCI_VENDOR_ATI
#define	PCI_VENDOR_ID_DELL	PCI_VENDOR_DELL
#define	PCI_VENDOR_ID_IBM	PCI_VENDOR_IBM
#define	PCI_VENDOR_ID_HP	PCI_VENDOR_HP
#define	PCI_VENDOR_ID_INTEL	PCI_VENDOR_INTEL
#define	PCI_VENDOR_ID_NVIDIA	PCI_VENDOR_NVIDIA
#define	PCI_VENDOR_ID_SI	PCI_VENDOR_SIS
#define	PCI_VENDOR_ID_SONY	PCI_VENDOR_SONY
#define	PCI_VENDOR_ID_VIA	PCI_VENDOR_VIATECH

#define	PCI_SUBVENDOR_ID_REDHAT_QUMRANET	0x1af4

#define	PCI_DEVICE_ID_ATI_RADEON_QY	PCI_PRODUCT_ATI_RADEON_RV100_QY

#define	PCI_SUBDEVICE_ID_QEMU		0x1100

#define	PCI_DEVFN(DEV, FN)						\
	(__SHIFTIN((DEV), __BITS(3, 7)) | __SHIFTIN((FN), __BITS(0, 2)))
#define	PCI_SLOT(DEVFN)		((int)__SHIFTOUT((DEVFN), __BITS(3, 7)))
#define	PCI_FUNC(DEVFN)		((int)__SHIFTOUT((DEVFN), __BITS(0, 2)))

#define	PCI_DEVID(BUS, DEVFN)						      \
	(__SHIFTIN((BUS), __BITS(15, 8)) | __SHIFTIN((DEVFN), __BITS(7, 0)))
#define	PCI_BUS_NUM(DEVID)	((int)__SHIFTOUT((DEVID), __BITS(15,8)))

#define	PCI_NUM_RESOURCES	((PCI_MAPREG_END - PCI_MAPREG_START) / 4)
#define	DEVICE_COUNT_RESOURCE	PCI_NUM_RESOURCES

#define	PCI_CAP_ID_AGP	PCI_CAP_AGP

#define PCI_EXP_LNKCTL			PCIE_LCSR
#define  PCI_EXP_LNKCTL_HAWD		PCIE_LCSR_HAWD
#define PCI_EXP_LNKSTA			(PCIE_LCSR + 2)
#define PCI_EXP_DEVSTA			(PCIE_DCSR + 2)
#define  PCI_EXP_DEVSTA_TRPND		(PCIE_DCSR_TRANSACTION_PND >> 16)
#define PCI_EXP_LNKCTL2			PCIE_LCAP2
#define  PCI_EXP_LNKCTL2_ENTER_COMP	PCIE_LCSR2_ENT_COMPL
#define  PCI_EXP_LNKCTL2_TX_MARGIN	PCIE_LCSR2_TX_MARGIN
#define  PCI_EXP_LNKCTL2_TLS		PCIE_LCSR2_TGT_LSPEED
#define  PCI_EXP_LNKCTL2_TLS_2_5GT	PCIE_LCSR2_TGT_LSPEED_2_5G
#define  PCI_EXP_LNKCTL2_TLS_5_0GT	PCIE_LCSR2_TGT_LSPEED_5G
#define  PCI_EXP_LNKCTL2_TLS_8_0GT	PCIE_LCSR2_TGT_LSPEED_8G
#define PCI_EXP_LNKCAP			PCIE_LCAP
#define  PCI_EXP_LNKCAP_CLKPM		PCIE_LCAP_CLOCK_PM
#define PCI_EXP_DEVCAP2_ATOMIC_COMP32	PCIE_DCAP2_32ATOM
#define PCI_EXP_DEVCAP2_ATOMIC_COMP64	PCIE_DCAP2_64ATOM
#define	PCI_EXP_TYPE_UPSTREAM		PCIE_XCAP_TYPE_UP
#define	PCI_EXP_TYPE_DOWNSTREAM		PCIE_XCAP_TYPE_DOWN

#define	PCI_COMMAND			PCI_COMMAND_STATUS_REG
#define	PCI_VENDOR_ID			PCI_ID_REG
#define	PCI_COMMAND_MEMORY		PCI_COMMAND_MEM_ENABLE
#define	PCI_PRIMARY_BUS			PCI_BRIDGE_BUS_REG
#define	PCI_POSSIBLE_ERROR(value)	((value) == UINT32_MAX)

#define	PCI_EXT_CAP_ID_ERR		PCI_EXTCAP_AER
#define	PCI_EXT_CAP_ID_VNDR		PCI_EXTCAP_VENDOR
#define	PCI_EXT_CAP_ID_LTR		PCI_EXTCAP_LTR

#define	PCI_ERR_UNCOR_STATUS		PCI_AER_UC_STATUS
#define	PCI_ERR_COR_STATUS		PCI_AER_COR_STATUS

typedef int pci_power_t;

#define	PCI_D0		0
#define	PCI_D1		1
#define	PCI_D2		2
#define	PCI_D3hot	3
#define	PCI_D3cold	4

typedef unsigned int pci_channel_state_t;

enum {
	pci_channel_io_normal = 1,
	pci_channel_io_frozen = 2,
	pci_channel_io_perm_failure = 3,
};

typedef unsigned int pci_ers_result_t;

enum pci_ers_result {
	PCI_ERS_RESULT_NONE = 1,
	PCI_ERS_RESULT_CAN_RECOVER = 2,
	PCI_ERS_RESULT_NEED_RESET = 3,
	PCI_ERS_RESULT_DISCONNECT = 4,
	PCI_ERS_RESULT_RECOVERED = 5,
	PCI_ERS_RESULT_NO_AER_DRIVER = 6,
};

struct pci_error_handlers {
	pci_ers_result_t (*error_detected)(struct pci_dev *,
	    pci_channel_state_t);
	pci_ers_result_t (*mmio_enabled)(struct pci_dev *);
	pci_ers_result_t (*slot_reset)(struct pci_dev *);
	void (*reset_prepare)(struct pci_dev *);
	void (*reset_done)(struct pci_dev *);
	void (*resume)(struct pci_dev *);
	void (*cor_error_detected)(struct pci_dev *);
};

#define	__pci_iomem

struct pci_dev {
	struct pci_attach_args	pd_pa;
	int			pd_kludges;	/* Gotta lose 'em...  */
#define	NBPCI_KLUDGE_GET_MUMBLE	0x01
#define	NBPCI_KLUDGE_MAP_ROM	0x02
	bus_space_tag_t		pd_rom_bst;
	bus_space_handle_t	pd_rom_bsh;
	bus_size_t		pd_rom_size;
	bus_space_handle_t	pd_rom_found_bsh;
	bus_size_t		pd_rom_found_size;
	void			*pd_rom_vaddr;
	device_t		pd_dev;
	void			*pd_drvdata;
	struct {
		pcireg_t		type;
		bus_addr_t		addr;
		bus_size_t		size;
		int			flags;
		bus_space_tag_t		bst;
		bus_space_handle_t	bsh;
		void __pci_iomem	*kva;
		bool			mapped;
	}			pd_resources[PCI_NUM_RESOURCES];
	struct pci_conf_state	*pd_saved_state;
	struct acpi_devnode	*pd_ad;
	pci_intr_handle_t	*pd_intr_handles;
	unsigned		pd_enablecnt;

	/* Linx API only below */
	struct pci_bus		*bus;
	uint32_t		devfn;
	uint16_t		vendor;
	uint16_t		device;
	uint16_t		subsystem_vendor;
	uint16_t		subsystem_device;
	uint8_t			revision;
	uint32_t		class;
	bool			msi_enabled;
	bool			no_64bit_msi;
	uint8_t			msix_cap;
};

#define	PCI_MSIX_FLAGS		(PCI_MSIX_CTL + 2)
#define	PCI_MSIX_FLAGS_ENABLE	(PCI_MSIX_CTL_ENABLE >> 16)

enum pci_bus_speed {
	PCI_SPEED_UNKNOWN,
	PCIE_SPEED_2_5GT,
	PCIE_SPEED_5_0GT,
	PCIE_SPEED_8_0GT,
	PCIE_SPEED_16_0GT,
	PCIE_SPEED_32_0GT,
	PCIE_SPEED_64_0GT,
};

/*
 * Actually values from the Link Status register, bits 16-19.  Don't use
 * these as a bit-mask -- these are the only known, valid values.
 */
enum pcie_link_width {
	PCIE_LNK_WIDTH_RESRV   = 0,
	PCIE_LNK_X1            = __BIT(0),
	PCIE_LNK_X2            = __BIT(1),
	PCIE_LNK_X4            = __BIT(2),
	PCIE_LNK_X8            = __BIT(3),
	PCIE_LNK_X12           = __BITS(2,3),
	PCIE_LNK_X16           = __BIT(4),
	PCIE_LNK_X32           = __BIT(5),
	PCIE_LNK_WIDTH_UNKNOWN = __BITS(0, 7),
};

#define	PCIBIOS_MIN_MEM	0x100000	/* XXX bogus x86 kludge bollocks */

#define	__pci_rom_iomem

struct pci_bus {
	/* NetBSD private members */
	pci_chipset_tag_t	pb_pc;
	device_t		pb_dev;

	/* Linux API */
	u_int			number;
	enum pci_bus_speed	max_bus_speed;

	struct pci_dev		*self;
};

static inline uint16_t
pci_dev_id(struct pci_dev *pdev)
{

	return PCI_DEVID(pdev->bus->number, pdev->devfn);
}

static inline struct pci_dev *
pci_upstream_bridge(struct pci_dev *pdev)
{

	if (pdev == NULL || pdev->bus == NULL)
		return NULL;

	return pdev->bus->self;
}

/* Namespace.  */
#define	pci_bus_alloc_resource		linux_pci_bus_alloc_resource
#define	pci_bus_read_config_byte	linux_pci_bus_read_config_byte
#define	pci_bus_read_config_dword	linux_pci_bus_read_config_dword
#define	pci_bus_read_config_word	linux_pci_bus_read_config_word
#define	pci_bus_write_config_byte	linux_pci_bus_write_config_byte
#define	pci_bus_write_config_dword	linux_pci_bus_write_config_dword
#define	pci_bus_write_config_word	linux_pci_bus_write_config_word
#define	pci_clear_master		linux_pci_clear_master
#define	pci_dev_dev			linux_pci_dev_dev
#define	pci_dev_present			linux_pci_dev_present
#define	pci_dev_put			linux_pci_dev_put
#define	pci_disable_msi			linux_pci_disable_msi
#define	pci_disable_rom			linux_pci_disable_rom
#define	pci_dma_supported		linux_pci_dma_supported
#define	pci_domain_nr			linux_pci_domain_nr
#define	pci_enable_msi			linux_pci_enable_msi
#define	pci_enable_rom			linux_pci_enable_rom
#define	pci_find_capability		linux_pci_find_capability
#define	pci_find_ext_capability		linux_pci_find_ext_capability
#define	pci_get_base_class		linux_pci_get_base_class
#define	pci_get_class			linux_pci_get_class
#define	pci_get_domain_bus_and_slot	linux_pci_get_domain_bus_and_slot
#define	pci_get_drvdata			linux_pci_get_drvdata
#define	pci_iomap			linux_pci_iomap
#define	pci_iounmap			linux_pci_iounmap
#define	pci_is_pcie			linux_pci_is_pcie
#define	pci_is_root_bus			linux_pci_is_root_bus
#define	pci_is_thunderbolt_attached	linux_pci_is_thunderbolt_attached
#define	pci_map_rom			linux_pci_map_rom
#define	pci_name			linux_pci_name
#define	pci_platform_rom		linux_pci_platform_rom
#define	pci_read_config_byte		linux_pci_read_config_byte
#define	pci_read_config_dword		linux_pci_read_config_dword
#define	pci_read_config_word		linux_pci_read_config_word
#define	pci_release_region		linux_pci_release_region
#define	pci_release_regions		linux_pci_release_regions
#define	pci_request_region		linux_pci_request_region
#define	pci_request_regions		linux_pci_request_regions
#define	pci_resource_end		linux_pci_resource_end
#define	pci_resource_flags		linux_pci_resource_flags
#define	pci_resource_len		linux_pci_resource_len
#define	pci_resource_start		linux_pci_resource_start
#define	pci_restore_state		linux_pci_restore_state
#define	pci_save_state			linux_pci_save_state
#define	pci_set_drvdata			linux_pci_set_drvdata
#define	pci_set_master			linux_pci_set_master
#define	pci_unmap_rom			linux_pci_unmap_rom
#define	pci_wait_for_pending_transaction	\
					linux_pci_wait_for_pending_transaction
#define	pci_write_config_byte		linux_pci_write_config_byte
#define	pci_write_config_dword		linux_pci_write_config_dword
#define	pci_write_config_word		linux_pci_write_config_word
#define	pcibios_align_resource		linux_pcibios_align_resource
#define	pcie_get_speed_cap		linux_pcie_get_speed_cap
#define	pcie_get_width_cap		linux_pcie_get_width_cap
#define	pcie_bandwidth_available	linux_pcie_bandwidth_available
#define	pcie_aspm_enabled		linux_pcie_aspm_enabled
#define	pcie_read_config_dword		linux_pcie_capability_read_dword
#define	pcie_read_config_word		linux_pcie_capability_read_word
#define	pcie_write_config_dword		linux_pcie_capability_write_dword
#define	pcie_write_config_word		linux_pcie_capability_write_word
#define	pci_enable_atomic_ops_to_root	linux_pci_enable_atomic_ops_to_root

/* NetBSD local additions.  */
void		linux_pci_dev_init(struct pci_dev *, device_t, device_t,
		    const struct pci_attach_args *, int);
void		linux_pci_dev_destroy(struct pci_dev *);

/* NetBSD no-renames because use requires review.  */
int		linux_pci_enable_device(struct pci_dev *);
void		linux_pci_disable_device(struct pci_dev *);

bool		pci_is_root_bus(struct pci_bus *);
int		pci_domain_nr(struct pci_bus *);

device_t	pci_dev_dev(struct pci_dev *);
void		pci_set_drvdata(struct pci_dev *, void *);
void *		pci_get_drvdata(struct pci_dev *);
const char *	pci_name(struct pci_dev *);

int		pci_find_capability(struct pci_dev *, int);
int		pci_find_ext_capability(struct pci_dev *, int);
bool		pci_is_pcie(struct pci_dev *);
bool		pci_dma_supported(struct pci_dev *, uintmax_t);
bool		pci_is_thunderbolt_attached(struct pci_dev *);

int		pci_read_config_dword(struct pci_dev *, int, uint32_t *);
int		pci_read_config_word(struct pci_dev *, int, uint16_t *);
int		pci_read_config_byte(struct pci_dev *, int, uint8_t *);
int		pci_write_config_dword(struct pci_dev *, int, uint32_t);
int		pci_write_config_word(struct pci_dev *, int, uint16_t);
int		pci_write_config_byte(struct pci_dev *, int, uint8_t);

int		pcie_capability_read_dword(struct pci_dev *, int, uint32_t *);
int		pcie_capability_read_word(struct pci_dev *, int, uint16_t *);
int		pcie_capability_write_dword(struct pci_dev *, int, uint32_t);
int		pcie_capability_write_word(struct pci_dev *, int, uint16_t);
bool		pcie_aspm_enabled(struct pci_dev *);

static inline int
pci_pcie_type(struct pci_dev *pdev)
{
	uint32_t xcap;

	if (pcie_capability_read_dword(pdev, PCIE_XCAP, &xcap) != 0)
		return PCIE_XCAP_TYPE_PCIE_DEV;

	return PCIE_XCAP_TYPE(xcap);
}

static inline struct pci_dev *
pcie_find_root_port(struct pci_dev *pdev)
{
	uint32_t xcap;

	while (pdev != NULL) {
		if (pcie_capability_read_dword(pdev, PCIE_XCAP, &xcap) == 0 &&
		    PCIE_XCAP_TYPE(xcap) == PCIE_XCAP_TYPE_RP)
			return pdev;
		pdev = pci_upstream_bridge(pdev);
	}

	return NULL;
}

static inline bool
pci_pr3_present(struct pci_dev *pdev __unused)
{

	/* XXX Teach the NetBSD PCI/ACPI glue how to query _PR3. */
	return false;
}

static inline int
pcie_capability_clear_and_set_word(struct pci_dev *pdev, int reg,
    uint16_t clear, uint16_t set)
{
	uint16_t value;
	int error;

	error = pcie_capability_read_word(pdev, reg, &value);
	if (error)
		return error;
	value &= ~clear;
	value |= set;
	return pcie_capability_write_word(pdev, reg, value);
}

static inline int
pcie_capability_clear_word(struct pci_dev *pdev, int reg, uint16_t clear)
{

	return pcie_capability_clear_and_set_word(pdev, reg, clear, 0);
}

static inline int
pcie_capability_set_word(struct pci_dev *pdev, int reg, uint16_t set)
{

	return pcie_capability_clear_and_set_word(pdev, reg, 0, set);
}

int		pci_bus_read_config_dword(struct pci_bus *, unsigned, int,
		    uint32_t *);
int		pci_bus_read_config_word(struct pci_bus *, unsigned, int,
		    uint16_t *);
int		pci_bus_read_config_byte(struct pci_bus *, unsigned, int,
		    uint8_t *);
int		pci_bus_write_config_dword(struct pci_bus *, unsigned, int,
		    uint32_t);
int		pci_bus_write_config_word(struct pci_bus *, unsigned, int,
		    uint16_t);
int		pci_bus_write_config_byte(struct pci_bus *, unsigned, int,
		    uint8_t);

int		pci_enable_msi(struct pci_dev *);
void		pci_disable_msi(struct pci_dev *);
void		pci_set_master(struct pci_dev *);
void		pci_clear_master(struct pci_dev *);

int		pcie_get_readrq(struct pci_dev *);
int		pcie_set_readrq(struct pci_dev *, int);

bus_addr_t	pcibios_align_resource(void *, const struct resource *,
		    bus_addr_t, bus_size_t);
int		pci_bus_alloc_resource(struct pci_bus *, struct resource *,
		    bus_size_t, bus_size_t, bus_addr_t, int,
		    bus_addr_t (*)(void *, const struct resource *, bus_addr_t,
			bus_size_t), struct pci_dev *);

/* XXX Kludges only -- do not use without checking the implementation!  */
struct pci_dev *pci_get_domain_bus_and_slot(int, int, int);
struct pci_dev *pci_get_base_class(uint32_t, struct pci_dev *);
struct pci_dev *pci_get_class(uint32_t, struct pci_dev *); /* i915 kludge */
int		pci_dev_present(const struct pci_device_id *);
void		pci_dev_put(struct pci_dev *);

void __pci_rom_iomem *
		pci_map_rom(struct pci_dev *, size_t *);
void __pci_rom_iomem *
		pci_platform_rom(struct pci_dev *, size_t *);
void		pci_unmap_rom(struct pci_dev *, void __pci_rom_iomem *);
int		pci_enable_rom(struct pci_dev *);
void		pci_disable_rom(struct pci_dev *);

int		pci_request_regions(struct pci_dev *, const char *);
void		pci_release_regions(struct pci_dev *);
int		pci_request_region(struct pci_dev *, int, const char *);
void		pci_release_region(struct pci_dev *, int);

bus_addr_t	pci_resource_start(struct pci_dev *, unsigned);
bus_size_t	pci_resource_len(struct pci_dev *, unsigned);
bus_addr_t	pci_resource_end(struct pci_dev *, unsigned);
int		pci_resource_flags(struct pci_dev *, unsigned);

static inline int
pci_rebar_bytes_to_size(uint64_t bytes)
{

	if (bytes <= (UINT64_C(1) << 20))
		return 0;

	/* Return the size encoding from the PCI Resizable BAR capability. */
	return fls64(bytes - 1) - 20;
}

void __pci_iomem *
		pci_iomap(struct pci_dev *, unsigned, bus_size_t);
void		pci_iounmap(struct pci_dev *, void __pci_iomem *);

int		pci_save_state(struct pci_dev *);
void		pci_restore_state(struct pci_dev *);
struct pci_saved_state *
		pci_store_saved_state(struct pci_dev *);
int		pci_load_saved_state(struct pci_dev *,
		    struct pci_saved_state *);
void		linux_pci_restore_state_force(struct pci_dev *,
		    struct pci_saved_state *);
int		pci_wait_for_pending_transaction(struct pci_dev *);

enum pci_bus_speed pcie_get_speed_cap(struct pci_dev *dev);
enum pcie_link_width pcie_get_width_cap(struct pci_dev *dev);
unsigned	pcie_bandwidth_available(struct pci_dev *dev,
					 struct pci_dev **limiting_dev,
					 enum pci_bus_speed *speed,
					 enum pcie_link_width *width);

static inline bool
dev_is_pci(struct device *dev)
{
	struct device *parent = device_parent(dev);

	return parent && device_is_a(parent, "pci");
}

static inline int
pci_enable_atomic_ops_to_root(struct pci_dev *dev, uint32_t cap_mask)
{

	return -EINVAL;
}

static inline bool
pci_dev_is_disconnected(const struct pci_dev *pdev)
{

	return false;
}

#endif  /* _LINUX_PCI_H_ */
