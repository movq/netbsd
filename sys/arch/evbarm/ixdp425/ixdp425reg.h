/*	$NetBSD: ixdp425reg.h,v 1.2 2003/05/24 01:59:32 ichiro Exp $ */
/*
 * Copyright (c) 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
 * All rights reserved.
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	_IXDP425REG_H_
#define	_IXDP425REG_H_

/*
 * GPIO  
 */

#if BOARDTYPE == ixdp425
#define	PCI_CLK		14
#define	PCI_RESET	13
#define	PCI_INT_D	IXP425_INT_GPIO_11
#define	PCI_INT_C	IXP425_INT_GPIO_10
#define	PCI_INT_B	IXP425_INT_GPIO_9
#define	PCI_INT_A	IXP425_INT_GPIO_8
#define	I2C_SDA		IXP425_INT_GPIO_7
#define	I2C_SCL		IXP425_INT_GPIO_6
#endif /* BOARDTYPE == ixdp425 */

#if BOARDTYPE == zao425		/* conf/ZAO425 */
#define	PCI_CLK		14
#define	PCI_RESET	13
#define	MPCI_GPIO0	IXP425_INT_GPIO_12
#define	PCI_INT_D	IXP425_INT_GPIO_11
#define	PCI_INT_C	IXP425_INT_GPIO_10
#define	PCI_INT_B	IXP425_INT_GPIO_9
#define	PCI_INT_A	IXP425_INT_GPIO_8
#define	I2C_SDA		IXP425_INT_GPIO_7
#define	I2C_SCL		IXP425_INT_GPIO_6
#define	MPCI_GPIO3	IXP425_INT_GPIO_5
#define	HSS0_INT	IXP425_INT_GPIO_4
#define	HSS0_SCLK	IXP425_INT_GPIO_3
#define	HSS0_SDI	IXP425_INT_GPIO_2
#define	HSS0_SDO	IXP425_INT_GPIO_1
#define	HSS0_CS		IXP425_INT_GPIO_0
#endif /* BOARDTYPE == zao425 */

#endif	/* _IXDP425REG_H_ */
