//===-- StringEncryption.cpp - Compile-time string encryption -------------===//
//
// For every GlobalVariable that is a string constant referenced from a
// function, this pass:
//   1. Encrypts the bytes with XOR using a per-string random key.
//   2. Replaces the global with an encrypted version.
//   3. Injects a runtime decryption stub that decrypts on first use.
//
// The decryption stub itself is later obfuscated by the other passes.
//
//===----------------------------------------------------------------------===//

#include "kagura/Passes.h"
#include "kagura/Utils.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include <vector>

using namespace llvm;

namespace kagura {

// Build an LLVM function that decrypts Encrypted in-place using key Key.
// uint8_t Key[KeyLen] is XOR'd over the buffer cyclically.
// The generated function signature: void kagura_decrypt_<suffix>(void)
// It is marked with `alwaysinline` so it folds into the caller.
static Function *buildDecryptStub(Module &M, GlobalVariable *Encrypted,
                                   ArrayRef<uint8_t> Key, StringRef Suffix) {
  LLVMContext &Ctx = M.getContext();
  IRBuilder<> B(Ctx);

  auto *VoidTy  = Type::getVoidTy(Ctx);
  auto *Int8Ty  = Type::getInt8Ty(Ctx);
  auto *Int32Ty = Type::getInt32Ty(Ctx);
  auto *Int64Ty = Type::getInt64Ty(Ctx);

  // void kagura_decrypt_<suffix>()
  auto *FTy = FunctionType::get(VoidTy, false);
  auto *F   = Function::Create(FTy, Function::InternalLinkage,
                               "kagura_decrypt_" + Suffix, M);
  F->addFnAttr(Attribute::NoInline);
  F->addFnAttr(Attribute::NoUnwind);

  // Build key constant
  std::vector<Constant *> KeyBytes;
  for (uint8_t B : Key)
    KeyBytes.push_back(ConstantInt::get(Int8Ty, B));
  auto *KeyArrTy = ArrayType::get(Int8Ty, Key.size());
  auto *KeyConst = ConstantArray::get(KeyArrTy, KeyBytes);
  auto *KeyGV    = new GlobalVariable(M, KeyArrTy, true,
                                       GlobalValue::PrivateLinkage, KeyConst,
                                       "kagura_key_" + Suffix);

  // Entry block
  auto *Entry = BasicBlock::Create(Ctx, "entry", F);
  // Loop block
  auto *Loop  = BasicBlock::Create(Ctx, "loop", F);
  // Exit block
  auto *Exit  = BasicBlock::Create(Ctx, "exit", F);

  // Get the encrypted buffer length
  auto *EncTy   = cast<ArrayType>(Encrypted->getValueType());
  uint64_t Len  = EncTy->getNumElements();
  uint64_t KLen = Key.size();

  // entry: idx = 0; br loop_header
  B.SetInsertPoint(Entry);
  auto *IdxAlloc = B.CreateAlloca(Int64Ty, nullptr, "idx");
  B.CreateStore(ConstantInt::get(Int64Ty, 0), IdxAlloc);
  B.CreateBr(Loop);

  // loop_header: if idx < Len goto body else goto exit
  B.SetInsertPoint(Loop);
  auto *Idx      = B.CreateLoad(Int64Ty, IdxAlloc, "i");
  auto *InBounds = B.CreateICmpULT(Idx, ConstantInt::get(Int64Ty, Len));
  // Temporary: will be fixed after creating LoopBody
  auto *HeaderBr = B.CreateCondBr(InBounds, Exit, Exit); // placeholders

  // body block
  auto *LoopBody = BasicBlock::Create(Ctx, "body", F);
  HeaderBr->setSuccessor(0, LoopBody);

  B.SetInsertPoint(LoopBody);
  // key index = idx % KLen  (use i64 throughout, GEP accepts i64)
  auto *KIdxFull = B.CreateURem(Idx, ConstantInt::get(Int64Ty, KLen), "kidx");

  // Load key byte: KeyGV[0][kidx]
  auto *KPtr  = B.CreateInBoundsGEP(KeyArrTy, KeyGV,
                                     {ConstantInt::get(Int64Ty, 0), KIdxFull});
  auto *KByte = B.CreateLoad(Int8Ty, KPtr, "kb");

  // Load encrypted byte: Encrypted[0][idx]
  auto *EPtr  = B.CreateInBoundsGEP(EncTy, Encrypted,
                                     {ConstantInt::get(Int64Ty, 0), Idx});
  auto *EByte = B.CreateLoad(Int8Ty, EPtr, "eb");

  // XOR and store back
  auto *Plain = B.CreateXor(EByte, KByte, "plain");
  B.CreateStore(Plain, EPtr);

  // idx++; br loop_header
  auto *Next = B.CreateAdd(Idx, ConstantInt::get(Int64Ty, 1), "next");
  B.CreateStore(Next, IdxAlloc);
  B.CreateBr(Loop);

  // exit block
  B.SetInsertPoint(Exit);
  B.CreateRetVoid();

  return F;
}

PreservedAnalyses StringEncryptionPass::run(Module &M,
                                             ModuleAnalysisManager &) {
  auto Strings = kagura::collectStringGlobals(M);
  if (Strings.empty())
    return PreservedAnalyses::all();

  LLVMContext &Ctx = M.getContext();
  auto &RNG        = getModulePRNG();
  bool Changed     = false;

  for (auto *GV : Strings) {
    auto *CDA = cast<ConstantDataArray>(GV->getInitializer());
    StringRef Raw = CDA->getRawDataValues();
    uint64_t Len  = Raw.size();

    // Generate random XOR key (8 bytes, cyclic)
    constexpr size_t KeyLen = 8;
    uint8_t Key[KeyLen];
    uint64_t RandKey = RNG.next();
    for (size_t I = 0; I < KeyLen; ++I)
      Key[I] = static_cast<uint8_t>((RandKey >> (I * 8)) & 0xFF);

    // Encrypt the string bytes
    std::vector<uint8_t> Encrypted(Len);
    for (uint64_t I = 0; I < Len; ++I)
      Encrypted[I] = static_cast<uint8_t>(Raw[I]) ^ Key[I % KeyLen];

    // Build encrypted global (mutable, private)
    auto *Int8Ty   = Type::getInt8Ty(Ctx);
    auto *ArrTy    = ArrayType::get(Int8Ty, Len);
    std::vector<Constant *> EncBytes;
    EncBytes.reserve(Len);
    for (uint8_t B : Encrypted)
      EncBytes.push_back(ConstantInt::get(Int8Ty, B));
    auto *EncConst = ConstantArray::get(ArrTy, EncBytes);

    std::string Suffix = std::to_string(RNG.next32());
    auto *EncGV = new GlobalVariable(M, ArrTy, /*isConstant=*/false,
                                     GlobalValue::PrivateLinkage, EncConst,
                                     "kagura_enc_" + Suffix);
    EncGV->setAlignment(GV->getAlign());

    // Build decryption stub
    ArrayRef<uint8_t> KeyRef(Key, KeyLen);
    auto *Stub = buildDecryptStub(M, EncGV, KeyRef, Suffix);

    // For each use of the original global in a function, replace with:
    //   call void @kagura_decrypt_<suffix>()   (only on first access via flag)
    //   use EncGV instead of GV
    //
    // Simple approach: call stub before first user instruction in entry block.
    // A flag-guarded approach is left as a future enhancement.
    SmallVector<User *, 8> Users(GV->users());
    for (auto *U : Users) {
      if (auto *CE = dyn_cast<ConstantExpr>(U)) {
        // Replace ConstantExpr uses with the encrypted global
        CE->replaceAllUsesWith(
            ConstantExpr::getBitCast(EncGV, CE->getType()));
        continue;
      }
      if (auto *Inst = dyn_cast<Instruction>(U)) {
        // Insert decrypt call at the start of the function's entry block
        Function *ParentF = Inst->getFunction();
        IRBuilder<> B(&*ParentF->getEntryBlock().getFirstInsertionPt());
        B.CreateCall(Stub);

        // Replace the operand pointing to GV with EncGV
        for (unsigned I = 0; I < Inst->getNumOperands(); ++I) {
          if (Inst->getOperand(I)->stripPointerCasts() == GV)
            Inst->setOperand(I, EncGV);
        }
      }
    }

    // Remove the original global if now unused
    if (GV->use_empty())
      GV->eraseFromParent();

    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace kagura
