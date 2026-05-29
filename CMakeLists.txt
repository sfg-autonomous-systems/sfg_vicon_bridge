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

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
    set(SYSTEM_ARCH x86_64)
else()
    message(FATAL_ERROR "Unsupported arch: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(VICON_DATASTREAM_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ViconDataStreamSDK")
set(VICON_DATASTREAM_SDK_LIBRARIES_DIR "${VICON_DATASTREAM_SDK_DIR}/lib/${SYSTEM_ARCH}")
set(VICON_DATASTREAM_SDK_INCLUDE_DIR "${VICON_DATASTREAM_SDK_DIR}/include")

add_library(vicon_datastream_sdk SHARED IMPORTED)
set_target_properties(vicon_datastream_sdk PROPERTIES
    IMPORTED_LOCATION "${VICON_DATASTREAM_SDK_LIBRARIES_DIR}/libViconDataStreamSDK_CPP.so"
    INTERFACE_INCLUDE_DIRECTORIES "${VICON_DATASTREAM_SDK_INCLUDE_DIR}"
)

# ########################################################################################################
# Add nodes and libraries.                                                                               #
# ########################################################################################################
sfg_utils__add_component("vicon_bridge" "${PROJECT_NAME}::ViconBridge"
    SOURCES
    "src/vicon_bridge_component.cpp"
    "src/vicon_bridge.cpp"
    DEPENDENCIES
    rclcpp::rclcpp
    rclcpp_components::component
    sfg_utils::sfg_utils
    ${tf2_geometry_msgs_TARGETS}
    tf2_ros::tf2_ros
    vicon_datastream_sdk
)

# ########################################################################################################
# Add other files/directories.                                                                           #
# ########################################################################################################
file(GLOB VICON_DATASTREAM_SDK_LIBRARIES "${VICON_DATASTREAM_SDK_LIBRARIES_DIR}/*.so*")

install(FILES ${VICON_DATASTREAM_SDK_LIBRARIES}
    DESTINATION lib
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