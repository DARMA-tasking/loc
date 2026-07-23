#!/usr/bin/env bash

set -euo pipefail
set -x

if (( $# != 2 )); then
    echo "Usage: $0 <source-dir> <build-root>" >&2
    exit 2
fi

export LOC=$1
export LOC_BUILD=$2/loc

if [[ ! -f "${LOC_BUILD}/CTestTestfile.cmake" ]]; then
    echo "Loc test build not found: ${LOC_BUILD}" >&2
    exit 2
fi

ctest \
    --test-dir "${LOC_BUILD}" \
    --output-on-failure \
    2>&1 | tee "${LOC_BUILD}/cmake-output.log"
