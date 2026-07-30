LIB=    LLVMTableGenBasic

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/utils/TableGen/Basic

SRCS+=	ARMTargetDefEmitter.cpp \
	Attributes.cpp \
	CodeGenIntrinsics.cpp \
	DirectiveEmitter.cpp \
	IntrinsicEmitter.cpp \
	RISCVTargetDefEmitter.cpp \
	RuntimeLibcallsEmitter.cpp \
	RuntimeLibcalls.cpp \
	SDNodeProperties.cpp \
	TableGen.cpp \
	TargetFeaturesEmitter.cpp \
	TargetLibraryInfoEmitter.cpp \
	VTEmitter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
