/*
    Author: Nicolas Lykke Iversen (au341503@gmail.com)

    1) Insert corrector slot into basic block (chosen by the user) with associated dominating checker as predecessor.
    2) The inserted checker is also checked by another checker inserted randomly within the CFG.
    3) Each checker computes the XOR of the basic block and compares it to zero (stealth transformation)
    4) If the comparison fails, the program crashes.
    5) The values of the corrector slots are determined at post link-time.
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
#include <algorithm>

#define DEBUG_TYPE "CheckerT"
#define RED_ZONE 128

using namespace llvm;

static std::string defaultCheckFn = "";
static std::string defaultCheckBB = "";
static std::string defaultCheckPID = "c";


static cl::opt<std::string> CheckFn("checkfn",
                                    cl::desc("Function containing basic block to check"),
                                    cl::value_desc("Function identifier"), cl::init(defaultCheckFn), cl::Optional);

static cl::opt<std::string> CheckBB("checkbb",
                                    cl::desc("Basic block that should be checked"),
                                    cl::value_desc("Basic block identifier"), cl::Required);

static cl::opt<std::string> CheckPID("checkpid",
                                    cl::desc("Identifer prefix of inserted checker"),
                                    cl::value_desc("Checker identifier prefix"), cl::init(defaultCheckPID), cl::Optional);

namespace {
    struct CheckerT : public ModulePass {
        static char ID;

        CheckerT() : ModulePass(ID) {
            srand(time(NULL));
        }

        BasicBlock *insertCheckerBefore(BasicBlock *BB, std::string &Id);

        bool insertCorrectorSlot(BasicBlock *BB, std::string &Id);

        virtual bool runOnModule(Module &M) {

            DEBUG(errs() << std::string(0, ' ') << "Searching for basic block \'" << CheckBB << "\' in "
                         << (CheckFn.empty() ? "any function" : (std::string("function ") + CheckFn))  << " (" << M.getName() << ")" "\n");

            DEBUG(errs() << std::string(2, ' ') << "Searching functions in module \'" << M.getName() << "\'" << "\n");

            bool foundFunction = false;

            for (auto &F : M) {

                DEBUG(errs() << std::string(4, ' ') << "Checking function \'" << F.getName() << "\'" << "\n");

                if (CheckFn.empty() || CheckFn == F.getName()) {

                    if (!CheckFn.empty()) {
                        DEBUG(errs() << std::string(4, ' ') << "Found function \'" << CheckFn << "\'" << "\n");
                    } else {
                        DEBUG(errs() << std::string(4, ' ') << "Function \'" << F.getName() << "\' match criteria <any>" << "\n");
                    }

                    foundFunction = true;

                    DEBUG(errs() << std::string(4, ' ') << "Searching basic blocks in function \'" << F.getName()
                                 << "\'" << "\n");

                    for (auto &BB : F) {

                        DEBUG(errs() << std::string(6, ' ') << "Checking basic block \'" << BB.getName() << "\'"
                                     << "\n");

                        if (!BB.getName().compare(CheckBB)) {

                            DEBUG(errs() << std::string(6, ' ') << "Found basic block \'" << BB.getName() << "\'"
                                         << "\n");

                            if (&F.getEntryBlock() == &BB) {
                                DEBUG(errs() << std::string(8, ' ') << "Basic block \'" << CheckBB << "\' is entry point of function."
                                             << "\n");
                                DEBUG(errs() << std::string(0 , ' ') << "Failed to insert checker for basic block \'" << CheckBB << "\'" << "\n");
                                return false;
                            }

                            if (pred_empty(&BB)) {
                                DEBUG(errs() << std::string(8, ' ') << "Basic block \'" << CheckBB << "\' has no predecessors." << "\n");
                                DEBUG(errs() << std::string(0 , ' ') << "Failed to insert checker for basic block \'" << CheckBB << "\'" << "\n");
                                return false;
                            }

                            std::string Id0 = CheckPID + std::to_string(0);

                            // Insert corrector slot into basic block
                            DEBUG(errs() << std::string(8, ' ') << "Inserting corrector slot into basic block \'"
                                         << BB.getName() << "\'"
                                         << "\n");
                            insertCorrectorSlot(&BB, Id0);

                            // Insert checker before basic block that dominates all uses
                            DEBUG(errs() << std::string(8, ' ') << "Inserting dominating checker \'" << Id0
                                         << "\' for basic block \'"
                                         << BB.getName() << "\'" << "\n");
                            BasicBlock *Checker = insertCheckerBefore(&BB, Id0);

                            std::string Id1 = CheckPID + std::to_string(1);

                            // Insert corrector slot into checker
                            DEBUG(errs() << std::string(8, ' ') << "Inserting corrector slot into checker \'" << Id0
                                         << "\'" << "\n");
                            insertCorrectorSlot(Checker, Id1);

                            // Insert checker at random position into CFG to check inserted checker
                            int numBasicBlocks = F.getBasicBlockList().size();
                            int randPos = rand() % numBasicBlocks;
                            randPos = randPos == 0 ? randPos + 1 : randPos; // Prevent inserting checker before 'entry'
                            Function::iterator It = F.begin();
                            std::advance(It, randPos);
                            BasicBlock *InsertBB = &*It;
                            DEBUG(errs() << std::string(8, ' ') << "Inserted checker \'" << Id1 << "\' for checker \'"
                                         << Id0
                                         << "\' before basic block \'" << InsertBB->getName() << "\'" << "\n");
                            insertCheckerBefore(InsertBB, Id1);

                            DEBUG(errs() << std::string(0 , ' ') << "Succeeded to insert checker for basic block \'" << CheckBB << "\'" << "\n");

                            DEBUG(F.viewCFG());

                            return true;
                        }
                    }

                    DEBUG(errs() << std::string(4, ' ') << "Could not find basic block \'" << CheckBB << "\' in function \'"
                                 << F.getName() << "\'" << "\n");
                }
            }

            if (!foundFunction && !CheckFn.empty()) {
                DEBUG(errs() << std::string(2, ' ') << "Could not find function \'" << CheckFn << "\'"
                             << " in module \'" << M.getName() << "\'" << "\n");
            }

            DEBUG(errs() << std::string(0 , ' ') << "Failed to insert checker for basic block \'" << CheckBB << "\'" << "\n");

            return false;
        }
    };
}

char CheckerT::ID = 0;

bool CheckerT::insertCorrectorSlot(BasicBlock *BB, std::string &Id) {
    std::vector<Type *> ArgsTy;
    FunctionType *VoidFunTy = FunctionType::get(Type::getVoidTy(BB->getContext()), ArgsTy, false);

    std::vector<Type *> ArgsTy2;
    ArgsTy2.push_back(Type::getInt32Ty(BB->getContext()));
    FunctionType *IntFunTy = FunctionType::get(Type::getVoidTy(BB->getContext()), ArgsTy2, false);

    IRBuilder<> Builder(&*BB->getFirstInsertionPt());
    Builder.CreateCall(InlineAsm::get(VoidFunTy, std::string("nop"), "", true));
    Builder.CreateCall(InlineAsm::get(VoidFunTy, std::string(".cstart_") + Id + std::string(":"), "", true));
    Builder.CreateCall(InlineAsm::get(VoidFunTy, std::string("jmp .end_") + Id, "", true));
    Builder.CreateCall(InlineAsm::get(VoidFunTy, std::string(".cslot_") + Id + std::string(":"), "", true));

    std::vector<Value *> CArgs;
    CArgs.push_back(ConstantInt::get(Type::getInt32Ty(BB->getContext()), 0x00));
    Builder.CreateCall(InlineAsm::get(IntFunTy, std::string(".byte ${0:c}"), "i", true), CArgs); // eliminate $

    Builder.CreateCall(InlineAsm::get(VoidFunTy, std::string(".end_") + Id + std::string(":"), "", true));

    Builder.SetInsertPoint(BB->getTerminator());
    Builder.CreateCall(InlineAsm::get(VoidFunTy, std::string(".cend_") + Id + std::string(":"), "", true));

    return true;
}

BasicBlock *CheckerT::insertCheckerBefore(BasicBlock *BB, std::string &Id) {
    std::vector<Type *> ArgsTy;
    FunctionType *FunTy = FunctionType::get(Type::getVoidTy(BB->getContext()), ArgsTy, false);

    // Split basic block at first non PHI node
    Instruction *SplitInst = BB->getFirstNonPHI();
    BasicBlock *SplitBB = BB->splitBasicBlock(SplitInst, BB->getName()); // Contains all instructions after PHI nodes

    // Restore names
    std::string Name = BB->getName();
    BB->setName(Id);
    SplitBB->setName(Name);

    // Make 'BB' a checker of 'SplitBB' (unique predecessor)
    IRBuilder<> Builder(&BB->back());
    Builder.CreateCall(
            InlineAsm::get(FunTy, std::string("subq $$") + std::to_string(RED_ZONE) + std::string(", %rsp"), "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string("pushq %rax"), "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string("pushq %rbx"), "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string("pushq %rsi"), "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string("xorq %rax, %rax"), "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string("movq $$.cstart_") + Id + std::string(", %rsi"), "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string(".cloop_") + Id + std::string(":"), "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string("movzbq (%rsi), %rbx"), "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string("xorq %rbx, %rax"), "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string("inc %rsi"), "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string("cmpq $$.cend_") + Id + std::string(", %rsi"), "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string("jne .cloop_") + Id, "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string("cmpq $$0, %rax"), "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string("je .restore_") + Id, "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string("xorq %rax, %rax"), "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string("callq *%rax"), "", true)); // trigger runtime error
    Builder.CreateCall(InlineAsm::get(FunTy, std::string(".restore_") + Id + std::string(":"), "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string("popq %rsi"), "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string("popq %rbx"), "", true));
    Builder.CreateCall(InlineAsm::get(FunTy, std::string("popq %rax"), "", true));
    Builder.CreateCall(
            InlineAsm::get(FunTy, std::string("addq $$") + std::to_string(RED_ZONE) + std::string(", %rsp"), "", true));

    return BB;
}

static RegisterPass<CheckerT> X("checkerT", "Inserts checkers before basic blocks for tamper proofing", false, false);
