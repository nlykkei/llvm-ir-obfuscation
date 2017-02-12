#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <string>
#include <vector>

using namespace llvm;

namespace
{
struct FlattenO : public FunctionPass {

    static char ID;

    void assignIDToBasicBlocks(Function& F, std::map<BasicBlock*, int>& BBMap);
    void printBasicBlocksWithIDs(std::map<BasicBlock*, int>& BBMap);

    FlattenO()
        : FunctionPass(ID)
    {
    }

    virtual bool runOnFunction(Function& F)
    {
        std::map<BasicBlock*, int> BBMap; // Mapping between BasicBlocks and their unique IDs
        std::vector<BasicBlock*> BBSkip;  // BasicBlocks whose branch instructions are left unmodified

        BasicBlock& EntryBB = F.front();
        EntryBB.setName("entry"); // Convenience name for 'entry' BasicBlock

        BBSkip.push_back(&EntryBB); // The 'entry' BasicBlock should not have its branches modified

        Instruction* TermInstEntryBB = EntryBB.getTerminator();

        // Check whether function consists of only one BasicBlock
        if(std::distance(F.begin(), F.end()) == 1) {
            errs() << F.getName() << " consists only of one BasicBlock."
                   << "\n";
            return false;
        }

        // Check whether other BasicBlocks are dead
        if(TermInstEntryBB->getOpcode() == Instruction::Ret) {
            errs() << F.getName() << " has only one BasicBlock that is not dead."
                   << "\n";
            return false;
        }

        // The 'entry' BasicBlock should branch
        BranchInst* BrInstEntryBB = dyn_cast<BranchInst>(TermInstEntryBB);

        if(!BrInstEntryBB) {
            errs() << F.getName() << " should end with a branch instruction"
                   << "\n";
            return false;
        }

        BasicBlock* SwitchBB = SplitBlock(&EntryBB, BrInstEntryBB);
        SwitchBB->setName("switch");
        BBSkip.push_back(SwitchBB);

        assignIDToBasicBlocks(F, BBMap);

        // Add 'switch_index' stack slot to 'entry' BasicBlock
        IRBuilder<> Builder(&EntryBB.front());
        Value* VAlloc = Builder.CreateAlloca(Type::getInt32Ty(F.getContext()), 0, "switch_index");

        if(BrInstEntryBB->isConditional()) {
            TerminatorInst* SplitTerm = EntryBB.getTerminator(); // br label %switch
            TerminatorInst* IfTrueTerm = SplitBlockAndInsertIfThen(BrInstEntryBB->getCondition(), SplitTerm, false);

            // Setup 'if.true' BasicBlock
            IfTrueTerm->getParent()->setName(std::string(EntryBB.getName()) + std::string(".if.true"));
            Builder.SetInsertPoint(IfTrueTerm);
            Builder.CreateStore(
                ConstantInt::get(Type::getInt32Ty(F.getContext()), BBMap[BrInstEntryBB->getSuccessor(0)]), VAlloc);
            IfTrueTerm->setSuccessor(0, SwitchBB);
            BBSkip.push_back(IfTrueTerm->getParent());

            // Setup 'if.cont' BasicBlock
            SplitTerm->getParent()->setName(std::string(EntryBB.getName()) + std::string(".if.cont"));
            Builder.SetInsertPoint(SplitTerm);
            Builder.CreateStore(
                ConstantInt::get(Type::getInt32Ty(F.getContext()), BBMap[BrInstEntryBB->getSuccessor(1)]), VAlloc);
            BBSkip.push_back(SplitTerm->getParent());
        } else {
            Builder.SetInsertPoint(EntryBB.getTerminator());
            Builder.CreateStore(
                ConstantInt::get(Type::getInt32Ty(F.getContext()), BBMap[BrInstEntryBB->getSuccessor(0)]), VAlloc);
        }

        BrInstEntryBB->eraseFromParent(); // Remove original 'entry' BasicBlock branch from 'switch' BasicBlock

        // Setup 'switch' BasicBlock
        Builder.SetInsertPoint(SwitchBB);
        Value* VLoad = Builder.CreateLoad(Type::getInt32Ty(F.getContext()), VAlloc);
        SwitchInst* ISwitch = Builder.CreateSwitch(VLoad, SwitchBB, BBMap.size());

        // Add cases to switch: One case for each BasicBlock in Function
        for(std::map<BasicBlock *, int>::iterator MI = BBMap.begin(), ME = BBMap.end(); MI != ME; ++MI) {
            if(MI->second != 0) {
                ISwitch->addCase(ConstantInt::get(Type::getInt32Ty(F.getContext()), MI->second), MI->first);
            }
        }

        // Retarget all branch instructions in BasicBlocks to 'switch' BasicBlock
        for(Function::iterator BI = F.begin(), BE = F.end(); BI != BE; ++BI) {

            if(std::find(BBSkip.begin(), BBSkip.end(), &(*BI)) != BBSkip.end()) {
                errs() << "Skip: " << BI->getName() << "\n";
                continue;
            }

            BranchInst* BrInst = dyn_cast<BranchInst>(BI->getTerminator());

            if(!BrInst) {
                continue;
            }

            if(BrInst->isConditional()) {
                TerminatorInst* IfTrueTerm = SplitBlockAndInsertIfThen(BrInst->getCondition(), BrInst, false);

                // Setup 'if.true' BasicBlock
                IfTrueTerm->getParent()->setName(std::string(BI->getName()) + std::string(".if.true"));
                Builder.SetInsertPoint(IfTrueTerm);
                Builder.CreateStore(
                    ConstantInt::get(Type::getInt32Ty(F.getContext()), BBMap[BrInst->getSuccessor(0)]), VAlloc);
                IfTrueTerm->setSuccessor(0, SwitchBB);
                BBSkip.push_back(IfTrueTerm->getParent());

                // Setup 'if.cont' BasicBlock
                BrInst->getParent()->setName(std::string(BI->getName()) + std::string(".if.cont"));
                Builder.SetInsertPoint(BrInst);
                Builder.CreateStore(
                    ConstantInt::get(Type::getInt32Ty(F.getContext()), BBMap[BrInst->getSuccessor(1)]), VAlloc);
                BBSkip.push_back(BrInst->getParent());
                Builder.CreateBr(SwitchBB);
                BrInst->eraseFromParent(); // Erase conditional branch
            } else {
                Builder.SetInsertPoint(BrInst);
                Builder.CreateStore(
                    ConstantInt::get(Type::getInt32Ty(F.getContext()), BBMap[BrInst->getSuccessor(0)]), VAlloc);
                BrInst->setSuccessor(0, SwitchBB);
            }
        }

        return true;
    }
};
}

char FlattenO::ID = 0;
static RegisterPass<FlattenO> X("flattenO", "Flattens the CFG by means of switching", false, false);

/// Assign unique ID's to all BasicBlock's in Function 'F'
void FlattenO::assignIDToBasicBlocks(Function& F, std::map<BasicBlock*, int>& BBMap)
{
    int BBID = 0;

    for(Function::iterator BI = F.begin(), BE = F.end(); BI != BE; ++BI) {
        if(BI->hasName()) {
            BBMap[&(*BI)] = BBID++;
        } else {
            BI->setName(Twine(std::string("bb") + std::to_string(BBID)));
            BBMap[&(*BI)] = BBID++;
        }
    }
}

void FlattenO::printBasicBlocksWithIDs(std::map<BasicBlock*, int>& BBMap)
{
    for(std::map<BasicBlock *, int>::iterator MI = BBMap.begin(), ME = BBMap.end(); MI != ME; ++MI) {
        errs() << MI->first->getName() << " has ID " << MI->second << "\n";
    }
}