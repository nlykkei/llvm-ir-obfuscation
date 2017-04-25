
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/CFG.h>
#include <fstream>
//#include <iostream>
#include <algorithm>
#include <llvm/Support/CommandLine.h>

#define DEBUG_TYPE "CheckerT"
#define RED_ZONE 128

using namespace llvm;

static const std::string defaultFilename = "watermark.dat";

static cl::list<unsigned int> Splits("splits", cl::Positional, cl::CommaSeparated,
                                     cl::desc("CRT watermark splits"));

static cl::opt<std::string> Filename("file", cl::Optional, cl::desc("File containing watermarked basic block(s)"),
                                     cl::init(defaultFilename));

namespace {
    struct ChineseWM : public ModulePass {
        static char ID;
        std::ofstream file;

        ChineseWM() : ModulePass(ID) {
            srand(time(NULL));
            file.open(Filename.c_str(), std::ios::out | std::ios::trunc);
            if (!file.is_open()) {
                errs() << "Could not open file: " << Filename << "\n";
            }
        }

        virtual ~ChineseWM() override {
            file.close();
        }


        void insertSplits(Module &M);

        virtual bool runOnModule(Module &M) {

            bool ready = false;

            for (auto &F : M) {
                if (!F.isDeclaration() && F.getBasicBlockList().size() > 1) {
                    ready = true;
                    break;
                }
            }

            if (ready) {
                insertSplits(M);
                return true;
            }

            return false;
        }
    };
}

char ChineseWM::ID = 0;

void ChineseWM::insertSplits(Module &M) {

    std::vector<BasicBlock *> WaterBB;
    int WM = 0;

    std::vector<Type *> ArgsTy;
    FunctionType *VoidFunTy = FunctionType::get(Type::getVoidTy(M.getContext()), ArgsTy, false);

    std::vector<Type *> ArgsTy2;
    ArgsTy2.push_back(Type::getInt32Ty(M.getContext()));
    FunctionType *IntFunTy = FunctionType::get(Type::getVoidTy(M.getContext()), ArgsTy2, false);

    std::vector<Function *> ValidFun;

    for (Function &F : M) {
        if (F.isDeclaration() || F.getBasicBlockList().size() == 1) {
            continue;
        }
        ValidFun.push_back(&F);
    }

    for (auto &Split : Splits) {

        int IdxF = 0;
        Module::iterator FI;
        int IdxBB = 0;
        Function::iterator BI;

        // Get random function in module
        IdxF = rand() % ValidFun.size();
        FI = M.begin();
        std::advance(FI, IdxF);

        // Get random basic block in function
        IdxBB = rand() % (FI->getBasicBlockList().size() - 1);
        IdxBB = IdxBB + 1;
        BI = FI->begin();
        std::advance(BI, IdxBB);

        DEBUG(errs() << "Inserting piece " << std::to_string(Split) << " into " << FI->getName() << ":" << BI->getName()
                     << "\n");

        if (std::find(WaterBB.begin(), WaterBB.end(), &*BI) == WaterBB.end()) {
            WaterBB.push_back(&*BI);
        } else {
            DEBUG(errs() << "Already inserted watermark into " << BI->getName() << "\n");
        }

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

    for (BasicBlock *BB : WaterBB) {
        file << std::string(BB->getParent()->getName()) << ":" << std::string(BB->getName()) << "\n";
        DEBUG(errs() << "Wrote \'" << BB->getParent()->getName() << ":" << BB->getName() << "\' to file" << "\n");
    }
}


static RegisterPass<ChineseWM> X("splitWM", "Inserts a CRT-split watermark into module", false, false);
