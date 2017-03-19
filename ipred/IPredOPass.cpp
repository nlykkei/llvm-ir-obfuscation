// Transformation:
//
//
// Before :
//                           entry
//                             |
//                       ______v______
//                      |   Original  |
//                      |_____________|
//                             |
//                             v
//                           return
//
// After :
//                           entry
//                             |
//                       ______v______
//                      |  phi nodes  |
//                      |  condition  | (false)
//                      |_____________|-------+
//                       (true)|              |
//                             |              |
//                       ______v______        |
//                  +-->|   Original* |       |
//                  |   |_____________|       |
//                  |   (false)|    | (true)  |
//                  |          |    +----------------> return
//                  |    ______v______        |
//                  |   |   Altered   |<------+
//                  |   |_____________|
//                  |          |
//                  +----------+
//

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <list>
#include <algorithm>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>

#define DEBUG_TYPE "IPredO"

// -stats
STATISTIC(InitNumBasicBlocks, "Initial numbers of basic blocks");
STATISTIC(ModifedNumBasicBlocks, "Modified number of basic blocks");
STATISTIC(AddedNumBasicBlocks, "Added number of basic blocks");
STATISTIC(FinalNumBasicBlocks, "Final number of basic blocks");

using namespace llvm;

static const int defaultObfRate = 100;
static const int defaultObfTime = 1;

static cl::opt<int>
        ObfProbRate("ipred-prob",
                    cl::desc("Probability [%] each basic blocks will be obfuscated"),
                    cl::value_desc("probability rate"), cl::init(defaultObfRate), cl::Optional);

static cl::opt<int>
        ObfTimes("ipred-times", cl::desc("Times the to loop on a function"),
                 cl::value_desc("number of times"), cl::init(defaultObfTime), cl::Optional);

namespace {
    struct IPredO : public ModulePass {
        static char ID;

        IPredO() : ModulePass(ID) {
            srand(time(NULL));
        }

        bool obfuscateCFG(Function &F);

        bool insertIPred(BasicBlock *BB);

        Value *insertIPredAndCondBefore(Instruction *I, bool Negate);

        void updateGlobalVariable(BasicBlock *BB);

        BasicBlock *createModifiedBasicBlock(BasicBlock *BB);

        virtual bool runOnModule(Module &M) {

            bool modified = false;

            if (ObfProbRate < 0 || ObfProbRate > 100) {
                M.getContext().emitError("-ipred-prob=p must be 0 <= p <= 100\n");
                M.getContext().emitError("Setting -ipred-prob to default value\n");
                ObfProbRate = defaultObfRate;
            }

            if (ObfTimes <= 0) {
                M.getContext().emitError("-ipred-times=n must be n > 0\n");
                M.getContext().emitError("Setting -pred-times to default value\n");
                ObfTimes = defaultObfTime;
            }

            std::vector<Type *> Args;
            FunctionType *FType = FunctionType::get(Type::getInt32Ty(M.getContext()), Args, false);
            M.getOrInsertFunction("rand", FType);

            M.getOrInsertGlobal("x", Type::getInt32Ty(M.getContext()));
            GlobalVariable *GVar = M.getNamedGlobal("x");

            if (!GVar) {
                M.getContext().emitError("Could not insert global variable into module\n");
                return false;
            }

            GVar->setInitializer(ConstantInt::get(Type::getInt32Ty(M.getContext()), rand() % 100));
            GVar->setLinkage(GlobalValue::InternalLinkage);

            for (auto &F : M) {
                modified |= obfuscateCFG(F);
            }

            return modified;
        }
    };
}

bool IPredO::obfuscateCFG(Function &F) {
    bool modified = false;

    DEBUG_WITH_TYPE("opt", errs() << "Obfuscating Function: " << F.getName() << "\n"); // -debug-only=opt,cfg
    DEBUG_WITH_TYPE("opt", errs() << "Probability rate: " << ObfProbRate << "\n");
    DEBUG_WITH_TYPE("opt", errs() << "Times: " << ObfTimes << "\n");

    int BBCount = std::distance(F.begin(), F.end());
    InitNumBasicBlocks += BBCount;
    FinalNumBasicBlocks += BBCount;

    for (int i = 0; i < ObfTimes; ++i) {
        // Must copy original basic blocks, since iterator becomes invalidated.
        std::list<BasicBlock *> BasicBlocks;
        for (auto &BB : F) {
            BasicBlocks.push_back(&BB);
        }

        for (auto &BB : BasicBlocks) {
            int p = rand() % 100 + 1;
            if (ObfProbRate >= p) {
                DEBUG_WITH_TYPE("opt", errs() << "Obfuscating BasicBlock: " << BB->getName() << "\n");
                if (insertIPred(BB)) {
                    ModifedNumBasicBlocks += 1;
                    AddedNumBasicBlocks += 3;
                    FinalNumBasicBlocks += 3;
                    modified = true;
                }
            } else {
                DEBUG_WITH_TYPE("opt", errs() << "Skipping: " << BB->getName() << "\n");
            }
        }
    }
    if (modified) {
        DEBUG_WITH_TYPE("cfg", errs() << "Function " << F.getName() << " has been modified\n");
        DEBUG_WITH_TYPE("cfg", F.viewCFG());
    } else {
        DEBUG_WITH_TYPE("cfg", errs() << "Function " << F.getName() << " has not been modified\n");
    }
    return modified;
}

bool IPredO::insertIPred(BasicBlock *BB) {
    Value* CmpRes;
    bool Negate;
    Instruction *SplitPoint = BB->getFirstNonPHIOrDbgOrLifetime();

    if (SplitPoint == BB->getTerminator()) {
        return false;
    }

    // The 'original' BasicBlock contains every instruction, except phi nodes and metadata
    BasicBlock *orgBBStart = BB->splitBasicBlock(SplitPoint, "orgBBStart");

    // Create a 'modified' BasicBlock based on the 'original' BasicBlock (control will never reach this block)
    BasicBlock *modifiedBB = createModifiedBasicBlock(orgBBStart);

    // Modify global variable 'x' to obfuscate control flow
    updateGlobalVariable(BB);
    updateGlobalVariable(orgBBStart);

    // Create invariant predicate with associated condition
    Negate = rand() & 0x01;
    CmpRes = insertIPredAndCondBefore(&BB->back(), Negate);

    // Erase old terminators to insert new ones
    BB->getTerminator()->eraseFromParent();
    modifiedBB->getTerminator()->eraseFromParent();

    // Branch to 'original' BasicBlock
    if (!Negate) {
        BranchInst::Create(orgBBStart, modifiedBB, CmpRes, BB);
    } else {
        BranchInst::Create(modifiedBB, orgBBStart, CmpRes, BB);
    }

    // The 'modified' BasicBlock branch to 'original' BasicBlock
    BranchInst::Create(orgBBStart, modifiedBB);

    // The 'original' BasicBlock may branch to 'modified' BasicBlock (control will never flow on this edge)
    BasicBlock *orgBBEnd = orgBBStart->splitBasicBlock(--orgBBStart->end(), "orgBBEnd");
    Negate = rand() & 0x01;
    CmpRes = insertIPredAndCondBefore(&orgBBStart->back(), Negate);
    orgBBStart->getTerminator()->eraseFromParent();

    if (!Negate) {
        BranchInst::Create(orgBBEnd, modifiedBB, CmpRes, orgBBStart);
    } else {
        BranchInst::Create(modifiedBB, orgBBEnd, CmpRes, orgBBStart);
    }
    return true;
}

char IPredO::ID = 0;

BasicBlock *IPredO::createModifiedBasicBlock(BasicBlock *BB) {
    ValueToValueMapTy VMap;
    BasicBlock *modifiedBB = CloneBasicBlock(BB, VMap, "mod", BB->getParent());

    for (BasicBlock::iterator BI = modifiedBB->begin(), BE = modifiedBB->end(); BI != BE; ++BI) {
        for (User::op_iterator OPI = BI->op_begin(), OPE = BI->op_end(); OPI != OPE; ++OPI) {
            Value *V = MapValue(*OPI, VMap);
            if (V) {
                *OPI = V;
            }
        }

        if (PHINode *PHI = dyn_cast<PHINode>(BI)) {
            for (int Idx = 0; Idx < PHI->getNumIncomingValues(); ++Idx) {
                Value *V = MapValue(PHI->getIncomingBlock(Idx), VMap);
                if (V) {
                    PHI->setIncomingBlock(Idx, cast<BasicBlock>(V));
                }
            }
        }
    }

    BasicBlock::iterator It = modifiedBB->begin();
    int InsertPos = rand() % std::distance(It, modifiedBB->end());
    std::advance(It, InsertPos);

    GlobalVariable *GVar = modifiedBB->getModule()->getNamedGlobal("x");

    if (!GVar) {
        BB->getContext().emitError("Could not find global variable in module\n");
    }

    IRBuilder<> Builder(&*It);
    // Increment global variable 'x' to look like a loop
    Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(BB->getContext()), GVar),
                                          ConstantInt::get(Type::getInt32Ty(BB->getContext()), 1)), GVar);

    return modifiedBB;
}



Value *IPredO::insertIPredAndCondBefore(Instruction *I, bool Negate) {
    Value *V, *LHS, *RHS, *Res;
    GlobalVariable *GVar = I->getModule()->getNamedGlobal("x"); // Wrapper around getGlobalVariable("x", true)

    if (!GVar) {
        I->getContext().emitError("Could not find global variable in module\n");
        exit(1);
    }

    IRBuilder<> Builder(I);

    switch (rand() % 3) {
        case 0:
            V = Builder.CreateURem(Builder.CreateLoad(Type::getInt32Ty(I->getContext()), GVar),
                                   ConstantInt::get(Type::getInt32Ty(I->getContext()),
                                                    19)); // GVar has type i32*

            LHS = Builder.CreateURem(Builder.CreateAdd(
                    Builder.CreateMul(ConstantInt::get(Type::getInt32Ty(I->getContext()), 4), Builder.CreateMul(V, V)),
                    ConstantInt::get(Type::getInt32Ty(I->getContext()), 4)),
                                     ConstantInt::get(Type::getInt32Ty(I->getContext()), 19));

            RHS = ConstantInt::get(Type::getInt32Ty(I->getContext()), 0);
            break;
        case 1:
            V = Builder.CreateURem(Builder.CreateLoad(Type::getInt32Ty(I->getContext()), GVar),
                                   ConstantInt::get(Type::getInt32Ty(I->getContext()),
                                                    11)); // GVar has type i32*

            LHS = Builder.CreateURem(Builder.CreateAdd(Builder.CreateAdd(
                    Builder.CreateMul(V, V),
                    Builder.CreateMul(ConstantInt::get(Type::getInt32Ty(I->getContext()), 4),V)), ConstantInt::get(Type::getInt32Ty(I->getContext()), 5)),
                                     ConstantInt::get(Type::getInt32Ty(I->getContext()), 11));

            RHS = ConstantInt::get(Type::getInt32Ty(I->getContext()), 0);
            break;
        case 2:
            V = Builder.CreateURem(Builder.CreateLoad(Type::getInt32Ty(I->getContext()), GVar),
                                   ConstantInt::get(Type::getInt32Ty(I->getContext()),
                                                    31)); // GVar has type i32*

            LHS = Builder.CreateURem(Builder.CreateAdd(Builder.CreateAdd(
                    Builder.CreateMul(ConstantInt::get(Type::getInt32Ty(I->getContext()), 5), Builder.CreateMul(V, V)),
                    Builder.CreateMul(ConstantInt::get(Type::getInt32Ty(I->getContext()), 6),V)), ConstantInt::get(Type::getInt32Ty(I->getContext()), 2)),
                                     ConstantInt::get(Type::getInt32Ty(I->getContext()), 31));

            RHS = ConstantInt::get(Type::getInt32Ty(I->getContext()), 0);
            break;
        default:
            break;
    }

    if (!Negate) {
        Res = Builder.CreateICmp(CmpInst::ICMP_NE, LHS, RHS);
    } else {
        Res = Builder.CreateICmp(CmpInst::ICMP_EQ, LHS, RHS);
    }

    return Res;
}

void IPredO::updateGlobalVariable(BasicBlock *BB) {
    Function *F = BB->getModule()->getFunction("rand");

    if (!F) {
        BB->getContext().emitError("Could not find function in module.");
        exit(1);
    }

    GlobalVariable *GVar = BB->getModule()->getNamedGlobal("x");

    if (!GVar) {
        BB->getContext().emitError("Could not find global variable in module.");
        exit(1);
    }

    IRBuilder<> Builder(&BB->back());

    switch (rand() % 7) {
        case 0:
            Builder.CreateStore(Builder.CreateURem(Builder.CreateCall(F),
                                                   ConstantInt::get(Type::getInt32Ty(BB->getContext()), 10)), GVar);
            break;
        case 1:
            Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(BB->getContext()), GVar),
                                                  ConstantInt::get(Type::getInt32Ty(BB->getContext()), rand() % 10)),
                                GVar);
            break;
        case 2:
            Builder.CreateStore(Builder.CreateSub(Builder.CreateLoad(Type::getInt32Ty(BB->getContext()), GVar),
                                                  ConstantInt::get(Type::getInt32Ty(BB->getContext()), rand() % 10)),
                                GVar);
            break;
        case 3:
            Builder.CreateStore(Builder.CreateMul(Builder.CreateLoad(Type::getInt32Ty(BB->getContext()), GVar),
                                                  ConstantInt::get(Type::getInt32Ty(BB->getContext()), rand() % 10)),
                                GVar);
            break;
        case 4:
            Builder.CreateStore(Builder.CreateShl(Builder.CreateLoad(Type::getInt32Ty(BB->getContext()), GVar),
                                                  ConstantInt::get(Type::getInt32Ty(BB->getContext()),
                                                                   (rand() % 3) + 1)),
                                GVar);
            break;
        case 5:
            Builder.CreateStore(Builder.CreateXor(Builder.CreateLoad(Type::getInt32Ty(BB->getContext()), GVar),
                                                  ConstantInt::get(Type::getInt32Ty(BB->getContext()), rand() % 10)),
                                GVar);
        case 6:
            // Do nothing
            break;
        default:
            break;
    }
}

static RegisterPass<IPredO> X("ipredO", "Obfuscates CFG by inserting invariant predicates.", false,
                              false);

