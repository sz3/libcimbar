add_executable(rs_find_primitive_poly EXCLUDE_FROM_ALL find_rs_primitive_poly.c)
target_link_libraries(rs_find_primitive_poly correct_static)
set(all_tools ${all_tools} rs_find_primitive_poly)

if(HAVE_LIBFEC)
    add_executable(conv_find_libfec_poly EXCLUDE_FROM_ALL find_conv_libfec_poly.c)
    target_link_libraries(conv_find_libfec_poly correct_static fec)
    set(all_tools ${all_tools} conv_find_libfec_poly)
endif()

if(HAVE_SSE)
    add_executable(conv_find_optim_poly EXCLUDE_FROM_ALL find_conv_optim_poly.c $<TARGET_OBJECTS:error_sim_sse>)
    target_link_libraries(conv_find_optim_poly correct_static)
    set(all_tools ${all_tools} conv_find_optim_poly)

    add_executable(conv_find_optim_poly_annealing EXCLUDE_FROM_ALL find_conv_optim_poly_annealing.c $<TARGET_OBJECTS:error_sim_sse>)
    target_link_libraries(conv_find_optim_poly_annealing correct_static)
    set(all_tools ${all_tools} conv_find_optim_poly_annealing)
else()
    add_executable(conv_find_optim_poly EXCLUDE_FROM_ALL find_conv_optim_poly.c $<TARGET_OBJECTS:error_sim>)
    target_link_libraries(conv_find_optim_poly correct_static)
    set(all_tools ${all_tools} conv_find_optim_poly)

    add_executable(conv_find_optim_poly_annealing EXCLUDE_FROM_ALL find_conv_optim_poly_annealing.c $<TARGET_OBJECTS:error_sim>)
    target_link_libraries(conv_find_optim_poly_annealing correct_static)
    set(all_tools ${all_tools} conv_find_optim_poly_annealing)
endif()

add_custom_target(tools DEPENDS ${all_tools})
