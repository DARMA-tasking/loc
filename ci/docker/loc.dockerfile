ARG REPO=lifflander1/vt
ARG ARCH=amd64
ARG IMAGE=wf-amd64-ubuntu-22.04-gcc-12-cpp

ARG BASE=${REPO}:${IMAGE}

FROM --platform=${ARCH} ${BASE} AS build

ARG IMAGE
ARG CACHE_ID=${IMAGE}
ARG COMM_REPOSITORY=https://github.com/DARMA-tasking/comm.git
ARG COMM_REV=master
ARG COMM_BOOTSTRAP=ON

RUN --mount=type=cache,id=${CACHE_ID},target=/build/ccache             \
    --mount=type=cache,id=BUILD-${CACHE_ID},target=/build/loc          \
    --mount=type=secret,id=GITHUB_TOKEN,env=GITHUB_TOKEN               \
    --mount=target=/loc,rw                                             \
        /loc/ci/build_cpp.sh /loc /build &&                            \
        /loc/ci/test_cpp.sh /loc /build
