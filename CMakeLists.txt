cmake_minimum_required(VERSION 3.20)
project(SecondScreen LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/core/protocol
    ${CMAKE_CURRENT_SOURCE_DIR}/native/windows
    ${CMAKE_CURRENT_SOURCE_DIR}/native/windows/vendor/nvidia
)

# Enable testing
enable_testing()

if(WIN32)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _UNICODE UNICODE)

    # Windows SecondScreen Native Host Executable
    add_executable(SecondScreenHost
        native/windows/main.cpp
        native/windows/host/HostEngine.cpp
        native/windows/capture/DXGICapture.cpp
        native/windows/encoder/HardwareEncoder.cpp
        native/windows/encoder/NVENCEncoder.cpp
        native/windows/encoder/AMFEncoder.cpp
        native/windows/encoder/QuickSyncEncoder.cpp
        native/windows/network/NetworkServer.cpp
    )

    target_link_libraries(SecondScreenHost PRIVATE
        d3d11
        dxgi
        dxguid
        ws2_32
    )

    target_include_directories(SecondScreenHost PRIVATE
        native/windows
        native/windows/vendor/nvidia
        core/protocol
    )

    add_executable(SecondScreenDriverInstaller
        native/windows/driver-installer/DriverInstaller.cpp
    )

    target_link_libraries(SecondScreenDriverInstaller PRIVATE
        setupapi
        newdev
    )
endif()

# Cross-platform unit tests
add_executable(test_protocol
    tests/test_protocol.cpp
)
target_include_directories(test_protocol PRIVATE core/protocol)
add_test(NAME ProtocolTest COMMAND test_protocol)

add_executable(test_framing
    tests/test_framing.cpp
)
target_include_directories(test_framing PRIVATE core/protocol)
add_test(NAME FramingTest COMMAND test_framing)

add_executable(test_pipeline
    tests/test_pipeline.cpp
)
target_include_directories(test_pipeline PRIVATE core/protocol native/windows)
add_test(NAME PipelineTest COMMAND test_pipeline)
