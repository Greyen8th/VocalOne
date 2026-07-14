#!/usr/bin/env bash
#
# build.sh — Automated build script for the VocalOne audio plugin.
#
# Detects/locates JUCE, configures the project with CMake and compiles the
# VST3 / AU / Standalone targets. Works on macOS, Linux and (via Git Bash /
# WSL) Windows.
#
# Usage:
#   ./build.sh [options]
#
# Options:
#   --debug            Build in Debug mode (default: Release)
#   --clean            Remove the build directory before configuring
#   --juce <path>      Path to the JUCE framework (default: ../JUCE)
#   --jobs <n>         Number of parallel build jobs (default: auto-detected)
#   --generator <g>    CMake generator to use (e.g. "Xcode", "Ninja")
#   -h | --help        Show this help text and exit
#
set -euo pipefail

# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="Release"
BUILD_DIR="${SCRIPT_DIR}/build"
JUCE_DIR="${SCRIPT_DIR}/../JUCE"
CLEAN=0
GENERATOR=""
JUCE_REPO="https://github.com/juce-framework/JUCE.git"

# Detect number of CPU cores for parallel build.
if command -v nproc >/dev/null 2>&1; then
    JOBS="$(nproc)"
elif command -v sysctl >/dev/null 2>&1; then
    JOBS="$(sysctl -n hw.ncpu)"
else
    JOBS=4
fi

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
print_help() {
    sed -n '2,25p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug)      BUILD_TYPE="Debug"; shift ;;
        --clean)      CLEAN=1; shift ;;
        --juce)       JUCE_DIR="$2"; shift 2 ;;
        --jobs)       JOBS="$2"; shift 2 ;;
        --generator)  GENERATOR="$2"; shift 2 ;;
        -h|--help)    print_help; exit 0 ;;
        *) echo "Unknown option: $1" >&2; print_help; exit 1 ;;
    esac
done

# ---------------------------------------------------------------------------
# Sanity checks
# ---------------------------------------------------------------------------
if ! command -v cmake >/dev/null 2>&1; then
    echo "ERROR: CMake is not installed or not in PATH." >&2
    echo "  macOS:   brew install cmake" >&2
    echo "  Ubuntu:  sudo apt install cmake" >&2
    echo "  Windows: https://cmake.org/download/" >&2
    exit 1
fi

# Locate / clone JUCE.
if [[ ! -f "${JUCE_DIR}/CMakeLists.txt" ]]; then
    echo ">> JUCE not found at '${JUCE_DIR}'."
    if command -v git >/dev/null 2>&1; then
        echo ">> Cloning JUCE into '${JUCE_DIR}' ..."
        git clone --depth=1 "${JUCE_REPO}" "${JUCE_DIR}"
    else
        echo "ERROR: git is not available to clone JUCE. Install git or pass --juce <path>." >&2
        exit 1
    fi
fi
JUCE_DIR="$(cd "${JUCE_DIR}" && pwd)"   # normalise to absolute path

# ---------------------------------------------------------------------------
# Clean if requested
# ---------------------------------------------------------------------------
if [[ "${CLEAN}" -eq 1 ]]; then
    echo ">> Cleaning build directory: ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
fi

# ---------------------------------------------------------------------------
# Configure
# ---------------------------------------------------------------------------
echo ">> Configuring VocalOne"
echo "   Build type : ${BUILD_TYPE}"
echo "   JUCE       : ${JUCE_DIR}"
echo "   Jobs       : ${JOBS}"

CMAKE_ARGS=(-B "${BUILD_DIR}" -S "${SCRIPT_DIR}"
            -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
            -DJUCE_DIR="${JUCE_DIR}")

if [[ -n "${GENERATOR}" ]]; then
    CMAKE_ARGS+=(-G "${GENERATOR}")
fi

cmake "${CMAKE_ARGS[@]}"

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------
echo ">> Building VocalOne (${BUILD_TYPE})"
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" --parallel "${JOBS}"

# ---------------------------------------------------------------------------
# Report artefacts
# ---------------------------------------------------------------------------
ARTEFACTS="${BUILD_DIR}/VocalOne_artefacts/${BUILD_TYPE}"
[[ -d "${ARTEFACTS}" ]] || ARTEFACTS="${BUILD_DIR}/VocalOne_artefacts"

echo ""
echo "=========================================================="
echo " Build completed successfully."
echo " Artefacts under: ${ARTEFACTS}"
echo "=========================================================="
if [[ -d "${ARTEFACTS}" ]]; then
    find "${ARTEFACTS}" -maxdepth 2 \( -name "*.vst3" -o -name "*.component" -o -name "VocalOne" -o -name "VocalOne.exe" \) -print 2>/dev/null || true
fi
