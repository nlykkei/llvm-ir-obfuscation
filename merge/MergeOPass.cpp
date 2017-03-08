
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include <map>

using namespace llvm;
namespace {
    struct MergeO : public ModulePass {
        std::map<Function*, int> FnMap;
        static char ID;

        MergeO() : ModulePass(ID) {}

        virtual bool runOnModule(Module &M) {
            
        }
    };
}

char MergeO::ID = 0;

static RegisterPass<MergeO> X("mergeO", "Merges functions in module", false, false);
