llvm_map_components_to_libnames(LLVM_LIBS
  Core
  Support
  Analysis
  TransformUtils
  Passes
)

add_llvm_pass_plugin(KaguraObfuscator
  PARTIAL_SOURCES_INTENDED

  # Core / Infrastructure
  Options.cpp
  Plugin.cpp
  Utils.cpp
  ObfuscationMetrics.cpp

  # Phase 4.1 Infrastructure
  Infrastructure/DWARFControl.cpp
  # Phase 4.6 Build System / DX
  Infrastructure/ConfigLoader.cpp
  Infrastructure/SymbolMap.cpp

  # CFG obfuscation
  CFG/ControlFlowFlattening.cpp
  CFG/BogusControlFlow.cpp
  CFG/BasicBlockReordering.cpp
  CFG/BasicBlockSplitting.cpp
  CFG/DeadCodeInsertion.cpp
  CFG/FunctionSplit.cpp
  CFG/IndirectBranch.cpp
  CFG/LoopTransform.cpp

  # Data obfuscation
  Data/StringEncryption.cpp
  Data/StringEncryptionAES.cpp
  Data/WideStringEncryption.cpp
  Data/MemoryValueObfuscation.cpp
  Data/ConstantObfuscation.cpp
  Data/GlobalEncryption.cpp
  Data/Substitution.cpp

  # Anti-analysis
  AntiAnalysis/AntiDebug.cpp
  AntiAnalysis/AntiTamper.cpp
  AntiAnalysis/HoneyValue.cpp
  AntiAnalysis/CallIndirection.cpp
  AntiAnalysis/PointerAuth.cpp
  AntiAnalysis/SymbolVisibility.cpp

  # Platform-specific
  Platform/ObjCObfuscation.cpp
  Platform/JNIObfuscation.cpp
  Platform/VMObfuscation.cpp

  DEPENDS
  intrinsics_gen
)

target_include_directories(KaguraObfuscator PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}
)
