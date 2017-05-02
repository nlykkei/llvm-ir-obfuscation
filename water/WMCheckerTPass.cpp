#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/CFG.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>

#define DEBUG_TYPE "CheckerT"
#define RED_ZONE 128

using namespace llvm;

static const std::string defaultFilename = "corrector.dat";

static cl::list<std::string> CheckFns("checkfns",
                                      cl::desc("Functions containing basic block to check"),
                                      cl::value_desc("Function identifier"), cl::Positional);

static cl::list<std::string> CheckBBs("checkbbs",
                                      cl::desc("Basic blocks that should be checked"),
                                      cl::value_desc("Basic block identifier"), cl::Positional);

static cl::opt<std::string> Filename("file", cl::Optional, cl::desc("File containing watermarked basic block(s)"),
                                     cl::init(defaultFilename));

namespace {
    struct WMCheckerT : public ModulePass {
        static char ID;
        std::ofstream file;
        std::vector<int> usedRandom;


        WMCheckerT() : ModulePass(ID) {
            srand(time(NULL));
            file.open(Filename, std::ios::out | std::ios::trunc);
            if (!file.is_open()) {
                errs() << "Could not open file \'" << Filename << "\'" << "\n";
                exit(1);
            }
        }

        virtual ~WMCheckerT() override {
            file.close();
        }


        BasicBlock *insertCheckerBefore(BasicBlock *BB, std::string &Id);

        bool insertCorrectorSlot(BasicBlock *BB, std::string &Id);

        bool findAndCheckBB(Module &M, std::string FName, std::string BBName);

        virtual bool runOnModule(Module &M) {

            bool changed = false;

            if (CheckFns.size() != CheckBBs.size()) {
                errs() << "[Error] The size of the input lists differ!" << "\n";
                return false;
            }

            int size = CheckFns.size();

            for (int i = 0; i < size; ++i) {
                std::string FName = CheckFns[i];
                std::string BBName = CheckBBs[i];

                changed |= findAndCheckBB(M, FName, BBName);
            }

            return changed;
        }
    };
}

char WMCheckerT::ID = 0;

bool WMCheckerT::findAndCheckBB(Module &M, std::string FName, std::string BBName) {

    int r = 0;

    do {
        r = rand() % 1000;
    } while (std::find(usedRandom.begin(), usedRandom.end(), r) != usedRandom.end());

    usedRandom.push_back(r);

    std::stringstream ss;
    ss << std::setw(3) << std::setfill('0') << r;
    std::string pid(ss.str());

    for (auto &F : M) {
        if (FName == F.getName()) {
            for (auto &BB : F) {
                if (BBName == BB.getName()) {

                    if (&F.getEntryBlock() == &BB) {
                        errs() << "[Error] Cannot insert checker before " << FName << ":" << BBName << "\n";
                        errs() << "[Error] Trying to find another candidate..." << "\n";
                        continue;
                    }

                    std::string Id0 = pid + std::to_string(0);
                    file << ".cstart_" << Id0 << ":" << ".cend_" << Id0 << ":" << ".cslot_" + Id0 << "\n";

                    // Insert corrector slot in basic block
                    insertCorrectorSlot(&BB, Id0);

                    // Insert checker before basic block that dominates all uses
                    BasicBlock *Checker = insertCheckerBefore(&BB, Id0);

                    std::string Id1 = pid + std::to_string(1);
                    file << ".cstart_" << Id1 << ":" << ".cend_" << Id1 << ":" << ".cslot_" + Id1 << "\n";

                    // Insert corrector slot into checker
                    insertCorrectorSlot(Checker, Id1);

                    // Insert checker at random position into CFG to check inserted checker
                    int numBasicBlocks = F.getBasicBlockList().size();
                    int randPos = rand() % numBasicBlocks;
                    randPos = randPos == 0 ? randPos + 1
                                           : randPos; // Prevent inserting checker before 'entry'
                    Function::iterator It = F.begin();
                    std::advance(It, randPos);
                    BasicBlock *InsertBB = &*It;
                    insertCheckerBefore(InsertBB, Id1);

                    errs() << "[Info] Inserting checker before " << FName << ":" << BBName << "\n";

                    return true;
                }
            }
        }
    }

    errs() << "[Error] Cannot insert checker before " << FName << ":" << BBName << "\n";
    return false;
}


bool WMCheckerT::insertCorrectorSlot(BasicBlock *BB, std::string &Id) {
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

BasicBlock *WMCheckerT::insertCheckerBefore(BasicBlock *BB, std::string &Id) {
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

static RegisterPass<WMCheckerT> X("checkerWMT", "Inserts checkers before watermarked basic blocks for tamper proofing",
                                  false, false);

