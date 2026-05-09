//===-- StringEncryptionAES.cpp - AES-128 CTR string encryption -----------===//
//
// For every GlobalVariable that is a string constant referenced from a
// function, this pass:
//   1. Encrypts the bytes with AES-128-CTR using a per-string random key+nonce.
//   2. Stores the encrypted blob, key, and nonce as module globals.
//   3. Replaces uses of the original global with a call to a per-string
//      decryption stub that calls kagura_aes128_ctr_decrypt() at runtime.
//
// 4.2.5: Short-lived decrypted buffer (zero after use)
//   The output buffer (kagura_aesbuf_<suffix>) is zeroed by injecting a call
//   to kagura_zero_buf(ptr, len) immediately after the last use of the
//   decrypted pointer in each basic block.  This minimises the window during
//   which the plaintext is resident in memory.
//
// Registered as: -kagura-str-aes
//
//===----------------------------------------------------------------------===//

#include "AES128.h"
#include "kagura/Passes.h"
#include "kagura/Utils.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include <cstdint>
#include <vector>

using namespace llvm;

namespace kagura {

//===----------------------------------------------------------------------===//
// Helper: declare kagura_aes128_ctr_decrypt in the module (if not yet present)
//
// Signature (mirrors runtime/aes.c):
//   void kagura_aes128_ctr_decrypt(const uint8_t *enc, uint32_t len,
//                                   const uint8_t key[16],
//                                   const uint8_t nonce[8],
//                                   uint8_t *out);
//===----------------------------------------------------------------------===//

// ---- 4.2.5: Zero-after-use runtime helper declaration ---------------------

/// Declare kagura_zero_buf(void *ptr, uint32_t len) — provided by the runtime.
/// Uses volatile semantics so the compiler cannot elide the zeroing.
static FunctionCallee getOrDeclareZeroBuf(Module &M) {
  LLVMContext &Ctx = M.getContext();
  auto *VoidTy  = Type::getVoidTy(Ctx);
  auto *PtrTy   = PointerType::getUnqual(Ctx);
  auto *Int32Ty = Type::getInt32Ty(Ctx);
  auto *FTy     = FunctionType::get(VoidTy, {PtrTy, Int32Ty}, false);
  return M.getOrInsertFunction("kagura_zero_buf", FTy);
}

// ---- AES decrypt runtime declaration -------------------------------------

static FunctionCallee getOrDeclareRuntimeDecrypt(Module &M) {
  LLVMContext &Ctx = M.getContext();
  auto *Int32Ty = Type::getInt32Ty(Ctx);
  auto *VoidTy  = Type::getVoidTy(Ctx);
  auto *PtrTy   = PointerType::getUnqual(Ctx);

  auto *FTy = FunctionType::get(VoidTy,
                                {PtrTy, Int32Ty, PtrTy, PtrTy, PtrTy},
                                /*isVarArg=*/false);
  return M.getOrInsertFunction("kagura_aes128_ctr_decrypt", FTy);
}

//===----------------------------------------------------------------------===//
// Per-string decryption stub
//===----------------------------------------------------------------------===//

static Function *buildAESDecryptStub(Module &M,
                                      GlobalVariable *EncGV,
                                      GlobalVariable *KeyGV,
                                      GlobalVariable *NonceGV,
                                      GlobalVariable *OutBuf,
                                      uint64_t Len,
                                      StringRef Suffix,
                                      FunctionCallee RuntimeDecrypt) {
  LLVMContext &Ctx = M.getContext();
  IRBuilder<> B(Ctx);

  auto *Int8Ty  = Type::getInt8Ty(Ctx);
  auto *Int32Ty = Type::getInt32Ty(Ctx);
  auto *PtrTy   = PointerType::getUnqual(Ctx);

  // i8* kagura_aesdec_<suffix>()
  auto *FTy = FunctionType::get(PtrTy, false);
  auto *F   = Function::Create(FTy, Function::InternalLinkage,
                               "kagura_aesdec_" + Suffix, M);
  F->addFnAttr(Attribute::NoInline);
  F->addFnAttr(Attribute::NoUnwind);

  auto *Entry = BasicBlock::Create(Ctx, "entry", F);
  B.SetInsertPoint(Entry);

  auto *EncPtr   = B.CreateBitCast(EncGV,  PtrTy, "enc");
  auto *KeyPtr   = B.CreateBitCast(KeyGV,  PtrTy, "key");
  auto *NoncePtr = B.CreateBitCast(NonceGV, PtrTy, "nonce");
  auto *OutPtr   = B.CreateBitCast(OutBuf,  PtrTy, "out");

  B.CreateCall(RuntimeDecrypt,
               {EncPtr,
                ConstantInt::get(Int32Ty, static_cast<uint32_t>(Len)),
                KeyPtr,
                NoncePtr,
                OutPtr});

  B.CreateRet(OutPtr);
  return F;
}

//===----------------------------------------------------------------------===//
// Pass entry point
//===----------------------------------------------------------------------===//

PreservedAnalyses StringEncryptionAESPass::run(Module &M,
                                                ModuleAnalysisManager &) {
  auto Strings = collectStringGlobals(M, /*StrictLinkage=*/true);
  if (Strings.empty())
    return PreservedAnalyses::all();

  LLVMContext &Ctx = M.getContext();
  auto &RNG        = getModulePRNG();
  bool Changed     = false;

  FunctionCallee RuntimeDecrypt = getOrDeclareRuntimeDecrypt(M);

  auto *Int8Ty = Type::getInt8Ty(Ctx);

  for (auto *GV : Strings) {
    auto *CDA = cast<ConstantDataArray>(GV->getInitializer());
    StringRef Raw = CDA->getRawDataValues();
    uint64_t Len  = Raw.size();

    // Generate random key (16 bytes) and nonce (8 bytes)
    uint8_t Key[16];
    uint8_t Nonce[8];
    fillRandomBytes(Key, 16);
    fillRandomBytes(Nonce, 8);

    // AES-128-CTR encrypt at compile time
    const auto *DataPtr = reinterpret_cast<const uint8_t *>(Raw.data());
    std::vector<uint8_t> Encrypted =
        aes::ctrCrypt(DataPtr, static_cast<size_t>(Len), Key, Nonce);

    std::string Suffix = std::to_string(RNG.next32());

    // Emit encrypted blob as a mutable private global
    auto *ArrTy = ArrayType::get(Int8Ty, Len);
    {
      std::vector<Constant *> EncBytes;
      EncBytes.reserve(Len);
      for (uint8_t B : Encrypted)
        EncBytes.push_back(ConstantInt::get(Int8Ty, B));
      auto *EncConst = ConstantArray::get(ArrTy, EncBytes);
      auto *EncGV = new GlobalVariable(M, ArrTy, /*isConstant=*/false,
                                       GlobalValue::PrivateLinkage, EncConst,
                                       "kagura_aesenc_" + Suffix);
      EncGV->setAlignment(GV->getAlign());

      // Emit key and nonce globals using shared utility
      auto *KeyGV = createPrivateByteGlobal(
          M, ArrayRef<uint8_t>(Key, 16), "kagura_aeskey_" + Suffix);
      auto *NonceGV = createPrivateByteGlobal(
          M, ArrayRef<uint8_t>(Nonce, 8), "kagura_aesnonce_" + Suffix);

      // 4.2.5: Static output buffer — declared here so we can zero it later.
      auto *OutArrTy = ArrayType::get(Int8Ty, Len);
      auto *OutBuf   = new GlobalVariable(
          M, OutArrTy, /*isConstant=*/false, GlobalValue::PrivateLinkage,
          ConstantAggregateZero::get(OutArrTy),
          "kagura_aesbuf_" + Suffix);

      // Build per-string decryption stub
      Function *Stub = buildAESDecryptStub(M, EncGV, KeyGV, NonceGV, OutBuf,
                                           Len, Suffix, RuntimeDecrypt);

      // 4.2.5: Declare zero helper once per module.
      FunctionCallee ZeroBuf = getOrDeclareZeroBuf(M);
      auto *Int32Ty = Type::getInt32Ty(Ctx);
      auto *PtrTy   = PointerType::getUnqual(Ctx);

      // Replace uses of the original global.
      // Track all call results so we can insert zero-after-use.
      SmallVector<User *, 8> Users(GV->users());
      for (auto *U : Users) {
        if (auto *CE = dyn_cast<ConstantExpr>(U)) {
          SmallVector<User *, 4> CEUsers(CE->users());
          for (auto *CU : CEUsers) {
            if (auto *Inst = dyn_cast<Instruction>(CU)) {
              IRBuilder<> IB(Inst);
              Value *DecPtr = IB.CreateCall(Stub, {}, "aesdec");
              Value *Replacement = DecPtr;
              if (CE->getType() != DecPtr->getType())
                Replacement = IB.CreateBitCast(DecPtr, CE->getType());
              Inst->replaceUsesOfWith(CE, Replacement);

              // 4.2.5: zero immediately after the instruction that consumed
              // the decrypted pointer (best-effort; covers the common case).
              if (Instruction *NextI = Inst->getNextNode()) {
                IRBuilder<> ZB(NextI);
                ZB.CreateCall(ZeroBuf, {
                    ZB.CreateBitCast(OutBuf, PtrTy),
                    ConstantInt::get(Int32Ty, static_cast<uint32_t>(Len))});
              }
            }
          }
          continue;
        }

        if (auto *Inst = dyn_cast<Instruction>(U)) {
          IRBuilder<> IB(Inst);
          Value *DecPtr = IB.CreateCall(Stub, {}, "aesdec");
          for (unsigned I = 0; I < Inst->getNumOperands(); ++I) {
            if (Inst->getOperand(I)->stripPointerCasts() == GV) {
              Value *Replacement = DecPtr;
              if (Inst->getOperand(I)->getType() != DecPtr->getType())
                Replacement =
                    IB.CreateBitCast(DecPtr, Inst->getOperand(I)->getType());
              Inst->setOperand(I, Replacement);
            }
          }
          // 4.2.5: zero buffer right after the use-site instruction.
          if (Instruction *NextI = Inst->getNextNode()) {
            IRBuilder<> ZB(NextI);
            ZB.CreateCall(ZeroBuf, {
                ZB.CreateBitCast(OutBuf, PtrTy),
                ConstantInt::get(Int32Ty, static_cast<uint32_t>(Len))});
          }
        }
      }

      if (GV->use_empty())
        GV->eraseFromParent();

      Changed = true;
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace kagura
