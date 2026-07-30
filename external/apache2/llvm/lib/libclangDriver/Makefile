#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:56 joerg Exp $

LIB=	clangDriver

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Driver

CPPFLAGS+=	-I${CLANG_SRCDIR}/lib/Driver

SRCS+=	Action.cpp \
	Compilation.cpp \
	CreateASTUnitFromArgs.cpp \
	CreateInvocationFromArgs.cpp \
	Distro.cpp \
	Driver.cpp \
	Job.cpp \
	Multilib.cpp \
	MultilibBuilder.cpp \
	OffloadBundler.cpp \
	Phases.cpp \
	SanitizerArgs.cpp \
	Tool.cpp \
	ToolChain.cpp \
	Types.cpp \
	XRayArgs.cpp

.PATH: ${CLANG_SRCDIR}/lib/Driver/ToolChains

SRCS+=	AIX.cpp \
	AMDGPU.cpp \
	AMDGPUOpenMP.cpp \
	AVR.cpp \
	BareMetal.cpp \
	CSKYToolChain.cpp \
	Clang.cpp \
	CommonArgs.cpp \
	CrossWindows.cpp \
	Cuda.cpp \
	Cygwin.cpp \
	Darwin.cpp \
	DragonFly.cpp \
	Flang.cpp \
	FreeBSD.cpp \
	Fuchsia.cpp \
	Gnu.cpp \
	Haiku.cpp \
	Hexagon.cpp \
	HIPAMD.cpp \
	HIPSPV.cpp \
	HIPUtility.cpp \
	HLSL.cpp \
	Hurd.cpp \
	InterfaceStubs.cpp \
	LFILinux.cpp \
	Linux.cpp \
	Managarm.cpp \
	MinGW.cpp \
	MipsLinux.cpp \
	MSP430.cpp \
	MSVC.cpp \
	NetBSD.cpp \
	OHOS.cpp \
	OpenBSD.cpp \
	PPCFreeBSD.cpp \
	PPCLinux.cpp \
	PS4CPU.cpp \
	SPIRV.cpp \
	SPIRVOpenMP.cpp \
	Solaris.cpp \
	SYCL.cpp \
	TCE.cpp \
	UEFI.cpp \
	VEToolchain.cpp \
	WebAssembly.cpp \
	XCore.cpp \
	ZOS.cpp

.PATH: ${CLANG_SRCDIR}/lib/Driver/ToolChains/Arch

SRCS+=	AArch64.cpp \
	ARM.cpp \
	CSKY.cpp \
	LoongArch.cpp \
	M68k.cpp \
	Mips.cpp \
	PPC.cpp \
	RISCV.cpp \
	Sparc.cpp \
	SystemZ.cpp \
	VE.cpp \
	X86.cpp




.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
