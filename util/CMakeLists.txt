add_library(error_sim OBJECT error-sim.c)

add_library(error_sim_shim OBJECT error-sim.c error-sim-shim.c)

if(HAVE_LIBFEC)
    add_library(error_sim_fec OBJECT error-sim.c error-sim-fec.c)
endif()

if(HAVE_SSE)
    add_library(error_sim_sse OBJECT error-sim.c error-sim-sse.c)
endif()
