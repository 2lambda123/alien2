target_sources(tests
PUBLIC
    AttackerTests.cpp
    CellConnectionTests.cpp
    ConstructorTests.cpp
    DataTransferTests.cpp
    DefenderTests.cpp
    DescriptionHelperTests.cpp
    DetonatorTests.cpp
    InjectorTests.cpp
    IntegrationTestFramework.cpp
    IntegrationTestFramework.h
    LivingStateTransitionTests.cpp
    MuscleTests.cpp
    MutationTests.cpp
    NerveTests.cpp
    NeuronTests.cpp
    SensorTests.cpp
    StatisticsTests.cpp
    Testsuite.cpp
    TransmitterTests.cpp)

target_link_libraries(tests alien_base_lib)
target_link_libraries(tests alien_engine_gpu_kernels_lib)
target_link_libraries(tests alien_engine_impl_lib)
target_link_libraries(tests alien_engine_interface_lib)

target_link_libraries(tests CUDA::cudart_static)
target_link_libraries(tests CUDA::cuda_driver)
target_link_libraries(tests Boost::boost)
target_link_libraries(tests OpenGL::GL OpenGL::GLU)
target_link_libraries(tests GLEW::GLEW)
target_link_libraries(tests glfw)
target_link_libraries(tests glad::glad)
target_link_libraries(tests GTest::GTest GTest::Main)

if (MSVC)
    target_compile_options(tests PRIVATE "/MP")
endif()
