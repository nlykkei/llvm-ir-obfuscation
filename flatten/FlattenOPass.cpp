#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <map>
#include <string>

using namespace llvm;

namespace {
struct FlattenO : public FunctionPass {
  std::map<BasicBlock*, int> BBMap;
  
  static char ID;
  
  void assignIDToBasicBlocks(Function& F, std::map<BasicBlock*, int>& BBMap);
  
  FlattenO() : FunctionPass(ID) {}
  
	virtual bool runOnFunction(Function &F) {
        
      bool changed;
    
	  BasicBlock& EntryBB = F.front();
      EntryBB.setName("entry");
      
      Instruction* TermInstEntryBB = EntryBB.getTerminator();
      
      if (TermInstEntryBB->getOpcode() == Instruction::Ret) {
          return changed;
       }
    
      BranchInst* BrInstEntryBB = dyn_cast<BranchInst>(TermInstEntryBB);
      
      if (!BrInstEntryBB) {
          return changed;
      }
            
      BasicBlock* SwitchBB = SplitBlock(&EntryBB, BrInstEntryBB);
      SwitchBB->setName("switch");
      
      assignIDToBasicBlocks(F, BBMap);
      //printBBMap(BBMap);
            
      if (BrInstEntryBB->isConditional()) {
          // todo
      } else {
          // Add 'switch_index' stack slot to 'entry' BasicBlock
          int BrIndex = BBMap[BrInstEntryBB->getSuccessor(0)];
          Instruction& FirstInstEntryBB = EntryBB.front();
          IRBuilder<> Builder(&FirstInstEntryBB);
          Value* VAlloc = Builder.CreateAlloca(Type::getInt32Ty(F.getContext()), 0, "switch_index");
          Builder.CreateStore(ConstantInt::get(Type::getInt32Ty(F.getContext()), BrIndex), VAlloc);
        
          // Setup 'switch' BasicBlock
          BrInstEntryBB->eraseFromParent();
          Builder.SetInsertPoint(SwitchBB);
          Value* VLoad = Builder.CreateLoad(Type::getInt32Ty(F.getContext()), VAlloc); 
          SwitchInst* ISwitch = Builder.CreateSwitch(VLoad, SwitchBB, BBMap.size());
          
          // Add cases to switch: One case for each BasicBlock in Function.
          for (std::map<BasicBlock*, int>::iterator MI = BBMap.begin(), ME = BBMap.end(); MI != ME; ++MI) {
            if (MI->second != 0) {
                ISwitch->addCase(ConstantInt::get(Type::getInt32Ty(F.getContext()), MI->second), MI->first);
            }
          }
          
          // Retarget unconditional branch instructions to 'switch' BasicBlock
          for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; ++BI) {
            
            if (std::string(BI->getName()).compare("entry") == 0) {
              continue; // Ignore 'entry' BasicBlock
            }
            
            Instruction* TermInst = BI->getTerminator();
            BranchInst* BrInst = dyn_cast<BranchInst>(TermInst);
      
            if (!BrInst || BrInst->isConditional()) {
                continue;
            }
            
            int BrIndex = BBMap[BrInst->getSuccessor(0)];
            
            BrInst->setSuccessor(0, SwitchBB);
            Builder.SetInsertPoint(TermInst);
            Builder.CreateStore(ConstantInt::get(Type::getInt32Ty(F.getContext()), BrIndex), VAlloc);
            
            
          }  
        }
      
      //Instruction& PFI = EntryBB.front();
      //AllocaInst* AI = new AllocaInst(Type::getInt32Ty(F.getContext()), 0, "switch_index", &PFI);
	
    
    
    
    
	  return changed;
	}
};
}

char FlattenO::ID = 0;
static RegisterPass<FlattenO> X("flattenO", "Flattens the CFG by means of switching", false, false);


/// Map all BasicBlock's in Function 'f' to unique ID's. 
void FlattenO::assignIDToBasicBlocks(Function& F, std::map<BasicBlock*, int>& BBMap) {
    
    int BBID = 0;

    for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; ++BI) {  
        if (BI->hasName()) {
	      BBMap[&(*BI)] = BBID++;
	    } else {
	      BI->setName(Twine(std::string("bb") + std::to_string(BBID)));
	      BBMap[&(*BI)] = BBID++;
	    }
	  }
}