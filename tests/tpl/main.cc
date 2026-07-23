/*
//@HEADER
// *****************************************************************************
//                                tpl/main.cc
//                       DARMA/loc => Location Coordinator
//         Copyright 2019-2024 NTESS (see LICENSE for full details)
// *****************************************************************************
//@HEADER
*/

#include <loc/coordinator.h>

#include <functional>
#include <utility>

namespace {

/**
 * Minimal in-process backend standing in for the adapter supplied by VT.
 *
 * Sending dispatches immediately to the registered coordinator. This is enough
 * to compile and exercise every part of loc's communication contract without
 * including or linking standalone comm or MPI.
 */
struct LocalCommunicator {
  template <typename Instance>
  using HandleType = Instance*;

  int getRank() const { return 0; }

  template <typename Instance>
  HandleType<Instance> registerInstanceCollective(Instance* instance) {
    return instance;
  }

  template <auto Handler, typename Instance, typename... Args>
  void send(
    loc::NodeType /*node*/, Instance* instance, Args&&... args
  ) {
    std::invoke(
      Handler, instance, std::forward<Args>(args)...
    );
  }
};

} /* end anonymous namespace */

int main() {
  LocalCommunicator comm;
  loc::Coordinator<int, LocalCommunicator> coordinator{comm};

  constexpr int entity = 42;
  constexpr loc::NodeType remote_home = 1;

  bool resolved = false;
  loc::NodeType location = loc::no_node;
  coordinator.getLocation(entity, remote_home, [&](loc::NodeType node) {
    resolved = true;
    location = node;
  });

  // The synchronous test backend routes the request and registration through
  // Coordinator's message handlers, including the buffered-response path.
  coordinator.registerEntity(entity, remote_home);

  if (!resolved || location != coordinator.thisNode()) {
    return 1;
  }

  bool checked = false;
  coordinator.entityExists(
    entity, remote_home, [&](bool exists, loc::NodeType node) {
      checked = exists && node == coordinator.thisNode();
    }
  );

  return checked ? 0 : 2;
}
