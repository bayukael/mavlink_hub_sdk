cmake_minimum_required(VERSION 3.13)
project(pendarlab_mavlink_hub_sdk VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(NAMESPACE pendarlab)
set(PACKAGE_FILENAME mavlink_hub_sdk)
set(PACKAGE_NAME PendarlabMavlinkHubSdk)

set(MAVLINK_ENDPOINT_VERSION "1.0.0")

find_package(PendarlabMavlinkEndpoint ${MAVLINK_ENDPOINT_VERSION} REQUIRED)

##############################################################################
###############################     Agent     ################################
##############################################################################

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/${PACKAGE_FILENAME}/Version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/${PACKAGE_FILENAME}/Version.h
    @ONLY
)

add_library(MavlinkHubSdk SHARED
  src/agent/Agent.cpp
)
target_include_directories(MavlinkHubSdk PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> 
    $<INSTALL_INTERFACE:include> 
)
target_link_libraries(MavlinkHubSdk
    PUBLIC pendarlab::MavlinkEndpoint
)
add_library(${NAMESPACE}::MavlinkHubSdk ALIAS MavlinkHubSdk)
set_target_properties(MavlinkHubSdk PROPERTIES OUTPUT_NAME mavlink_hub_sdk)

##############################################################################
################################     Test     ################################
##############################################################################

add_executable(MavlinkHubSdkHeader
    test/header_check.cpp
)
target_include_directories(MavlinkHubSdkHeader PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> 
    $<INSTALL_INTERFACE:include> 
)
target_link_libraries(MavlinkHubSdkHeader
    PUBLIC pendarlab::MavlinkEndpoint
)
set_target_properties(MavlinkHubSdkHeader PROPERTIES OUTPUT_NAME mavlink_hub_sdk_header)

##############################################################################
######################     Define Files to Install     #######################
##############################################################################

set(TARGET_EXPORT_NAME MavlinkHubSdkTargets)
set(TARGET_EXPORT_FILENAME mavlink_hub_sdk_targets)
set(PACKAGE_DIRECTORY_DESTINATION lib/cmake/${PACKAGE_NAME})

install(
	TARGETS MavlinkHubSdk
	EXPORT ${TARGET_EXPORT_NAME}
)
install(
	DIRECTORY include/
	DESTINATION include
	PATTERN "*.in" EXCLUDE
)
install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/include/${PACKAGE_FILENAME}/Version.h
	DESTINATION include/${PACKAGE_FILENAME}
)
install(
	EXPORT ${TARGET_EXPORT_NAME}
	FILE ${TARGET_EXPORT_FILENAME}.cmake
	NAMESPACE ${NAMESPACE}::
	DESTINATION ${PACKAGE_DIRECTORY_DESTINATION}
)

##############################################################################
##########################     CMake Packaging     ###########################
##############################################################################

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PACKAGE_FILENAME}-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}Config.cmake
    INSTALL_DESTINATION ${PACKAGE_DIRECTORY_DESTINATION}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# -------------------------------------------------
# 5. Install Config + Version files
# -------------------------------------------------
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}ConfigVersion.cmake
    DESTINATION ${PACKAGE_DIRECTORY_DESTINATION}
)