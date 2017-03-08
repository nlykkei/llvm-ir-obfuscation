#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#define DEBUG 0

using namespace llvm;

namespace {
    struct FlattenO : public ModulePass {

        static char ID;

        void assignIDToBasicBlocks(Function &F, std::map<BasicBlock *, int> &BBMap);

        void printBasicBlocksWithIDs(std::map<BasicBlock *, int> &BBMap);

        void insertOpaqueSwitchIndex(Instruction *insertBefore, int target, Value *destination);

        FlattenO()
                : ModulePass(ID) {
        }

        virtual bool runOnModule(Module &M) {
            std::map<BasicBlock *, int> BBMap; // Mapping between BasicBlocks and their unique IDs
            std::vector<BasicBlock *> BBSkip;  // BasicBlocks whose branch instructions are left unmodified

            // Insert global array and initialize it
            ArrayType *ArrayTy_0 = ArrayType::get(IntegerType::get(M.getContext(), 32), 10);

            M.getOrInsertGlobal("g_array", ArrayTy_0);
            GlobalVariable *GArray = M.getNamedGlobal("g_array");
            GArray->setAlignment(4);

            std::vector<llvm::Constant *> InitValues;

            InitValues.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), 22));  // [2] mod 5
            InitValues.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), 14));  // [4] mod 5
            InitValues.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), 73));  // [3] mod 5
            InitValues.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), 16));  // [5] mod 11
            InitValues.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), 37));  // [4] mod 11
            InitValues.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), 117)); // [7] mod 11
            InitValues.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), 2));   // [2] mod 11
            InitValues.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), 80));  // [3] mod 11
            InitValues.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), 19));  // [8] mod 11
            InitValues.push_back(ConstantInt::get(Type::getInt32Ty(M.getContext()), 77));  // [0] mod 7

            GArray->setInitializer(ConstantArray::get(ArrayTy_0, InitValues));

            // Insert global array index ("m" always points to [2] mod 5 "g_array")
            M.getOrInsertGlobal("m", Type::getInt32Ty(M.getContext()));
            GlobalVariable *GVar = M.getNamedGlobal("m");
            GVar->setInitializer(ConstantInt::get(Type::getInt32Ty(M.getContext()), 0));

            // Insert permute function
            std::vector<Type *> ArgsTy;

            ArgsTy.push_back(Type::getInt32PtrTy(M.getContext()));
            ArgsTy.push_back(Type::getInt32Ty(M.getContext()));
            ArgsTy.push_back(Type::getInt32PtrTy(M.getContext()));

            FunctionType *FunTy = FunctionType::get(Type::getVoidTy(M.getContext()), ArgsTy, false);
            Function::Create(FunTy, Function::ExternalLinkage, "permute", &M);

            for (Module::iterator FI = M.begin(), FE = M.end(); FI != FE; ++FI) {

                if (FI->isDeclaration()) {
                    continue;
                }

                BasicBlock &EntryBB = FI->front();
                EntryBB.setName("entry"); // Convenience name for 'entry' BasicBlock

                BBSkip.push_back(&EntryBB); // The 'entry' BasicBlock should not have its branches modified

                Instruction *TermInstEntryBB = EntryBB.getTerminator();

                // Check whether function consists of only one BasicBlock
                if (std::distance(FI->begin(), FI->end()) == 1) {
                    errs() << FI->getName() << " consists only of one BasicBlock."
                           << "\n";
                    return false;
                }

                // Check whether other BasicBlocks are dead
                if (TermInstEntryBB->getOpcode() == Instruction::Ret) {
                    errs() << FI->getName() << " has only one BasicBlock that is not dead."
                           << "\n";
                    return false;
                }

                // The 'entry' BasicBlock should branch
                BranchInst *BrInstEntryBB = dyn_cast<BranchInst>(TermInstEntryBB);

                if (!BrInstEntryBB) {
                    errs() << FI->getName() << " should end with a branch instruction"
                           << "\n";
                    return false;
                }

                BasicBlock *SwitchBB = SplitBlock(&EntryBB, BrInstEntryBB);
                SwitchBB->setName("switch");
                BBSkip.push_back(SwitchBB);

                assignIDToBasicBlocks(*FI, BBMap);

                // Add 'switch_index' stack slot to 'entry' BasicBlock
                IRBuilder<> Builder(&EntryBB.front());
                Value *VAlloc = Builder.CreateAlloca(Type::getInt32Ty(FI->getContext()), 0, "switch_index");

                if (BrInstEntryBB->isConditional()) {
                    TerminatorInst *SplitTerm = EntryBB.getTerminator(); // br label %switch
                    TerminatorInst *IfTrueTerm = SplitBlockAndInsertIfThen(BrInstEntryBB->getCondition(), SplitTerm,
                                                                           false);

                    // Setup 'if.true' BasicBlock
                    IfTrueTerm->getParent()->setName(std::string(EntryBB.getName()) + std::string(".if.true"));
                    // Builder.SetInsertPoint(IfTrueTerm);
                    // Builder.CreateStore(
                    //    ConstantInt::get(Type::getInt32Ty(F.getContext()), BBMap[BrInstEntryBB->getSuccessor(0)]), VAlloc);
                    insertOpaqueSwitchIndex(IfTrueTerm, BBMap[BrInstEntryBB->getSuccessor(0)], VAlloc);
                    IfTrueTerm->setSuccessor(0, SwitchBB);
                    BBSkip.push_back(IfTrueTerm->getParent());

                    // Setup 'if.cont' BasicBlock
                    SplitTerm->getParent()->setName(std::string(EntryBB.getName()) + std::string(".if.cont"));
                    // Builder.SetInsertPoint(SplitTerm);
                    // Builder.CreateStore(
                    //    ConstantInt::get(Type::getInt32Ty(F.getContext()), BBMap[BrInstEntryBB->getSuccessor(1)]), VAlloc);
                    insertOpaqueSwitchIndex(SplitTerm, BBMap[BrInstEntryBB->getSuccessor(1)], VAlloc);
                    BBSkip.push_back(SplitTerm->getParent());
                } else {
                    // Builder.SetInsertPoint(EntryBB.getTerminator());
                    // Builder.CreateStore(
                    //    ConstantInt::get(Type::getInt32Ty(F.getContext()), BBMap[BrInstEntryBB->getSuccessor(0)]), VAlloc);
                    insertOpaqueSwitchIndex(EntryBB.getTerminator(), BBMap[BrInstEntryBB->getSuccessor(0)], VAlloc);
                }

                BrInstEntryBB->eraseFromParent(); // Remove original 'entry' BasicBlock branch from 'switch' BasicBlock

                // Setup 'switch' BasicBlock
                Builder.SetInsertPoint(SwitchBB);
                Value *VLoad = Builder.CreateLoad(Type::getInt32Ty(FI->getContext()), VAlloc);
                SwitchInst *ISwitch = Builder.CreateSwitch(VLoad, SwitchBB, BBMap.size());

                // Add cases to switch: One case for each BasicBlock in Function
                for (std::map<BasicBlock *, int>::iterator MI = BBMap.begin(), ME = BBMap.end(); MI != ME; ++MI) {
                    if (MI->second != 0) {
                        ISwitch->addCase(ConstantInt::get(Type::getInt32Ty(FI->getContext()), MI->second), MI->first);
                    }
                }

                // Retarget all branch instructions in BasicBlocks to 'switch' BasicBlock
                for (Function::iterator BI = FI->begin(), BE = FI->end(); BI != BE; ++BI) {

                    if (std::find(BBSkip.begin(), BBSkip.end(), &(*BI)) != BBSkip.end()) {
                        errs() << "Skip: " << BI->getName() << "\n";
                        continue;
                    }

                    BranchInst *BrInst = dyn_cast<BranchInst>(BI->getTerminator());

                    if (!BrInst) {
                        continue;
                    }

                    if (BrInst->isConditional()) {
                        TerminatorInst *IfTrueTerm = SplitBlockAndInsertIfThen(BrInst->getCondition(), BrInst, false);

                        // Setup 'if.true' BasicBlock
                        IfTrueTerm->getParent()->setName(std::string(BI->getName()) + std::string(".if.true"));
                        // Builder.SetInsertPoint(IfTrueTerm);
                        // Builder.CreateStore(
                        //    ConstantInt::get(Type::getInt32Ty(F.getContext()), BBMap[BrInst->getSuccessor(0)]), VAlloc);
                        insertOpaqueSwitchIndex(IfTrueTerm, BBMap[BrInst->getSuccessor(0)], VAlloc);
                        IfTrueTerm->setSuccessor(0, SwitchBB);
                        BBSkip.push_back(IfTrueTerm->getParent());

                        // Setup 'if.cont' BasicBlock
                        BrInst->getParent()->setName(std::string(BI->getName()) + std::string(".if.cont"));
                        // Builder.SetInsertPoint(BrInst);
                        // Builder.CreateStore(
                        //    ConstantInt::get(Type::getInt32Ty(F.getContext()), BBMap[BrInst->getSuccessor(1)]), VAlloc);
                        insertOpaqueSwitchIndex(BrInst, BBMap[BrInst->getSuccessor(1)], VAlloc);
                        BBSkip.push_back(BrInst->getParent());
                        Builder.SetInsertPoint(BrInst);
                        Builder.CreateBr(SwitchBB);
                        BrInst->eraseFromParent(); // Erase conditional branch
                    } else {
                        // Builder.SetInsertPoint(BrInst);
                        // Builder.CreateStore(
                        //    ConstantInt::get(Type::getInt32Ty(F.getContext()), BBMap[BrInst->getSuccessor(0)]), VAlloc);
                        insertOpaqueSwitchIndex(BrInst, BBMap[BrInst->getSuccessor(0)], VAlloc);
                        BrInst->setSuccessor(0, SwitchBB);
                    }
                }
            }

            return true;
        }
    };
}

char FlattenO::ID = 0;
static RegisterPass<FlattenO> X("flattenO", "Flattens the CFG by means of switching", false, false);

/// Assign unique ID's to all BasicBlock's in Function 'F'
void FlattenO::assignIDToBasicBlocks(Function &F, std::map<BasicBlock *, int> &BBMap) {
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

/// Print BasicBlock's and their associated ID's
void FlattenO::printBasicBlocksWithIDs(std::map<BasicBlock *, int> &BBMap) {
    for (std::map<BasicBlock *, int>::iterator MI = BBMap.begin(), ME = BBMap.end(); MI != ME; ++MI) {
        errs() << MI->first->getName() << " has ID " << MI->second << "\n";
    }
}

/// Assign an opaque value as switch index.
/// The assigned value is equal to 'target', but is computed from array aliasing
void FlattenO::insertOpaqueSwitchIndex(Instruction *insertBefore, int target, Value *destination) {
    int quotient = target / 10;
    int remainder = target % 10; // Integer between 0 and 9
    Module *M = insertBefore->getModule();

    ArrayType *ArrayTy_0 = ArrayType::get(IntegerType::get(M->getContext(), 32), 10);
    GlobalVariable *GArray = M->getNamedGlobal("g_array");

    GlobalVariable *GVar = M->getNamedGlobal("m");

    std::vector<Type *> ArgsTy;

    ArgsTy.push_back(Type::getInt32PtrTy(M->getContext()));
    ArgsTy.push_back(Type::getInt32Ty(M->getContext()));
    ArgsTy.push_back(Type::getInt32PtrTy(M->getContext()));

    FunctionType *FunTy = FunctionType::get(Type::getVoidTy(M->getContext()), ArgsTy, false);
    Constant *Const = M->getOrInsertFunction("permute", FunTy);

    Function *FPermute = dyn_cast<Function>(Const);

    if (!FPermute) {
        errs() << "Could not find prototype for permute() function."
               << "\n";
    }

    IRBuilder<> Builder(insertBefore);

    // Permute array of values

    std::vector<Value *> Args;

    std::vector<Value *> IdxList;
    IdxList.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
    IdxList.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));

    Args.push_back(Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList))); // Array pointer
    Args.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 10));         // Array length
    Args.push_back(GVar);                                                            // Index pointer

    Builder.CreateCall(FPermute, Args);

    switch (remainder) {
        case 0: {

            Value *VOffset =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 9), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            std::vector<Value *> IdxList;
            IdxList.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList.push_back(VOffset);

            Value *VTargetPtr = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList), "target_ptr");

            Value *VTarget = Builder.CreateAdd(
                    Builder.CreateURem(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr, "part"),
                                       ConstantInt::get(Type::getInt32Ty(M->getContext()), 7), "total"),
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), quotient * 10), "target_low");

            Builder.CreateStore(VTarget, destination);

            break;
        }
        case 1: {
            Value *VOffset1 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            Value *VOffset2 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 2), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            std::vector<Value *> IdxList1;
            IdxList1.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList1.push_back(VOffset1);

            Value *VTargetPtr1 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList1), "target_ptr");

            std::vector<Value *> IdxList2;
            IdxList2.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList2.push_back(VOffset2);

            Value *VTargetPtr2 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList2), "target_ptr");

            Value *VTargetLow = Builder.CreateURem(
                    Builder.CreateMul(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr1, "part_2"),
                                      Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr2, "part_1"),
                                      "total"),
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 5), "target_low");

            Value *VTarget = Builder.CreateAdd(
                    VTargetLow, ConstantInt::get(Type::getInt32Ty(M->getContext()), quotient * 10), "target_val");

            Builder.CreateStore(VTarget, destination);

            break;
        }
        case 2: {
            Value *VOffset1 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 1), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            Value *VOffset2 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 2), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            std::vector<Value *> IdxList1;
            IdxList1.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList1.push_back(VOffset1);

            Value *VTargetPtr1 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList1), "target_ptr");

            std::vector<Value *> IdxList2;
            IdxList2.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList2.push_back(VOffset2);

            Value *VTargetPtr2 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList2), "target_ptr");

            Value *VTargetLow = Builder.CreateURem(
                    Builder.CreateMul(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr1, "part_2"),
                                      Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr2, "part_1"),
                                      "total"),
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 5), "target_low");

            Value *VTarget = Builder.CreateAdd(
                    VTargetLow, ConstantInt::get(Type::getInt32Ty(M->getContext()), quotient * 10), "target_val");

            Builder.CreateStore(VTarget, destination);

            break;
        }
        case 3: {
            Value *VOffset1 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            Value *VOffset2 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 1), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            std::vector<Value *> IdxList1;
            IdxList1.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList1.push_back(VOffset1);

            Value *VTargetPtr1 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList1), "target_ptr");

            std::vector<Value *> IdxList2;
            IdxList2.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList2.push_back(VOffset2);

            Value *VTargetPtr2 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList2), "target_ptr");

            Value *VTargetLow = Builder.CreateURem(
                    Builder.CreateMul(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr1, "part_2"),
                                      Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr2, "part_1"),
                                      "total"),
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 5), "target_low");

            Value *VTarget = Builder.CreateAdd(
                    VTargetLow, ConstantInt::get(Type::getInt32Ty(M->getContext()), quotient * 10), "target_val");

            Builder.CreateStore(VTarget, destination);

            break;
        }
        case 4: {
            Value *VOffset1 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 0), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            Value *VOffset2 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 1), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            Value *VOffset3 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 2), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            std::vector<Value *> IdxList1;
            IdxList1.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList1.push_back(VOffset1);

            Value *VTargetPtr1 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList1), "target_ptr");

            std::vector<Value *> IdxList2;
            IdxList2.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList2.push_back(VOffset2);

            Value *VTargetPtr2 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList2), "target_ptr");

            std::vector<Value *> IdxList3;
            IdxList3.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList3.push_back(VOffset3);

            Value *VTargetPtr3 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList3), "target_ptr");

            Value *VTargetLow = Builder.CreateURem(
                    Builder.CreateMul(
                            Builder.CreateURem(
                                    Builder.CreateMul(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr1,
                                                                         "part_3"),
                                                      Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr2,
                                                                         "part_2"), "part_4"),
                                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 5), "part_5"),
                            Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr3, "part_1"), "total"),
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 5), "target_low");

            Value *VTarget = Builder.CreateAdd(
                    VTargetLow, ConstantInt::get(Type::getInt32Ty(M->getContext()), quotient * 10), "target_val");

            Builder.CreateStore(VTarget, destination);

            break;
        }
        case 5: {
            Value *VOffset1 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 6), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            Value *VOffset2 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 8), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            std::vector<Value *> IdxList1;
            IdxList1.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList1.push_back(VOffset1);

            Value *VTargetPtr1 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList1), "target_ptr");

            std::vector<Value *> IdxList2;
            IdxList2.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList2.push_back(VOffset2);

            Value *VTargetPtr2 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList2), "target_ptr");

            Value *VTargetLow = Builder.CreateURem(
                    Builder.CreateMul(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr1, "part_2"),
                                      Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr2, "part_1"),
                                      "total"),
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 11), "target_low");

            Value *VTarget = Builder.CreateAdd(
                    VTargetLow, ConstantInt::get(Type::getInt32Ty(M->getContext()), quotient * 10), "target_val");

            Builder.CreateStore(VTarget, destination);

            break;
        }
        case 6: {
            Value *VOffset1 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 6), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            Value *VOffset2 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 7), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            std::vector<Value *> IdxList1;
            IdxList1.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList1.push_back(VOffset1);

            Value *VTargetPtr1 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList1), "target_ptr");

            std::vector<Value *> IdxList2;
            IdxList2.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList2.push_back(VOffset2);

            Value *VTargetPtr2 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList2), "target_ptr");

            Value *VTargetLow = Builder.CreateURem(
                    Builder.CreateMul(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr1, "part_2"),
                                      Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr2, "part_1"),
                                      "total"),
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 11), "target_low");

            Value *VTarget = Builder.CreateAdd(
                    VTargetLow, ConstantInt::get(Type::getInt32Ty(M->getContext()), quotient * 10), "target_val");

            Builder.CreateStore(VTarget, destination);

            break;
        }
        case 7: {
            Value *VOffset =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 5), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            std::vector<Value *> IdxList;
            IdxList.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList.push_back(VOffset);

            Value *VTargetPtr = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList), "target_ptr");

            Value *VTarget = Builder.CreateAdd(
                    Builder.CreateURem(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr, "part"),
                                       ConstantInt::get(Type::getInt32Ty(M->getContext()), 11), "target_low"),
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), quotient * 10), "target_val");

            Builder.CreateStore(VTarget, destination);

            break;
        }
        case 8: {
            Value *VOffset1 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 3), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            Value *VOffset2 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 4), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            Value *VOffset3 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 5), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            std::vector<Value *> IdxList1;
            IdxList1.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList1.push_back(VOffset1);

            Value *VTargetPtr1 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList1), "target_ptr");

            std::vector<Value *> IdxList2;
            IdxList2.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList2.push_back(VOffset2);

            Value *VTargetPtr2 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList2), "target_ptr");

            std::vector<Value *> IdxList3;
            IdxList3.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList3.push_back(VOffset3);

            Value *VTargetPtr3 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList3), "target_ptr");

            Value *VTargetLow = Builder.CreateURem(
                    Builder.CreateMul(
                            Builder.CreateURem(
                                    Builder.CreateMul(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr1,
                                                                         "part_3"),
                                                      Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr2,
                                                                         "part_2"), "part_4"),
                                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 11), "part_5"),
                            Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr3, "part_1"), "total"),
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 11), "target_low");

            Value *VTarget = Builder.CreateAdd(
                    VTargetLow, ConstantInt::get(Type::getInt32Ty(M->getContext()), quotient * 10), "target_val");

            Builder.CreateStore(VTarget, destination);

            break;
        }
        case 9: {
            Value *VOffset1 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 3), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            Value *VOffset2 =
                    Builder.CreateURem(
                            Builder.CreateAdd(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), GVar, "m_val"),
                                              ConstantInt::get(Type::getInt32Ty(M->getContext()), 4), "array_offset"),
                            ConstantInt::get(Type::getInt32Ty(M->getContext()), 10), "array_index");

            std::vector<Value *> IdxList1;
            IdxList1.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList1.push_back(VOffset1);

            Value *VTargetPtr1 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList1), "target_ptr");

            std::vector<Value *> IdxList2;
            IdxList2.push_back(ConstantInt::get(Type::getInt32Ty(M->getContext()), 0));
            IdxList2.push_back(VOffset2);

            Value *VTargetPtr2 = Builder.CreateGEP(ArrayTy_0, GArray, ArrayRef<Value *>(IdxList2), "target_ptr");

            Value *VTargetLow = Builder.CreateURem(
                    Builder.CreateMul(Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr1, "part_2"),
                                      Builder.CreateLoad(Type::getInt32Ty(M->getContext()), VTargetPtr2, "part_1"),
                                      "total"),
                    ConstantInt::get(Type::getInt32Ty(M->getContext()), 11), "target_low");

            Value *VTarget = Builder.CreateAdd(
                    VTargetLow, ConstantInt::get(Type::getInt32Ty(M->getContext()), quotient * 10), "target_val");

            Builder.CreateStore(VTarget, destination);

            break;
        }
        default: {
            errs() << "error: default case reached in insertOpaqueSwitchIndex()."
                   << "\n";
            break;
        }
    }
}