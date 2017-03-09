
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <algorithm>

#define DEBUG 0

using namespace llvm;
namespace {
    struct CyclomaticA : public FunctionPass {
        static char ID;

        CyclomaticA() : FunctionPass(ID) {}

        virtual bool runOnFunction(Function &F) {
            int n = 0; // #basic blocks
            int e = 0; // #edges
            for (auto BI = F.begin(), BE = F.end(); BI != BE; ++BI) {
                TerminatorInst* TermInst = BI->getTerminator();
                auto Succ = TermInst->successors();
                e += std::distance(Succ.begin(), Succ.end());
                ++n;
#if DEBUG
                errs() << BI->getName() << " has successors: " << "\n";
                for (auto SI = Succ.begin(), SE = Succ.end(); SI != SE; ++SI) {
                    errs() << SI->getName() << "\n";
                }
#endif
            }
            errs() << F.getName() << " has cyclomatic number: " << n + e + 2 << "\n";
        }
    };
}

char CyclomaticA::ID = 0;

static RegisterPass<CyclomaticA> X("cyclomaticA", "Computes the cyclomatic number of each function in a module", false,
                                   false);

