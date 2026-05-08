llvm_map_components_to_libnames(LLVM_LIBS
  Core
  Support
  Analysis
  TransformUtils
  Passes
)

add_llvm_pass_plugin(KaguraObfuscator
  Plugin.cpp
  StringEncryption.cpp
  ControlFlowFlattening.cpp
  BogusControlFlow.cpp
  Substitution.cpp
  ConstantObfuscation.cpp
  AntiDebug.cpp
  ObjCObfuscation.cpp
  JNIObfuscation.cpp
  Utils.cpp

  DEPENDS
  intrinsics_gen
)

target_include_directories(KaguraObfuscator PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
