
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/CFG.h>
#include <algorithm>
#include <llvm/Support/CommandLine.h>

#define DEBUG_TYPE "CheckerT"
#define RED_ZONE 128

using namespace llvm;

static cl::list<int> Splits("splits", cl::Positional, cl::CommaSeparated,
                            cl::desc("CRT watermark splits"));

namespace {
    struct ChineseWM : public ModulePass {
        static char ID;

        ChineseWM() : ModulePass(ID) {
            srand(time(NULL));
        }

        void insertSplits(Module &M);

        virtual bool runOnModule(Module &M) {

            insertSplits(M);

            return true;
        }
    };
}

char ChineseWM::ID = 0;

void ChineseWM::insertSplits(Module &M) {

    int WM = 0;

    std::vector<Type *> ArgsTy;
    FunctionType *VoidFunTy = FunctionType::get(Type::getVoidTy(M.getContext()), ArgsTy, false);

    std::vector<Type *> ArgsTy2;
    ArgsTy2.push_back(Type::getInt32Ty(M.getContext()));
    FunctionType *IntFunTy = FunctionType::get(Type::getVoidTy(M.getContext()), ArgsTy2, false);

    for (int Split : Splits) {

        int IdxF = 0;
        Module::iterator FI;

        do {
            IdxF = rand() % M.getFunctionList().size();
            FI = M.begin();
            std::advance(FI, IdxF);
        } while (FI->isDeclaration());

        errs() << FI->getName() << "\n";

        int IdxBB = rand() % FI->getBasicBlockList().size();
        Function::iterator BI = FI->begin();
        std::advance(BI, IdxBB);

        Instruction *I = &*BI->getFirstInsertionPt();

        IRBuilder<> Builder(I);

        Builder.CreateCall(InlineAsm::get(VoidFunTy, std::string("jmp .end_") + std::to_string(WM), "", true));
        Builder.CreateCall(
                InlineAsm::get(VoidFunTy, std::string(".wm_split") + std::to_string(WM) + std::string(":"), "", true));

        std::vector<Value *> CArgs;
        CArgs.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), Split));
        Builder.CreateCall(InlineAsm::get(IntFunTy, std::string(".4byte ${0:c}"), "i", true), CArgs); // eliminate $

        Builder.CreateCall(
                InlineAsm::get(VoidFunTy, std::string(".end_") + std::to_string(WM) + std::string(":"), "", true));

        ++WM;
    }
}


static RegisterPass<ChineseWM> X("splitWM", "Inserts a CRT-split watermark into module", false, false);
