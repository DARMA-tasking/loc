/*
//@HEADER
// *****************************************************************************
//                                 resolve.cc
//                       DARMA/loc => Location Coordinator
//         Copyright 2019-2024 NTESS (see LICENSE for full details)
// *****************************************************************************
//@HEADER
*/

// Minimal example: rank 0 registers an entity; rank 1 resolves its location.
//
//   mpirun -np 2 ./examples/resolve

#include <loc/coordinator.h>

#include <comm/comm/MPI/comm_mpi.h>
#include <fmt/format.h>

#include <mpi.h>

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  comm::CommMPI comm;
  comm.init(argc, argv, MPI_COMM_WORLD);

  // Collectively construct the coordinator (same order on every rank).
  loc::Coordinator<int, comm::CommMPI> coord{comm};

  constexpr loc::NodeType home = 0;
  constexpr int entity = 7;
  auto const me = coord.thisNode();

  // The entity lives on its home rank.
  if (me == home) {
    coord.registerEntity(entity, home);
  }

  // A different rank asks where it is.
  if (me == 1) {
    coord.getLocation(entity, home, [entity](loc::NodeType node) {
      fmt::print("rank 1: entity {} resolved to node {}\n", entity, node);
    });
  }

  // Drive the communicator until all control messages quiesce.
  while (comm.poll()) {
  }

  comm.finalize();
  MPI_Finalize();
  return 0;
}
