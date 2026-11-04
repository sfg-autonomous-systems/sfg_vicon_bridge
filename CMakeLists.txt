cmake_minimum_required(VERSION 3.8)
project(sfg_vicon_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ########################################################################################################
# Find dependencies.                                                                                     #
# ########################################################################################################
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sfg_utils REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(tf2_ros REQUIRED)

include(FetchContent)

FetchContent_Declare(
    vicon_datastream_sdk
    GIT_REPOSITORY https://github.com/sfg-autonomous-systems/sfg_vicon_datastream_sdk.git
    GIT_TAG 5aa21b4f493b88349b7c76e3761799d5b0a6223d
)
FetchContent_MakeAvailable(vicon_datastream_sdk)

# ########################################################################################################
# Add nodes and libraries.                                                                               #
# ########################################################################################################
sfg_utils__add_component("vicon_bridge" "${PROJECT_NAME}::ViconBridge"
    SOURCES
    "src/vicon_bridge_component.cpp"
    "src/vicon_bridge.cpp"
    AMENT_DEPENDENCIES
    rclcpp
    rclcpp_components
    sfg_utils
    tf2_geometry_msgs
    tf2_ros
    SYSTEM_DEPENDENCIES
    PRIVATE
    ViconDataStreamSDK_CPP
)

if(BUILD_TESTING)
    find_package(ament_lint_auto REQUIRED)

    # The following line skips the linter which checks for copyrights.
    # Comment the line when a copyright and license is added to all source files.
    set(ament_cmake_copyright_FOUND TRUE)

    # The following line skips cpplint (only works in a git repo).
    # Comment the line when this package is in a git repo and when
    # a copyright and license is added to all source files.
    set(ament_cmake_cpplint_FOUND TRUE)
    ament_lint_auto_find_test_dependencies()
endif()

ament_package()