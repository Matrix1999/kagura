llvm_map_components_to_libnames(LLVM_LIBS
  Core
  Support
  Analysis
  TransformUtils
  Passes
)

add_llvm_pass_plugin(KaguraObfuscator
  PARTIAL_SOURCES_INTENDED
  Plugin.cpp
  StringEncryption.cpp
  StringEncryptionAES.cpp
  AntiTamper.cpp
  ControlFlowFlattening.cpp
  BogusControlFlow.cpp
  Substitution.cpp
  ConstantObfuscation.cpp
  ObfuscationMetrics.cpp
  AntiDebug.cpp
  ObjCObfuscation.cpp
  JNIObfuscation.cpp
  VMObfuscation.cpp
  FunctionSplit.cpp
  IndirectBranch.cpp
  CallIndirection.cpp
  PointerAuth.cpp
  GlobalEncryption.cpp
  LoopTransform.cpp
  BasicBlockReordering.cpp
  BasicBlockSplitting.cpp
  DeadCodeInsertion.cpp
  SymbolVisibility.cpp
  Utils.cpp

  DEPENDS
  intrinsics_gen
)

target_include_directories(KaguraObfuscator PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
