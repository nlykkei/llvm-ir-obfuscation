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
  struct AddO : public FunctionPass {
    
    static char ID;
    
    AddO() : FunctionPass(ID) {}
    
    virtual bool runOnFunction(Function &F) {

      bool changed = false;
      
      for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; ++BI) {
	for (BasicBlock::iterator II = BI->begin(), IE = BI->end(); II != IE; ++II) {
	  Instruction& I  = *II;
	  BinaryOperator* BO = dyn_cast<BinaryOperator>(&I);
	  
	  if (!BO || BO->getOpcode() != Instruction::Add) {
	    continue;
	  }
          
    	  IRBuilder<> Builder(BO);
	  Value* V = Builder.CreateAdd(Builder.CreateXor(BO->getOperand(0), BO->getOperand(1)), 
				       Builder.CreateMul(ConstantInt::get(BO->getType(), 2), 
							 Builder.CreateAnd(BO->getOperand(0), BO->getOperand(1))));
	  
	  ReplaceInstWithValue(BI->getInstList(), II, V);
	  
	  changed = true;
	} 
      }   
      
      return changed;
    }
  };
}

char AddO::ID = 0;
static RegisterPass<AddO> X("addO", "Obfuscation of add instructions", false, false);
