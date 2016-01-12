add_input_file_links()

add_dumux_test(test_dec1p test_dec1p test_1p.cc
               python ${CMAKE_SOURCE_DIR}/bin/runtest.py
                 --script fuzzy
                 --files ${CMAKE_SOURCE_DIR}/test/references/test_1p-reference.vtu
                         ${CMAKE_CURRENT_BINARY_DIR}/test_1p-00001.vtu
                 --command "${CMAKE_CURRENT_BINARY_DIR}/test_dec1p -ParameterFile ${CMAKE_CURRENT_SOURCE_DIR}/test_1p.input")

add_dumux_test(test_diffusion test_diffusion test_diffusion.cc
               python ${CMAKE_SOURCE_DIR}/bin/runtest.py
                 --script fuzzy
                 --files ${CMAKE_SOURCE_DIR}/test/references/diffusion-reference.vtu
                         ${CMAKE_CURRENT_BINARY_DIR}/mimeticdiffusion-00001.vtu
                 --command "${CMAKE_CURRENT_BINARY_DIR}/test_diffusion 3")

add_dumux_test(test_diffusion3d test_diffusion3d test_diffusion3d.cc
               python ${CMAKE_SOURCE_DIR}/bin/runtest.py
                 --script fuzzy
                 --files ${CMAKE_SOURCE_DIR}/test/references/test_diffusion3d_fv-reference.vtu
                         ${CMAKE_CURRENT_BINARY_DIR}/test_diffusion3d_fv-00001.vtu
                         ${CMAKE_SOURCE_DIR}/test/references/test_diffusion3d_fvmpfal-reference.vtu
                         ${CMAKE_CURRENT_BINARY_DIR}/test_diffusion3d_fvmpfal-00001.vtu
                         ${CMAKE_SOURCE_DIR}/test/references/test_diffusion3d_mimetic-reference.vtu
                         ${CMAKE_CURRENT_BINARY_DIR}/test_diffusion3d_mimetic-00001.vtu
                 --command "${CMAKE_CURRENT_BINARY_DIR}/test_diffusion3d -ParameterFile ${CMAKE_CURRENT_SOURCE_DIR}/test_diffusion3d_reference.input")

#install sources
install(FILES
resultevaluation3d.hh
resultevaluation.hh
test_1p.cc
test_1pproblem.hh
test_1pspatialparams.hh
test_diffusion3d.cc
test_diffusion.cc
test_diffusionproblem3d.hh
test_diffusionproblem.hh
test_diffusionspatialparams3d.hh
test_diffusionspatialparams.hh
DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dumux/test/decoupled/1p)