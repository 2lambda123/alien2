
add_library(alien_engine_impl_lib
    AccessDataTOCache.cpp
    AccessDataTOCache.h
    DescriptionConverter.cpp
    DescriptionConverter.h
    Definitions.h
    EngineWorker.cpp
    EngineWorker.h
    SimulationControllerImpl.cpp
    SimulationControllerImpl.h)

target_link_libraries(alien_engine_impl_lib alien_base_lib)
target_link_libraries(alien_engine_impl_lib alien_engine_gpu_kernels_lib)

target_link_libraries(alien_engine_impl_lib CUDA::cudart_static)
target_link_libraries(alien_engine_impl_lib Boost::boost)

if (MSVC)
    target_compile_options(alien_engine_impl_lib PRIVATE "/MP")
endif()
