# 4.6.11: Bazel Starlark macros for KaguraObfuscator.
#
# Provides:
#   kagura_cc_binary  — cc_binary with kagura plugin injected
#   kagura_cc_library — cc_library with kagura plugin injected
#
# Usage in BUILD:
#   load("@kagura//integration/bazel:kagura.bzl",
#        "kagura_cc_binary", "kagura_cc_library")
#
#   kagura_cc_binary(
#       name = "my_binary",
#       srcs = ["main.cc"],
#       kagura_passes = ["-kagura-fla", "-kagura-sub", "-kagura-str"],
#       kagura_config = "//path/to:policy.json",  # optional
#   )
#
# Prerequisites:
#   - In WORKSPACE:
#       load("@kagura//integration/bazel:repositories.bzl", "kagura_repositories")
#       kagura_repositories()
#   - Set KAGURA_PLUGIN_PATH or define @kagura_prebuilt repository.

def _kagura_copts(passes, config_label):
    """Build the -fpass-plugin and -kagura-* flag list."""
    copts = [
        "-fpass-plugin=$(location @kagura_prebuilt//:libKaguraObfuscator)",
    ]
    copts.extend(passes)
    if config_label:
        copts.append("-kagura-config=$(location {})".format(config_label))
    return copts

def _kagura_deps(extra_deps, config_label):
    deps = list(extra_deps)
    deps.append("@kagura//integration/bazel:kagura_runtime")
    deps.append("@kagura_prebuilt//:libKaguraObfuscator")
    if config_label:
        deps.append(config_label)
    return deps

def kagura_cc_binary(
        name,
        srcs = [],
        hdrs = [],
        deps = [],
        copts = [],
        kagura_passes = None,
        kagura_config = None,
        **kwargs):
    """cc_binary with kagura obfuscation passes enabled.

    Args:
      kagura_passes: list of -kagura-* flag strings to enable.
                     Defaults to ["-kagura-fla", "-kagura-str", "-kagura-sub"].
      kagura_config: optional label for a JSON policy file.
    """
    if kagura_passes == None:
        kagura_passes = ["-kagura-fla", "-kagura-str", "-kagura-sub"]

    native.cc_binary(
        name = name,
        srcs = srcs,
        hdrs = hdrs,
        deps = _kagura_deps(deps, kagura_config),
        copts = copts + _kagura_copts(kagura_passes, kagura_config),
        **kwargs
    )

def kagura_cc_library(
        name,
        srcs = [],
        hdrs = [],
        deps = [],
        copts = [],
        kagura_passes = None,
        kagura_config = None,
        **kwargs):
    """cc_library with kagura obfuscation passes enabled.

    Args:
      kagura_passes: list of -kagura-* flag strings to enable.
      kagura_config: optional label for a JSON policy file.
    """
    if kagura_passes == None:
        kagura_passes = ["-kagura-fla", "-kagura-str", "-kagura-sub"]

    native.cc_library(
        name = name,
        srcs = srcs,
        hdrs = hdrs,
        deps = _kagura_deps(deps, kagura_config),
        copts = copts + _kagura_copts(kagura_passes, kagura_config),
        **kwargs
    )
