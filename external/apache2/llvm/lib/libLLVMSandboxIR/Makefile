#	$NetBSD$

LIB=	LLVMSandboxIR

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/SandboxIR

SRCS+=	Argument.cpp \
	BasicBlock.cpp \
	Constant.cpp \
	Context.cpp \
	Function.cpp \
	Instruction.cpp \
	Module.cpp \
	Pass.cpp \
	PassManager.cpp \
	Region.cpp \
	Tracker.cpp \
	Type.cpp \
	User.cpp \
	Use.cpp \
	Value.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
