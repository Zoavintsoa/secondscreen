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

    # Legacy Windows prototype (not the shipped native host). The canonical host is
    # native/windows-host/SecondScreenHost and is built by build-native.yml.
    add_executable(SecondScreenLegacyHost
        native/windows/main.cpp
        native/windows/host/HostEngine.cpp
        native/windows/capture/DXGICapture.cpp
        native/windows/encoder/HardwareEncoder.cpp
        native/windows/encoder/NVENCEncoder.cpp
        native/windows/encoder/AMFEncoder.cpp
        native/windows/encoder/QuickSyncEncoder.cpp
        native/windows/network/NetworkServer.cpp
    )

    target_link_libraries(SecondScreenLegacyHost PRIVATE
        d3d11
        dxgi
        dxguid
        ws2_32
    )

    target_include_directories(SecondScreenLegacyHost PRIVATE
        native/windows
        native/windows/vendor/nvidia
        core/protocol
    )

    add_executable(SecondScreenLegacyDriverInstaller
        native/windows/driver-installer/DriverInstaller.cpp
    )

    target_link_libraries(SecondScreenLegacyDriverInstaller PRIVATE
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

add_executable(test_h264_bitstream
    tests/test_h264_bitstream.cpp
)
target_include_directories(test_h264_bitstream PRIVATE core/protocol)
add_test(NAME BitstreamTest COMMAND test_h264_bitstream)

add_executable(test_session_security_perf
    tests/test_session_security_perf.cpp
)
target_include_directories(test_session_security_perf PRIVATE core/protocol)
add_test(NAME SessionSecurityPerfTest COMMAND test_session_security_perf)

# The shared protocol is the intended cross-platform control path. Compile its
# production sources in CI instead of testing only the legacy core/protocol tree.
add_executable(test_shared_control_protocol
    tests/test_shared_control_protocol.cpp
    native/shared-protocol/CONTROL_PROTOCOL.cpp
    native/shared-protocol/CONTROL_SESSION.cpp
)
target_include_directories(test_shared_control_protocol PRIVATE native/shared-protocol)
add_test(NAME SharedControlProtocolTest COMMAND test_shared_control_protocol)
