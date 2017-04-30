
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
                errs() << "Could not open file \'" << Filename << "\'" << "\n";
                exit(1);
            }
        }

        virtual ~ChineseWM() override {
            file.close();
        }


        void insertSplits(Module &M, std::vector<Function *> &ValidFn);

        virtual bool runOnModule(Module &M) {

            std::vector<Function *> ValidFn;

            for (Function &F : M) {
                if (!F.isDeclaration() && F.getBasicBlockList().size() > 1) {
                    ValidFn.push_back(&F);
                }
            }

            if (!ValidFn.empty()) {
                insertSplits(M, ValidFn);
                return true;
            }

            return false;
        }
    };
}

char ChineseWM::ID = 0;

void ChineseWM::insertSplits(Module &M, std::vector<Function *> &ValidFn) {

    std::vector<BasicBlock *> WaterBB;
    int WM = 0;

    std::vector<Type *> ArgsTy0;
    FunctionType *VoidFunTy = FunctionType::get(Type::getVoidTy(M.getContext()), ArgsTy0, false);

    std::vector<Type *> ArgsTy1;
    ArgsTy1.push_back(Type::getInt32Ty(M.getContext()));
    FunctionType *IntFunTy0 = FunctionType::get(Type::getVoidTy(M.getContext()), ArgsTy1, false);

    std::vector<Type *> ArgsTy2;
    ArgsTy2.push_back(Type::getInt32Ty(M.getContext()));
    ArgsTy2.push_back(Type::getInt32Ty(M.getContext()));
    FunctionType *IntFunTy1 = FunctionType::get(Type::getVoidTy(M.getContext()), ArgsTy2, false);

    for (auto &Split : Splits) {

        int IdxF = 0;
        std::vector<Function *>::iterator FI;
        int IdxBB = 0;
        Function::iterator BI;

        // Get random function in module
        IdxF = rand() % ValidFn.size();
        FI = ValidFn.begin();
        std::advance(FI, IdxF);

        // Get random basic block in function
        IdxBB = rand() % ((*FI)->getBasicBlockList().size() - 1);
        IdxBB = IdxBB + 1;
        BI = (*FI)->begin();
        std::advance(BI, IdxBB);

        errs() << "[Info] Inserting watermark piece " << std::to_string(Split) << " into " << (*FI)->getName() << ":" << BI->getName() << "\n";

        if (std::find(WaterBB.begin(), WaterBB.end(), &*BI) == WaterBB.end()) {
            WaterBB.push_back(&*BI);
        }

        Instruction *I = &*BI->getFirstInsertionPt();

        IRBuilder<> Builder(I);

        Builder.CreateCall(InlineAsm::get(VoidFunTy, std::string("jmp .end_") + std::to_string(WM), "", true));

        Builder.CreateCall(
                InlineAsm::get(VoidFunTy, std::string(".wm_split") + std::to_string(WM) + std::string(":"), "", true));

        std::vector<Value *> CArgs0;
        CArgs0.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), 15));
        CArgs0.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), 0x66));

        Builder.CreateCall(InlineAsm::get(IntFunTy1, std::string(".fill ${0:c}, 1, ${1:c}"), "i,i", true), CArgs0); // eliminate $

        std::vector<Value *> CArgs1;
        CArgs1.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), Split));
        Builder.CreateCall(InlineAsm::get(IntFunTy0, std::string(".4byte ${0:c}"), "i", true), CArgs1); // eliminate $

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
