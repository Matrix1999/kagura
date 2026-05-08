//===-- StringEncryptionAES.cpp - AES-128 CTR string encryption -----------===//
//
// For every GlobalVariable that is a string constant referenced from a
// function, this pass:
//   1. Encrypts the bytes with AES-128-CTR using a per-string random key+nonce.
//   2. Stores the encrypted blob, key, and nonce as module globals.
//   3. Replaces uses of the original global with a call to a per-string
//      decryption stub that calls kagura_aes128_ctr_decrypt() at runtime.
//
// Registered as: -kagura-str-aes
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

#include <array>
#include <cstdint>
#include <vector>

using namespace llvm;

namespace kagura {

//===----------------------------------------------------------------------===//
// Compile-time AES-128 implementation (forward cipher only, used to encrypt
// strings during pass execution).  Standard table-based Rijndael.
//===----------------------------------------------------------------------===//

namespace aes {

// Rijndael S-box
static constexpr uint8_t SBox[256] = {
  0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
  0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
  0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
  0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
  0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
  0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
  0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
  0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
  0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
  0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
  0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
  0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
  0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
  0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
  0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
  0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16,
};

// Round constants (Rcon) for key schedule
static constexpr uint8_t Rcon[11] = {
  0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36,
};

// GF(2^8) multiply by 2 (xtime)
static inline uint8_t xtime(uint8_t x) {
  return static_cast<uint8_t>((x << 1) ^ ((x & 0x80) ? 0x1b : 0x00));
}

// GF(2^8) multiply
static inline uint8_t gmul(uint8_t a, uint8_t b) {
  uint8_t p = 0;
  for (int i = 0; i < 8; ++i) {
    if (b & 1) p ^= a;
    bool hi = (a & 0x80) != 0;
    a = static_cast<uint8_t>(a << 1);
    if (hi) a ^= 0x1b;
    b >>= 1;
  }
  return p;
}

using Block = std::array<uint8_t, 16>;
using RoundKeys = std::array<Block, 11>;

// AES-128 key expansion
static RoundKeys expandKey(const uint8_t key[16]) {
  RoundKeys rk;
  // Round 0: copy the key itself
  for (int i = 0; i < 16; ++i)
    rk[0][i] = key[i];

  for (int r = 1; r <= 10; ++r) {
    // w[r*4+0]
    uint8_t t0 = SBox[rk[r-1][13]] ^ Rcon[r];
    uint8_t t1 = SBox[rk[r-1][14]];
    uint8_t t2 = SBox[rk[r-1][15]];
    uint8_t t3 = SBox[rk[r-1][12]];

    rk[r][0]  = rk[r-1][0]  ^ t0;
    rk[r][1]  = rk[r-1][1]  ^ t1;
    rk[r][2]  = rk[r-1][2]  ^ t2;
    rk[r][3]  = rk[r-1][3]  ^ t3;
    // w[r*4+1..3]: each word XOR'd with previous round word
    for (int c = 1; c < 4; ++c) {
      rk[r][c*4+0] = rk[r-1][c*4+0] ^ rk[r][c*4-4];
      rk[r][c*4+1] = rk[r-1][c*4+1] ^ rk[r][c*4-3];
      rk[r][c*4+2] = rk[r-1][c*4+2] ^ rk[r][c*4-2];
      rk[r][c*4+3] = rk[r-1][c*4+3] ^ rk[r][c*4-1];
    }
  }
  return rk;
}

// AES-128 forward cipher (single 16-byte block)
static Block encryptBlock(Block in, const RoundKeys &rk) {
  Block s = in;

  // AddRoundKey — round 0
  for (int i = 0; i < 16; ++i) s[i] ^= rk[0][i];

  // Rounds 1-9: SubBytes, ShiftRows, MixColumns, AddRoundKey
  for (int r = 1; r <= 9; ++r) {
    // SubBytes
    for (int i = 0; i < 16; ++i) s[i] = SBox[s[i]];

    // ShiftRows (row-major, column-first layout: state[row][col] = s[col*4+row])
    // Row 1: shift left 1
    uint8_t tmp = s[1];
    s[1] = s[5]; s[5] = s[9]; s[9] = s[13]; s[13] = tmp;
    // Row 2: shift left 2
    std::swap(s[2], s[10]); std::swap(s[6], s[14]);
    // Row 3: shift left 3 (= shift right 1)
    tmp = s[15]; s[15] = s[11]; s[11] = s[7]; s[7] = s[3]; s[3] = tmp;

    // MixColumns
    for (int c = 0; c < 4; ++c) {
      uint8_t a0 = s[c*4+0], a1 = s[c*4+1], a2 = s[c*4+2], a3 = s[c*4+3];
      s[c*4+0] = gmul(0x02,a0)^gmul(0x03,a1)^a2^a3;
      s[c*4+1] = a0^gmul(0x02,a1)^gmul(0x03,a2)^a3;
      s[c*4+2] = a0^a1^gmul(0x02,a2)^gmul(0x03,a3);
      s[c*4+3] = gmul(0x03,a0)^a1^a2^gmul(0x02,a3);
    }

    // AddRoundKey
    for (int i = 0; i < 16; ++i) s[i] ^= rk[r][i];
  }

  // Round 10: SubBytes, ShiftRows, AddRoundKey (no MixColumns)
  for (int i = 0; i < 16; ++i) s[i] = SBox[s[i]];
  // ShiftRows
  uint8_t tmp = s[1];
  s[1] = s[5]; s[5] = s[9]; s[9] = s[13]; s[13] = tmp;
  std::swap(s[2], s[10]); std::swap(s[6], s[14]);
  tmp = s[15]; s[15] = s[11]; s[11] = s[7]; s[7] = s[3]; s[3] = tmp;
  // AddRoundKey
  for (int i = 0; i < 16; ++i) s[i] ^= rk[10][i];

  return s;
}

// AES-128 CTR encrypt/decrypt (symmetric): XOR plaintext with keystream.
// Counter block layout: nonce[0..7] || counter[0..7] (big-endian counter).
static std::vector<uint8_t> ctrCrypt(const uint8_t *data, size_t len,
                                     const uint8_t key[16],
                                     const uint8_t nonce[8]) {
  RoundKeys rk = expandKey(key);
  std::vector<uint8_t> out(len);

  for (size_t offset = 0; offset < len; offset += 16) {
    uint64_t counter = static_cast<uint64_t>(offset / 16);
    // Build counter block: nonce || counter (big-endian)
    Block ctrBlock;
    for (int i = 0; i < 8; ++i) ctrBlock[i] = nonce[i];
    for (int i = 0; i < 8; ++i)
      ctrBlock[8 + i] = static_cast<uint8_t>((counter >> ((7 - i) * 8)) & 0xFF);

    Block ks = encryptBlock(ctrBlock, rk);

    size_t blockLen = std::min(static_cast<size_t>(16), len - offset);
    for (size_t j = 0; j < blockLen; ++j)
      out[offset + j] = data[offset + j] ^ ks[j];
  }
  return out;
}

} // namespace aes

//===----------------------------------------------------------------------===//
// String global collection (same policy as StringEncryption.cpp)
//===----------------------------------------------------------------------===//

static std::vector<GlobalVariable *> collectStringGlobals(Module &M) {
  std::vector<GlobalVariable *> Result;
  for (auto &GV : M.globals()) {
    if (!GV.isConstant() || !GV.hasInitializer())
      continue;
    // Only private/internal linkage
    if (GV.getLinkage() != GlobalValue::PrivateLinkage &&
        GV.getLinkage() != GlobalValue::InternalLinkage)
      continue;
    auto *CDA = dyn_cast<ConstantDataArray>(GV.getInitializer());
    if (!CDA || !CDA->isString())
      continue;
    StringRef S = CDA->getAsString();
    if (S.size() < 4)
      continue;
    if (S.contains('%') || S.trim().empty())
      continue;
    // Must be used inside a function
    bool UsedInFunction = false;
    for (auto *U : GV.users()) {
      if (isa<Instruction>(U) ||
          (isa<ConstantExpr>(U) && !cast<ConstantExpr>(U)->user_empty())) {
        UsedInFunction = true;
        break;
      }
    }
    if (UsedInFunction)
      Result.push_back(&GV);
  }
  return Result;
}

//===----------------------------------------------------------------------===//
// Helper: declare kagura_aes128_ctr_decrypt in the module (if not yet present)
//
// Signature (mirrors runtime/aes.c):
//   void kagura_aes128_ctr_decrypt(const uint8_t *enc, uint32_t len,
//                                   const uint8_t key[16],
//                                   const uint8_t nonce[8],
//                                   uint8_t *out);
//===----------------------------------------------------------------------===//

static FunctionCallee getOrDeclareRuntimeDecrypt(Module &M) {
  LLVMContext &Ctx = M.getContext();
  auto *Int8Ty  = Type::getInt8Ty(Ctx);
  auto *Int32Ty = Type::getInt32Ty(Ctx);
  auto *VoidTy  = Type::getVoidTy(Ctx);
  auto *PtrTy   = PointerType::getUnqual(Int8Ty);

  // void kagura_aes128_ctr_decrypt(i8*, i32, i8*, i8*, i8*)
  auto *FTy = FunctionType::get(VoidTy,
                                {PtrTy, Int32Ty, PtrTy, PtrTy, PtrTy},
                                /*isVarArg=*/false);
  return M.getOrInsertFunction("kagura_aes128_ctr_decrypt", FTy);
}

//===----------------------------------------------------------------------===//
// Per-string decryption stub
//
// Emits a function:
//   i8* kagura_aesdec_<suffix>()
//
// The stub:
//   1. Allocates a stack buffer of the same length as the encrypted data.
//   2. Calls kagura_aes128_ctr_decrypt(encGV, len, keyGV, nonceGV, buf).
//   3. Returns buf.
//
// Marked NoInline so the obfuscation overhead stays out of every call site but
// can still be inlined by later passes that explicitly request it.
//===----------------------------------------------------------------------===//

static Function *buildAESDecryptStub(Module &M,
                                      GlobalVariable *EncGV,
                                      GlobalVariable *KeyGV,
                                      GlobalVariable *NonceGV,
                                      uint64_t Len,
                                      StringRef Suffix,
                                      FunctionCallee RuntimeDecrypt) {
  LLVMContext &Ctx = M.getContext();
  IRBuilder<> B(Ctx);

  auto *Int8Ty  = Type::getInt8Ty(Ctx);
  auto *Int32Ty = Type::getInt32Ty(Ctx);
  auto *PtrTy   = PointerType::getUnqual(Int8Ty);

  // Static output buffer — avoids returning a pointer to a stack frame.
  // Initialized to all-zeros; overwritten on each call to this stub.
  auto *OutArrTy = ArrayType::get(Int8Ty, Len);
  auto *OutBuf = new GlobalVariable(
      M, OutArrTy, /*isConstant=*/false, GlobalValue::PrivateLinkage,
      ConstantAggregateZero::get(OutArrTy),
      "kagura_aesbuf_" + Suffix);

  // i8* kagura_aesdec_<suffix>()
  auto *FTy = FunctionType::get(PtrTy, false);
  auto *F   = Function::Create(FTy, Function::InternalLinkage,
                               "kagura_aesdec_" + Suffix, M);
  F->addFnAttr(Attribute::NoInline);
  F->addFnAttr(Attribute::NoUnwind);

  auto *Entry = BasicBlock::Create(Ctx, "entry", F);
  B.SetInsertPoint(Entry);

  // Cast globals to i8* for the runtime call
  auto *EncPtr   = B.CreateBitCast(EncGV,  PtrTy, "enc");
  auto *KeyPtr   = B.CreateBitCast(KeyGV,  PtrTy, "key");
  auto *NoncePtr = B.CreateBitCast(NonceGV, PtrTy, "nonce");
  auto *OutPtr   = B.CreateBitCast(OutBuf,  PtrTy, "out");

  // Call runtime: kagura_aes128_ctr_decrypt(enc, len, key, nonce, out)
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
  auto Strings = collectStringGlobals(M);
  if (Strings.empty())
    return PreservedAnalyses::all();

  LLVMContext &Ctx = M.getContext();
  auto &RNG        = getModulePRNG();
  bool Changed     = false;

  // Declare the runtime helper once
  FunctionCallee RuntimeDecrypt = getOrDeclareRuntimeDecrypt(M);

  auto *Int8Ty = Type::getInt8Ty(Ctx);

  for (auto *GV : Strings) {
    auto *CDA = cast<ConstantDataArray>(GV->getInitializer());
    StringRef Raw = CDA->getRawDataValues();
    uint64_t Len  = Raw.size();

    // ---- Generate random key (16 bytes) and nonce (8 bytes) ----
    uint8_t Key[16];
    uint8_t Nonce[8];
    {
      uint64_t r0 = RNG.next(), r1 = RNG.next();
      for (int i = 0; i < 8; ++i) {
        Key[i]   = static_cast<uint8_t>((r0 >> (i * 8)) & 0xFF);
        Key[8+i] = static_cast<uint8_t>((r1 >> (i * 8)) & 0xFF);
      }
      uint64_t r2 = RNG.next();
      for (int i = 0; i < 8; ++i)
        Nonce[i] = static_cast<uint8_t>((r2 >> (i * 8)) & 0xFF);
    }

    // ---- AES-128-CTR encrypt at compile time ----
    const auto *DataPtr = reinterpret_cast<const uint8_t *>(Raw.data());
    std::vector<uint8_t> Encrypted =
        aes::ctrCrypt(DataPtr, static_cast<size_t>(Len), Key, Nonce);

    std::string Suffix = std::to_string(RNG.next32());

    // ---- Emit encrypted blob as a mutable private global ----
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

      // ---- Emit key global (16 bytes, constant) ----
      auto *KeyArrTy = ArrayType::get(Int8Ty, 16);
      std::vector<Constant *> KeyBytes;
      for (uint8_t B : Key)
        KeyBytes.push_back(ConstantInt::get(Int8Ty, B));
      auto *KeyConst = ConstantArray::get(KeyArrTy, KeyBytes);
      auto *KeyGV = new GlobalVariable(M, KeyArrTy, /*isConstant=*/true,
                                       GlobalValue::PrivateLinkage, KeyConst,
                                       "kagura_aeskey_" + Suffix);

      // ---- Emit nonce global (8 bytes, constant) ----
      auto *NonceArrTy = ArrayType::get(Int8Ty, 8);
      std::vector<Constant *> NonceBytes;
      for (uint8_t B : Nonce)
        NonceBytes.push_back(ConstantInt::get(Int8Ty, B));
      auto *NonceConst = ConstantArray::get(NonceArrTy, NonceBytes);
      auto *NonceGV = new GlobalVariable(M, NonceArrTy, /*isConstant=*/true,
                                         GlobalValue::PrivateLinkage, NonceConst,
                                         "kagura_aesnonce_" + Suffix);

      // ---- Build per-string decryption stub ----
      Function *Stub = buildAESDecryptStub(M, EncGV, KeyGV, NonceGV,
                                           Len, Suffix, RuntimeDecrypt);

      // ---- Replace uses of the original global ----
      // For each use site: insert a call to the stub and wire the returned
      // pointer in place of the original global reference.
      SmallVector<User *, 8> Users(GV->users());
      for (auto *U : Users) {
        if (auto *CE = dyn_cast<ConstantExpr>(U)) {
          // ConstantExpr users: replace with a pointer to the encrypted global.
          // The decryption call will be inserted at the instruction level for
          // the ConstantExpr's own users.
          SmallVector<User *, 4> CEUsers(CE->users());
          for (auto *CU : CEUsers) {
            if (auto *Inst = dyn_cast<Instruction>(CU)) {
              IRBuilder<> IB(Inst);
              Value *DecPtr = IB.CreateCall(Stub, {}, "aesdec");
              // Replace the ConstantExpr operand with the decrypted pointer,
              // cast to the expected type if needed.
              Value *Replacement = DecPtr;
              if (CE->getType() != DecPtr->getType())
                Replacement = IB.CreateBitCast(DecPtr, CE->getType());
              Inst->replaceUsesOfWith(CE, Replacement);
            }
          }
          continue;
        }

        if (auto *Inst = dyn_cast<Instruction>(U)) {
          IRBuilder<> IB(Inst);
          Value *DecPtr = IB.CreateCall(Stub, {}, "aesdec");
          // Replace the operand pointing to GV
          for (unsigned I = 0; I < Inst->getNumOperands(); ++I) {
            if (Inst->getOperand(I)->stripPointerCasts() == GV) {
              Value *Replacement = DecPtr;
              if (Inst->getOperand(I)->getType() != DecPtr->getType())
                Replacement =
                    IB.CreateBitCast(DecPtr, Inst->getOperand(I)->getType());
              Inst->setOperand(I, Replacement);
            }
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
