/*	$NetBSD$	*/

#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/LLVMDriver.h"

int clang_main(int, char **, const llvm::ToolContext &);

int
main(int argc, char **argv)
{
	llvm::InitLLVM X(argc, argv);
	return clang_main(argc, argv, {argv[0], nullptr, false});
}
