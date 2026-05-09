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
  $<$<BOOL:${KAGURA_LEGACY_PM}>:LegacyPlugin.cpp>

  # Phase 4.1 Infrastructure
  Infrastructure/DWARFControl.cpp
  # Phase 4.6 Build System / DX
  Infrastructure/ConfigLoader.cpp
  Infrastructure/SymbolMap.cpp
  Infrastructure/AuditLog.cpp

  # Phase 4.1.11 RTTI / vtable protection
  CFG/VTableProtection.cpp

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
  Data/EncryptedLookupTable.cpp
  Data/StringEncryption.cpp
  Data/StringEncryptionAES.cpp
  Data/WideStringEncryption.cpp
  Data/MemoryValueObfuscation.cpp
  Data/ConstantObfuscation.cpp
  Data/GlobalEncryption.cpp
  Data/Substitution.cpp
  Data/PointerEncryption.cpp

  # Anti-analysis
  AntiAnalysis/AntiDebug.cpp
  AntiAnalysis/AntiTamper.cpp
  AntiAnalysis/HoneyValue.cpp
  AntiAnalysis/CallIndirection.cpp
  AntiAnalysis/PointerAuth.cpp
  AntiAnalysis/SymbolVisibility.cpp
  AntiAnalysis/Telemetry.cpp
  AntiAnalysis/BasicBlockChecksum.cpp

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

# ---- 4.1.3 Legacy PM compile definition ------------------------------------
if(KAGURA_LEGACY_PM)
  target_compile_definitions(KaguraObfuscator PRIVATE KAGURA_LEGACY_PM=1)
endif()

# ---- 4.6.7 Incremental build: precompiled header ---------------------------
if(KAGURA_PCH)
  target_precompile_headers(KaguraObfuscator PRIVATE
    <llvm/IR/Function.h>
    <llvm/IR/Module.h>
    <llvm/IR/IRBuilder.h>
    <llvm/IR/Instructions.h>
    <llvm/IR/Constants.h>
    <llvm/IR/PassManager.h>
    <llvm/Support/raw_ostream.h>
  )
endif()

# ---- 4.6.7 Incremental build: unity build ----------------------------------
if(KAGURA_UNITY_BUILD)
  set_target_properties(KaguraObfuscator PROPERTIES UNITY_BUILD ON)
  set_source_files_properties(
    CFG/ControlFlowFlattening.cpp CFG/BogusControlFlow.cpp
    CFG/BasicBlockReordering.cpp  CFG/BasicBlockSplitting.cpp
    CFG/DeadCodeInsertion.cpp     CFG/FunctionSplit.cpp
    CFG/IndirectBranch.cpp        CFG/LoopTransform.cpp
    CFG/VTableProtection.cpp
    PROPERTIES UNITY_GROUP "kagura_cfg"
  )
  set_source_files_properties(
    Data/StringEncryption.cpp     Data/StringEncryptionAES.cpp
    Data/WideStringEncryption.cpp Data/MemoryValueObfuscation.cpp
    Data/ConstantObfuscation.cpp  Data/GlobalEncryption.cpp
    Data/Substitution.cpp         Data/PointerEncryption.cpp
    PROPERTIES UNITY_GROUP "kagura_data"
  )
  set_source_files_properties(
    AntiAnalysis/AntiDebug.cpp       AntiAnalysis/AntiTamper.cpp
    AntiAnalysis/HoneyValue.cpp      AntiAnalysis/CallIndirection.cpp
    AntiAnalysis/PointerAuth.cpp     AntiAnalysis/SymbolVisibility.cpp
    AntiAnalysis/Telemetry.cpp       AntiAnalysis/BasicBlockChecksum.cpp
    PROPERTIES UNITY_GROUP "kagura_antianalysis"
  )
endif()
