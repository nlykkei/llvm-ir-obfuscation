/*
    Author: Nicolas Lykke Iversen (au341503@gmail.com)

    Insert corrector slot into basic block with associated checker as predecessor dominating all uses:
    - The checker computes the XOR of the basic block and compares the computed value to zero:
    - If the comparison fails, the program prints an error message and enters an infinite loop.
    - The corrector slots are filled in pre link-time.

    TODO: Handle phi nodes
*/

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/CFG.h>

#define DEBUG_TYPE "CheckerT"

using namespace llvm;

static std::string defaultCheckBB = "entry";
static std::string defaultCheckId = "checker";
static int defaultCVal = 0x00;

static cl::opt<std::string> CheckBB("checkbb",
                                    cl::desc("Basic block that should be checked"),
                                    cl::value_desc("Basic block identifier"), cl::init(defaultCheckBB), cl::Optional);

static cl::opt<std::string> CheckId("checkid",
                            cl::desc("Identifer of inserted checker"),
                            cl::value_desc("Checker identifier"), cl::init(defaultCheckId), cl::Optional);


static cl::opt<int> CVal("cval",
                         cl::desc("Value to be filled into corrector slot"),
                         cl::value_desc("Corrector slot value"), cl::init(defaultCVal), cl::Optional);

namespace {
    struct CheckerT : public ModulePass {
        static char ID;

        CheckerT() : ModulePass(ID) {}

        BasicBlock *insertCheckerBefore(BasicBlock *BB);

        bool insertCorrectorSlot(BasicBlock *BB);

        virtual bool runOnModule(Module &M) {
            for (auto &F : M) {
                for (auto &BB : F) {
                    if (!BB.getName().compare(CheckBB)) {
                        DEBUG(errs() << "Found basic block: " << BB.getName() << "\n");

                        /*
                        if (&F.getEntryBlock() == &BB) {
                            DEBUG(errs() << "Basic block is entry point of function." << "\n");
                            return false;
                        }
                        */

                        /*
                        if (pred_empty(&BB)) {
                            DEBUG(errs() << "Basic block has no predecessors." << "\n");
                            return false;
                        }
                        */

                        // Insert corrector slot into basic block
                        insertCorrectorSlot(&BB);

                        // Insert checker before basic block that dominates all uses
                        BasicBlock *Checker = insertCheckerBefore(&BB);

                        DEBUG(errs() << "Inserted checker for basic block \'" << CheckBB << "\'." << "\n");
                        DEBUG(F.viewCFG());

                        return true;
                    }
                }
            }

            DEBUG(errs() << "Could not find basic block \'" << CheckBB << "\' in module." << "\n");
            return false;
        }
    };
}

char CheckerT::ID = 0;

bool CheckerT::insertCorrectorSlot(BasicBlock *BB) {
    std::vector<Type *> ArgsTy;
    FunctionType *VoidFunTy = FunctionType::get(Type::getVoidTy(BB->getContext()), ArgsTy, false);

    std::vector<Type *> ArgsTy2;
    ArgsTy2.push_back(Type::getInt32Ty(BB->getContext()));
    FunctionType *IntFunTy = FunctionType::get(Type::getVoidTy(BB->getContext()), ArgsTy2, false);

    DEBUG(errs() << "Inserting corrector slot into: " << BB->getName());

    IRBuilder<> Builder(&*BB->getFirstInsertionPt());
    Builder.CreateCall(InlineAsm::get(VoidFunTy, ".cstart:", "", true));
    Builder.CreateCall(InlineAsm::get(VoidFunTy, "jmp .end", "", true));
    Builder.CreateCall(InlineAsm::get(VoidFunTy, ".cslot:", "", true));

    std::vector<Value *> CArgs;
    CArgs.push_back(ConstantInt::get(Type::getInt32Ty(BB->getContext()), CVal));
    Builder.CreateCall(InlineAsm::get(IntFunTy, ".byte ${0:c}", "i", true), CArgs); // eliminate $

    Builder.CreateCall(InlineAsm::get(VoidFunTy, ".end:", "", true));

    Builder.SetInsertPoint(BB->getTerminator());
    Builder.CreateCall(InlineAsm::get(VoidFunTy, ".cend:", "", true));

    return true;
}

BasicBlock *CheckerT::insertCheckerBefore(BasicBlock *BB) {
    std::vector<Type *> ArgsTy;
    FunctionType *FunTy = FunctionType::get(Type::getVoidTy(BB->getContext()), ArgsTy, false);

    BasicBlock *Checker = BasicBlock::Create(BB->getContext(), CheckId, BB->getParent(), BB);

    IRBuilder<> Builder(Checker);
    Builder.CreateCall(InlineAsm::get(FunTy, "pushq %rax", "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, "pushq %rbx", "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, "pushq %rsi", "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, "xorq %rax, %rax", "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, "movq .cstart, %rsi", "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, "$$.cloop:", "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, "movzbq (%rsi), %rbx", "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, "xorq %rbx, %rax", "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, "inc %rsi", "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, "cmpq $$.cend, %rsi", "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, "jne .cloop", "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, "cmpq $$0, %rax", "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, "je .restore", "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, "xorq %rax, %rax", "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, "callq *%rax", "", true)); // trigger runtime error
    Builder.CreateCall(InlineAsm::get(FunTy, ".restore:", "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, "popq %rsi", "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, "popq %rbx", "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, "popq %rax", "", true));

    for (BasicBlock *Pred : predecessors(BB)) { // 'Checker' is not a user of 'entry'
        TerminatorInst *TI = Pred->getTerminator();
        for (int i = 0; i < TI->getNumSuccessors(); ++i)
            if (TI->getSuccessor(i) == BB) {
                TI->setSuccessor(i, Checker);
            }
    }

    Builder.CreateBr(BB);
    return Checker;
}

static RegisterPass<CheckerT> X("checkerT", "Inserts checkers before basic blocks for tamper proofing", false, false);
