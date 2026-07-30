/*	$NetBSD$	*/

#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/LLVMDriver.h"

#ifndef LLVM_TOOL_MAIN
#error LLVM_TOOL_MAIN must name the tool's entry point
#endif

int LLVM_TOOL_MAIN(int, char **, const llvm::ToolContext &);

int
main(int argc, char **argv)
{
	llvm::InitLLVM X(argc, argv);
	return LLVM_TOOL_MAIN(argc, argv, {argv[0], nullptr, false});
}
