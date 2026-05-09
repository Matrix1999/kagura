llvm_map_components_to_libnames(LLVM_LIBS
  Core
  Support
  Analysis
  TransformUtils
  Passes
)

# ---- Pass implementation sources (everything except the plugin entry point) -
#
# These are built into an OBJECT library so they can be consumed by both the
# MODULE plugin (loaded via -fpass-plugin) and by the standalone kagura-opt
# executable.  Building them once avoids duplicating compile time.
set(KAGURA_IMPL_SOURCES
  # Core / Infrastructure
  Options.cpp
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
)

# ---- OBJECT library ---------------------------------------------------------
#
# KaguraObfuscator is a MODULE_LIBRARY and cannot be linked into other targets.
# We compile all pass implementations into an OBJECT library (KaguraObfuscatorObjects)
# so that both the MODULE and kagura-opt can reference the same compiled objects.
#
# Plugin.cpp (the llvmGetPassPluginInfo export) is intentionally excluded here:
# it is compiled separately by the MODULE target and by kagura-opt, because
# it calls getKaguraPluginInfo() which must be present in the final binary in
# both cases but does not benefit from deduplication.
add_library(KaguraObfuscatorObjects OBJECT ${KAGURA_IMPL_SOURCES})

set_target_properties(KaguraObfuscatorObjects PROPERTIES
  POSITION_INDEPENDENT_CODE ON  # required for inclusion in a MODULE
)

target_include_directories(KaguraObfuscatorObjects PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# ---- 4.1.3 Legacy PM compile definition ------------------------------------
if(KAGURA_LEGACY_PM)
  target_compile_definitions(KaguraObfuscatorObjects PRIVATE KAGURA_LEGACY_PM=1)
endif()

# ---- Pass plugin: MODULE loaded via -fpass-plugin --------------------------
#
# Plugin.cpp is the sole explicit source; it provides the required
# llvmGetPassPluginInfo() export.  All pass implementations are pulled in
# via target_sources() after the target is created, using the generator
# expression $<TARGET_OBJECTS:...>.  Using target_sources() rather than
# the add_llvm_pass_plugin() argument list avoids passing generator
# expressions through LLVM's llvm_check_source_file_list() (which calls
# get_filename_component() and would mishandle the expression).
add_llvm_pass_plugin(KaguraObfuscator
  PARTIAL_SOURCES_INTENDED
  Plugin.cpp
  DEPENDS intrinsics_gen
)

# Pull in all pass implementations from the OBJECT library.
target_sources(KaguraObfuscator PRIVATE
  $<TARGET_OBJECTS:KaguraObfuscatorObjects>
)

target_include_directories(KaguraObfuscator PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# Export the object library target name so tools/CMakeLists.txt can use it.
set(KAGURA_OBJECTS_TARGET KaguraObfuscatorObjects CACHE INTERNAL
    "OBJECT library containing all kagura pass implementations (excluding Plugin.cpp)")

# ---- 4.6.7 Incremental build: precompiled header ---------------------------
if(KAGURA_PCH)
  target_precompile_headers(KaguraObfuscatorObjects PRIVATE
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
  set_target_properties(KaguraObfuscatorObjects PROPERTIES UNITY_BUILD ON)
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
