#!/usr/bin/env bash

set -euo pipefail

usage() {
    cat >&2 <<EOF
Usage: $0 <source-dir> <build-root> [comm-prefix]

Build loc in <build-root>/loc.

The optional comm prefix may be either the comm installation root or the
directory containing commConfig.cmake. It can also be supplied with
COMM_PREFIX or CMAKE_PREFIX_PATH.
EOF
}

if (( $# < 2 || $# > 3 )); then
    usage
    exit 2
fi

source_dir=$1
build_root=$2

comm_prefix=${3:-${COMM_PREFIX:-}}
loc_build=${build_root}/loc
generator=${CMAKE_GENERATOR:-Ninja}

if [[ ! -f "${source_dir}/CMakeLists.txt" ]]; then
    echo "Loc source directory not found: ${source_dir}" >&2
    exit 2
fi

# In a sibling checkout, use comm's local installation automatically.
if [[ -z "${comm_prefix}" ]]; then
    for candidate in \
        "${source_dir}/../comm/install/cmake" \
        "${source_dir}/../comm/install"
    do
        if [[ -f "${candidate}/commConfig.cmake" ||
              -f "${candidate}/cmake/commConfig.cmake" ]]; then
            comm_prefix=${candidate}
            break
        fi
    done
fi

if [[ -n "${comm_prefix}" ]]; then
    if [[ -f "${comm_prefix}/commConfig.cmake" ]]; then
        comm_config_dir=${comm_prefix}
    elif [[ -f "${comm_prefix}/cmake/commConfig.cmake" ]]; then
        comm_config_dir=${comm_prefix}/cmake
    else
        echo "commConfig.cmake not found below comm prefix: ${comm_prefix}" >&2
        exit 2
    fi
elif [[ -z "${CMAKE_PREFIX_PATH:-}" ]]; then
    cat >&2 <<EOF
Unable to locate an installed comm package.

Pass its installation prefix as the third argument, set COMM_PREFIX, or set
CMAKE_PREFIX_PATH. For example:
  $0 "${source_dir}" "${build_root}" /path/to/comm/install
EOF
    exit 2
fi

if command -v ccache >/dev/null 2>&1; then
    echo "=== ccache statistics before build ===" >&2
    ccache --show-stats
else
    echo "=== ccache not found; compiling without it ===" >&2
fi

mkdir -p "${loc_build}"

cmake_command=(
    cmake
    -S "${source_dir}"
    -B "${loc_build}"
    -G "${generator}"
    -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Debug}"
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    -DBUILD_SHARED_LIBS="${BUILD_SHARED_LIBS:-OFF}"
)

if [[ -n "${comm_config_dir:-}" ]]; then
    # A prefix path, unlike comm_DIR, also lets commConfig.cmake discover its
    # transitive checkpoint package installed alongside it.
    echo "=== using comm package from ${comm_config_dir} ===" >&2
    cmake_command+=("-DCMAKE_PREFIX_PATH=${comm_config_dir}")
fi

if command -v ccache >/dev/null 2>&1; then
    cmake_command+=(-DCMAKE_CXX_COMPILER_LAUNCHER=ccache)
fi

echo "=== configuring loc (${generator}, ${CMAKE_BUILD_TYPE:-Debug}) ===" >&2
"${cmake_command[@]}" 2>&1 | tee "${loc_build}/cmake-configure.log"

echo "=== compiling loc ===" >&2
build_command=(cmake --build "${loc_build}" --parallel)
"${build_command[@]}" 2>&1 | tee "${loc_build}/compilation-output.log"

if command -v ccache >/dev/null 2>&1; then
    echo "=== ccache statistics after build ===" >&2
    ccache --show-stats
fi
