add_input_file_links(FILES params.input)

# isothermal tests
add_executable(test_richards_lens_tpfa EXCLUDE_FROM_ALL main.cc)
target_compile_definitions(test_richards_lens_tpfa PUBLIC TYPETAG=RichardsLensCC)

add_executable(test_richards_lens_box EXCLUDE_FROM_ALL main.cc)
target_compile_definitions(test_richards_lens_box PUBLIC TYPETAG=RichardsLensBox)

dumux_add_test(TARGET test_richards_lens_box
              COMMAND ${CMAKE_SOURCE_DIR}/bin/testing/runtest.py
              CMD_ARGS --script fuzzy
                       --files ${CMAKE_SOURCE_DIR}/test/references/test_richards_lens_box-reference.vtu
                               ${CMAKE_CURRENT_BINARY_DIR}/test_richards_lens_box-00007.vtu
                       --command "${CMAKE_CURRENT_BINARY_DIR}/test_richards_lens_box params.input -Problem.Name test_richards_lens_box")

dumux_add_test(TARGET test_richards_lens_tpfa
              COMMAND ${CMAKE_SOURCE_DIR}/bin/testing/runtest.py
              CMD_ARGS --script fuzzy
                       --files ${CMAKE_SOURCE_DIR}/test/references/test_richards_lens_tpfa-reference.vtu
                               ${CMAKE_CURRENT_BINARY_DIR}/test_richards_lens_tpfa-00007.vtu
                       --command "${CMAKE_CURRENT_BINARY_DIR}/test_richards_lens_tpfa params.input -Problem.Name test_richards_lens_tpfa")

dumux_add_test(NAME test_richards_lens_box_parallel_yasp
              TARGET test_richards_lens_box
              CMAKE_GUARD MPI_FOUND
              COMMAND ${CMAKE_SOURCE_DIR}/bin/testing/runtest.py
              CMD_ARGS --script fuzzy --zeroThreshold {"process rank":100}
                       --files ${CMAKE_SOURCE_DIR}/test/references/test_richards_lens_box_parallel-reference.vtu
                               ${CMAKE_CURRENT_BINARY_DIR}/s0002-p0000-test_richards_lens_box_parallel_yasp-00007.vtu
                               ${CMAKE_SOURCE_DIR}/test/references/test_richards_lens_box-reference.vtu
                               ${CMAKE_CURRENT_BINARY_DIR}/s0002-test_richards_lens_box_parallel_yasp-00007.pvtu
                       --command "${MPIEXEC} -np 2 ${CMAKE_CURRENT_BINARY_DIR}/test_richards_lens_box params.input -Problem.Name test_richards_lens_box_parallel_yasp -Grid.Overlap 0")

dumux_add_test(NAME test_richards_lens_tpfa_parallel_yasp
              TARGET test_richards_lens_tpfa
              CMAKE_GUARD MPI_FOUND
              COMMAND ${CMAKE_SOURCE_DIR}/bin/testing/runtest.py
              CMD_ARGS --script fuzzy --zeroThreshold {"process rank":100}
                      --files ${CMAKE_SOURCE_DIR}/test/references/test_richards_lens_tpfa_parallel-reference.vtu
                              ${CMAKE_CURRENT_BINARY_DIR}/s0002-p0000-test_richards_lens_tpfa_parallel_yasp-00007.vtu
                              ${CMAKE_SOURCE_DIR}/test/references/test_richards_lens_tpfa-reference.vtu
                              ${CMAKE_CURRENT_BINARY_DIR}/s0002-test_richards_lens_tpfa_parallel_yasp-00007.pvtu
                      --command "${MPIEXEC} -np 2 ${CMAKE_CURRENT_BINARY_DIR}/test_richards_lens_tpfa params.input -Problem.Name test_richards_lens_tpfa_parallel_yasp -Grid.Overlap 1")

# TODO: there is a small difference because of the precision loss when writing and reading to and from vtk with single precision
# This is why we need a bit higher saturation threshold since pc-sw is very sensitive to pressure for saturations close to 0
dumux_add_test(NAME test_richards_lens_tpfa_parallel_yasp_restart
              TARGET test_richards_lens_tpfa
              CMAKE_GUARD MPI_FOUND
              COMMAND ${CMAKE_SOURCE_DIR}/bin/testing/runtest.py
              CMD_ARGS --script fuzzy --zeroThreshold {"process rank":100,"S_liq":1e-3,"water content":1e-3}
                      --files ${CMAKE_SOURCE_DIR}/test/references/test_richards_lens_tpfa_parallel-reference.vtu
                              ${CMAKE_CURRENT_BINARY_DIR}/s0002-p0000-test_richards_lens_tpfa_parallel_yasp_restart-00004.vtu
                              ${CMAKE_SOURCE_DIR}/test/references/test_richards_lens_tpfa-reference.vtu
                              ${CMAKE_CURRENT_BINARY_DIR}/s0002-test_richards_lens_tpfa_parallel_yasp_restart-00004.pvtu
                      --command "${MPIEXEC} -np 2 ${CMAKE_CURRENT_BINARY_DIR}/test_richards_lens_tpfa params.input -Problem.Name test_richards_lens_tpfa_parallel_yasp_restart -Grid.Overlap 1 -Restart.Time 536.797 -Restart.File s0002-test_richards_lens_tpfa_parallel_yasp-00003.pvtu -TimeLoop.DtInitial 408.68")

# the restart test has to run after the test that produces the corresponding vtu file
set_tests_properties(test_richards_lens_tpfa_parallel_yasp_restart PROPERTIES DEPENDS test_richards_lens_tpfa_parallel_yasp)

dumux_add_test(NAME test_richards_lens_box_parallel_ug
              SOURCES main.cc
              CMAKE_GUARD "( MPI_FOUND AND HAVE_UG )"
              COMPILE_DEFINITIONS TYPETAG=RichardsLensBox GRIDTYPE=Dune::UGGrid<2>
              COMMAND ${CMAKE_SOURCE_DIR}/bin/testing/runtest.py
              CMD_ARGS --script fuzzy --zeroThreshold {"process rank":100}
                       --files ${CMAKE_SOURCE_DIR}/test/references/test_richards_lens_box-reference.vtu
                               ${CMAKE_CURRENT_BINARY_DIR}/s0002-test_richards_lens_box_parallel_ug-00007.pvtu
                       --command "${MPIEXEC} -np 2 ${CMAKE_CURRENT_BINARY_DIR}/test_richards_lens_box_parallel_ug params.input -Problem.Name test_richards_lens_box_parallel_ug")

dumux_add_test(NAME test_richards_lens_tpfa_parallel_ug
              SOURCES main.cc
              CMAKE_GUARD "( MPI_FOUND AND HAVE_UG )"
              COMPILE_DEFINITIONS TYPETAG=RichardsLensCC GRIDTYPE=Dune::UGGrid<2>
              COMMAND ${CMAKE_SOURCE_DIR}/bin/testing/runtest.py
              CMD_ARGS --script fuzzy --zeroThreshold {"process rank":100}
                       --files ${CMAKE_SOURCE_DIR}/test/references/test_richards_lens_tpfa-reference.vtu
                               ${CMAKE_CURRENT_BINARY_DIR}/s0002-test_richards_lens_tpfa_parallel_ug-00007.pvtu
                       --command "${MPIEXEC} -np 2 ${CMAKE_CURRENT_BINARY_DIR}/test_richards_lens_tpfa_parallel_ug params.input -Problem.Name test_richards_lens_tpfa_parallel_ug")

dumux_add_test(NAME test_richards_lens_box_parallel_alu
              SOURCES main.cc
              CMAKE_GUARD "( MPI_FOUND AND dune-alugrid_FOUND )"
              COMPILE_DEFINITIONS TYPETAG=RichardsLensBox GRIDTYPE=Dune::ALUGrid<2,2,Dune::cube,Dune::nonconforming>
              COMMAND ${CMAKE_SOURCE_DIR}/bin/testing/runtest.py
              CMD_ARGS --script fuzzy --zeroThreshold {"process rank":100}
                       --files ${CMAKE_SOURCE_DIR}/test/references/test_richards_lens_box-reference.vtu
                               ${CMAKE_CURRENT_BINARY_DIR}/s0002-test_richards_lens_box_parallel_alu-00007.pvtu
                       --command "${MPIEXEC} -np 2 ${CMAKE_CURRENT_BINARY_DIR}/test_richards_lens_box_parallel_alu params.input -Problem.Name test_richards_lens_box_parallel_alu")

dumux_add_test(NAME test_richards_lens_tpfa_parallel_alu
              SOURCES main.cc
              CMAKE_GUARD "( MPI_FOUND AND dune-alugrid_FOUND )"
              COMPILE_DEFINITIONS TYPETAG=RichardsLensCC GRIDTYPE=Dune::ALUGrid<2,2,Dune::cube,Dune::nonconforming>
              COMMAND ${CMAKE_SOURCE_DIR}/bin/testing/runtest.py
              CMD_ARGS --script fuzzy --zeroThreshold {"process rank":100}
                       --files ${CMAKE_SOURCE_DIR}/test/references/test_richards_lens_tpfa-reference.vtu
                               ${CMAKE_CURRENT_BINARY_DIR}/s0002-test_richards_lens_tpfa_parallel_alu-00007.pvtu
                       --command "${MPIEXEC} -np 2 ${CMAKE_CURRENT_BINARY_DIR}/test_richards_lens_tpfa_parallel_alu params.input -Problem.Name test_richards_lens_tpfa_parallel_alu")
