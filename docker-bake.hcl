variable "REPO" {
  default = "lifflander1/vt"
}

variable "GIT_BRANCH" {}

function "arch" {
  params = [item]
  result = lookup(item, "arch", "amd64")
}

function "loc_docs" {
  params = [item]
  result = lookup(item, "loc_docs", "0")
}

function "variant" {
  params = [item]
  result = lookup(item, "variant", "")
}

function "target_suffix" {
  params = [item]
  result = variant(item) == "" ? "" : "-${variant(item)}"
}

target "loc-build" {
  target = "build"
  context = "."
  dockerfile = "ci/docker/loc.dockerfile"

  platforms = [
    "linux/amd64",
    # "linux/arm64"
  ]
  ulimits = [
    "core=0"
  ]

  secret = ["id=GITHUB_TOKEN,env=GITHUB_TOKEN"]
}

target "loc-build-all" {
  name = "loc-build-${replace(item.image, ".", "-")}${target_suffix(item)}"
  inherits = ["loc-build"]
  tags = ["${REPO}:loc-${item.image}"]

  args = {
    ARCH = arch(item)
    GIT_BRANCH = "${GIT_BRANCH}"
    IMAGE = "wf-${item.image}"
    REPO = REPO
    LOC_DOXYGEN_ENABLED = loc_docs(item)
  }

  # to get the list of available images from DARMA-tasking/workflows:
  # workflows > docker buildx bake --print build-all | grep "lifflander1/vt:"
  matrix = {
    item = [
      {
        image = "amd64-alpine-3.16-clang-cpp"
      },
      {
        image = "amd64-ubuntu-22.04-clang-12-cpp"
      },
      {
        image = "amd64-ubuntu-22.04-clang-13-cpp"
      },
      {
        image = "amd64-ubuntu-22.04-clang-14-cpp"
      },
      {
        image = "amd64-ubuntu-22.04-clang-15-cpp"
      },
      {
        image = "amd64-ubuntu-22.04-gcc-11-cpp"
      },
      {
        image = "amd64-ubuntu-22.04-gcc-12-cpp"
      },
      {
        image = "amd64-ubuntu-20.04-gcc-9-cpp"
      },
      {
        image = "amd64-ubuntu-20.04-gcc-9-cpp"
        loc_docs = 1
        variant = "docs"
      },
      {
        image = "amd64-ubuntu-22.04-gcc-12-vtk-cpp"
      },
      {
        image = "amd64-ubuntu-22.04-gcc-12-zoltan-cpp"
      },
      {
        image = "amd64-ubuntu-24.04-clang-16-cpp"
      },
      {
        image = "amd64-ubuntu-24.04-clang-16-vtk-cpp"
      },
      {
        image = "amd64-ubuntu-24.04-clang-16-zoltan-cpp"
      },
      {
        image = "amd64-ubuntu-24.04-clang-17-cpp"
      },
      {
        image = "amd64-ubuntu-24.04-clang-18-cpp"
      },
      {
        image = "amd64-ubuntu-24.04-gcc-13-cpp"
      },
      {
        image = "amd64-ubuntu-24.04-gcc-14-cpp"
      }
    ]
  }
}
