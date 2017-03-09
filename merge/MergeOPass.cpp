
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include <llvm/IR/Module.h>


using namespace llvm;
namespace {
    struct MergeO : public ModulePass {
        std::map<Function*, int> FnMap;
        static char ID;

        int maxFnArgCount(Module& M);
        void normFnArgNames(Module& M);
        void printFnArgs(Module& M);

        MergeO() : ModulePass(ID) {}

        virtual bool runOnModule(Module &M) {
            normFnArgNames(M);
            printFnArgs(M);
        }
    };
}

char MergeO::ID = 0;

static RegisterPass<MergeO> X("mergeO", "Merges functions in module", false, false);

int MergeO::maxFnArgCount(Module &M) {
    int MaxArgCount = 0;
    for (Module::iterator FI = M.begin(), FE = M.end(); FI != FE; ++FI) {
        int ArgCount = FI->arg_size();
        if (MaxArgCount < ArgCount) {
            MaxArgCount = ArgCount;
        }
    }
    return MaxArgCount;
}

void MergeO::normFnArgNames(Module &M) {
    for (Module::iterator FI = M.begin(), FE = M.end(); FI != FE; ++FI) {
        char c = 'a';
        for (Function::arg_iterator AI = FI->arg_begin(), AE = FI->arg_end(); AI != AE; ++AI) {
            AI->setName(std::string(&c));
            c++;
        }
    }
}

void MergeO::printFnArgs(Module &M) {
    for (Module::iterator FI = M.begin(), FE = M.end(); FI != FE; ++FI) {
        errs() << FI->getName() << "()" << " has " << FI->arg_size() << " arguments: " << "\n";
        for (Function::arg_iterator AI = FI->arg_begin(), AE = FI->arg_end(); AI != AE; ++AI) {
            errs() << AI->getName() << " ";
        }
        errs() << "\n";
    }
}