/*
//@HEADER
// *****************************************************************************
//                                   main.cc
//                       DARMA/loc => Location Coordinator
//         Copyright 2019-2024 NTESS (see LICENSE for full details)
// *****************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>

#if defined LOC_TESTS_USE_MPI
  #include <mpi.h>
#endif

namespace loc { namespace tests {

int test_argc = 0;
char** test_argv = nullptr;

}} /* end namespace loc::tests */

int main(int argc, char** argv) {
  loc::tests::test_argc = argc;
  loc::tests::test_argv = argv;

  ::testing::InitGoogleTest(&argc, argv);
  auto const ret = RUN_ALL_TESTS();

#if defined LOC_TESTS_USE_MPI
  int init = 0;
  MPI_Initialized(&init);
  if (init) {
    MPI_Finalize();
  }
#endif

  return ret;
}
