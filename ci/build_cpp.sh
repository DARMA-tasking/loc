#!/usr/bin/env bash

set -euo pipefail

usage() {
    cat >&2 <<EOF
Usage: $0 <source-dir> <build-root> [comm-prefix]

Build loc in <build-root>/loc.

The optional comm prefix may be either the comm installation root or the
directory containing commConfig.cmake. It can also be supplied with
COMM_PREFIX or CMAKE_PREFIX_PATH. If none is available, the script builds
COMM_REV from COMM_REPOSITORY in its dependency cache.
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

bootstrap_comm() {
    local dependency_root=${loc_build}/_deps
    local comm_source=${dependency_root}/comm-src
    local comm_build=${dependency_root}/comm
    local comm_install=${dependency_root}/comm-install
    local comm_repository=${COMM_REPOSITORY:-https://github.com/DARMA-tasking/comm.git}
    local comm_revision=${COMM_REV:-master}

    if [[ -f "${comm_install}/cmake/commConfig.cmake" ]]; then
        comm_prefix=${comm_install}/cmake
        return
    fi

    if ! command -v git >/dev/null 2>&1; then
        echo "git is required to bootstrap comm but was not found in PATH" >&2
        exit 2
    fi

    mkdir -p "${dependency_root}"

    if [[ ! -f "${comm_source}/CMakeLists.txt" ]]; then
        if [[ -e "${comm_source}" ]]; then
            echo "${comm_source} exists but is not a comm source tree" >&2
            exit 2
        fi

        echo "=== cloning comm (${comm_revision}) ===" >&2
        git clone \
            --branch "${comm_revision}" \
            --depth 1 \
            "${comm_repository}" \
            "${comm_source}"
    fi

    echo "=== building comm dependency ===" >&2
    "${comm_source}/ci/build_cpp.sh" "${comm_source}" "${dependency_root}"

    echo "=== installing comm dependency ===" >&2
    cmake --install "${comm_build}" --prefix "${comm_install}"
    comm_prefix=${comm_install}/cmake
}

# A docs-only build parses headers and does not need comm or MPI.
if [[ "${LOC_DOXYGEN_ENABLED:-0}" != "1" && -z "${comm_prefix}" ]]; then
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

if [[ "${LOC_DOXYGEN_ENABLED:-0}" != "1" &&
      -z "${comm_prefix}" &&
      -z "${CMAKE_PREFIX_PATH:-}" ]]; then
    if [[ "${COMM_BOOTSTRAP:-ON}" == "ON" ]]; then
        bootstrap_comm
    else
        cat >&2 <<EOF
Unable to locate an installed comm package and COMM_BOOTSTRAP is disabled.

Pass its installation prefix as the third argument, set COMM_PREFIX, or set
CMAKE_PREFIX_PATH.
EOF
        exit 2
    fi
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
    -DLOC_DOXYGEN_ENABLED="${LOC_DOXYGEN_ENABLED:-0}"
)

if test "${LOC_DOXYGEN_ENABLED:-0}" -eq 1
then
    cmake_command+=(
        -DLOC_ENABLE_COMM=OFF
        -DLOC_ENABLE_MPI=OFF
    )
fi

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

if test "${LOC_DOXYGEN_ENABLED:-0}" -eq 1
then
    MCSS=${loc_build}/m.css
    GHPAGE=${loc_build}/DARMA-tasking.github.io

    git clone --depth=1 "https://x-access-token:${GITHUB_TOKEN}@github.com/DARMA-tasking/DARMA-tasking.github.io" "${GHPAGE}"
    git clone https://github.com/mosra/m.css "${MCSS}"
    git -C "${MCSS}" checkout 699abdd5
    "$MCSS/documentation/doxygen.py" "${loc_build}/Doxyfile-mcss"

    if test "${GIT_BRANCH:-}" = "11-build-doc"
    then
        CKPT_NAME=loc_docs
        mv docs "$CKPT_NAME"
        cp  -R "$CKPT_NAME" "$GHPAGE"
        cd "$GHPAGE"
        git config --global user.email "jliffla@sandia.gov"
        git config --global user.name "Jonathan Lifflander"
        git add "$CKPT_NAME"
        git commit --allow-empty -m "Update loc_docs (auto-build)"
        git push origin master
    fi
else
    echo "=== compiling loc ===" >&2
    build_command=(cmake --build "${loc_build}" --parallel)
    "${build_command[@]}" 2>&1 | tee "${loc_build}/compilation-output.log"
fi

if command -v ccache >/dev/null 2>&1; then
    echo "=== ccache statistics after build ===" >&2
    ccache --show-stats
fi
