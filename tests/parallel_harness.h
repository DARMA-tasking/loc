/*
//@HEADER
// *****************************************************************************
//                             parallel_harness.h
//                       DARMA/loc => Location Coordinator
//         Copyright 2019-2024 NTESS (see LICENSE for full details)
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_LOC_TESTS_PARALLEL_HARNESS_H
#define INCLUDED_LOC_TESTS_PARALLEL_HARNESS_H

#include <gtest/gtest.h>
#include <mpi.h>

#include <comm/comm/MPI/comm_mpi.h>

namespace loc { namespace tests {

extern int test_argc;
extern char** test_argv;

/**
 * \brief GoogleTest fixture that stands up a comm::CommMPI over MPI_COMM_WORLD.
 *
 * MPI is initialized once (in main); the communicator runs in interop mode so
 * per-test teardown drains outstanding messages without finalizing MPI.
 */
struct ParallelHarness : ::testing::Test {
  void SetUp() override {
    int init = 0;
    MPI_Initialized(&init);
    if (!init) {
      MPI_Init(&test_argc, &test_argv);
    }
    comm.init(test_argc, test_argv, MPI_COMM_WORLD);
  }

  void TearDown() override {
    // Drain any outstanding control messages, then release the communicator.
    while (comm.poll()) {
    }
    comm.finalize();
  }

  /// Pump the communicator until it quiesces (all messages delivered).
  void drain() {
    while (comm.poll()) {
    }
  }

  /// Pump the communicator a bounded number of iterations. Used between phases
  /// of a test where a full termination drain would be awkward to repeat.
  void pump(int iters = 2000) {
    for (int i = 0; i < iters; ++i) {
      comm.poll();
    }
  }

  comm::CommMPI comm;
};

}} /* end namespace loc::tests */

#endif /*INCLUDED_LOC_TESTS_PARALLEL_HARNESS_H*/
