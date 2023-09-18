cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(mqtt_client)

find_package(ros_environment REQUIRED QUIET)
set(ROS_VERSION $ENV{ROS_VERSION})

## Compile as C++17
add_compile_options(-std=c++17)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# === ROS2 (AMENT) =============================================================
if(${ROS_VERSION} EQUAL 2)

  find_package(ament_cmake REQUIRED)

  find_package(fmt REQUIRED)
  find_package(mqtt_client_interfaces REQUIRED)
  find_package(rclcpp REQUIRED)
  find_package(rclcpp_components REQUIRED)
  find_package(std_msgs REQUIRED)

  # Paho MQTT C++ apt package doesn't include CMake config
  # find_package(PahoMqttCpp REQUIRED)
  find_library(PahoMqttC_LIBRARY libpaho-mqtt3as.so.1 REQUIRED)
  find_library(PahoMqttCpp_LIBRARY libpaho-mqttpp3.so.1 REQUIRED)

  add_library(${PROJECT_NAME}_lib SHARED src/MqttClient.ros2.cpp)

  rclcpp_components_register_node(${PROJECT_NAME}_lib
    PLUGIN "mqtt_client::MqttClient"
    EXECUTABLE ${PROJECT_NAME}
  )

  target_include_directories(${PROJECT_NAME}_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

  target_link_libraries(${PROJECT_NAME}_lib
    ${PahoMqttC_LIBRARY}
    ${PahoMqttCpp_LIBRARY}
  )

  ament_target_dependencies(${PROJECT_NAME}_lib
    fmt
    mqtt_client_interfaces
    rclcpp
    rclcpp_components
    std_msgs
  )

  install(TARGETS ${PROJECT_NAME}_lib
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
  )

  install(
    DIRECTORY launch
    DESTINATION share/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*ros2*"
  )

  install(
    DIRECTORY config
    DESTINATION share/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*ros2*"
  )

  # if(BUILD_TESTING)
  #   find_package(ament_lint_auto REQUIRED)
  #   # the following line skips the linter which checks for copyrights
  #   # comment the line when a copyright and license is added to all source files
  #   set(ament_cmake_copyright_FOUND TRUE)
  #   # the following line skips cpplint (only works in a git repo)
  #   # comment the line when this package is in a git repo and when
  #   # a copyright and license is added to all source files
  #   set(ament_cmake_cpplint_FOUND TRUE)
  #   ament_lint_auto_find_test_dependencies()
  # endif()

  ament_package()

# === ROS1 (CATKIN) ============================================================
elseif(${ROS_VERSION} EQUAL 1)

  ## Find catkin macros and libraries
  ## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
  ## is used, also find other catkin packages
  find_package(catkin REQUIRED COMPONENTS
    mqtt_client_interfaces
    nodelet
    roscpp
    rosfmt ## For fmt::format.
    std_msgs
    topic_tools
  )

  ## System dependencies are found with CMake's conventions
  find_package(PahoMqttCpp REQUIRED)
  set(PahoMqttCpp_LIBRARIES PahoMqttCpp::paho-mqttpp3)


  ## Uncomment this if the package has a setup.py. This macro ensures
  ## modules and global scripts declared therein get installed
  ## See http://ros.org/doc/api/catkin/html/user_guide/setup_dot_py.html
  # catkin_python_setup()

  ################################################
  ## Declare ROS messages, services and actions ##
  ################################################

  ## To declare and build messages, services or actions from within this
  ## package, follow these steps:
  ## * Let MSG_DEP_SET be the set of packages whose message types you use in
  ##   your messages/services/actions (e.g. std_msgs, actionlib_msgs, ...).
  ## * In the file package.xml:
  ##   * add a build_depend tag for "message_generation"
  ##   * add a build_depend and a exec_depend tag for each package in MSG_DEP_SET
  ##   * If MSG_DEP_SET isn't empty the following dependency has been pulled in
  ##     but can be declared for certainty nonetheless:
  ##     * add a exec_depend tag for "message_runtime"
  ## * In this file (CMakeLists.txt):
  ##   * add "message_generation" and every package in MSG_DEP_SET to
  ##     find_package(catkin REQUIRED COMPONENTS ...)
  ##   * add "message_runtime" and every package in MSG_DEP_SET to
  ##     catkin_package(CATKIN_DEPENDS ...)
  ##   * uncomment the add_*_files sections below as needed
  ##     and list every .msg/.srv/.action file to be processed
  ##   * uncomment the generate_messages entry below
  ##   * add every package in MSG_DEP_SET to generate_messages(DEPENDENCIES ...)

  ## Generate messages in the 'msg' folder
  # add_message_files(
  #   FILES
  #   Message1.msg
  #   Message2.msg
  # )

  ## Generate services in the 'srv' folder
  # add_service_files(
  #   FILES
  #   Service1.srv
  #   Service2.srv
  # )

  ## Generate actions in the 'action' folder
  # add_action_files(
  #   FILES
  #   Action1.action
  #   Action2.action
  # )

  ## Generate added messages and services with any dependencies listed here
  # generate_messages(
  #   DEPENDENCIES
  #   std_msgs
  # )

  ################################################
  ## Declare ROS dynamic reconfigure parameters ##
  ################################################

  ## To declare and build dynamic reconfigure parameters within this
  ## package, follow these steps:
  ## * In the file package.xml:
  ##   * add a build_depend and a exec_depend tag for "dynamic_reconfigure"
  ## * In this file (CMakeLists.txt):
  ##   * add "dynamic_reconfigure" to
  ##     find_package(catkin REQUIRED COMPONENTS ...)
  ##   * uncomment the "generate_dynamic_reconfigure_options" section below
  ##     and list every .cfg file to be processed

  ## Generate dynamic reconfigure parameters in the 'cfg' folder
  # generate_dynamic_reconfigure_options(
  #   cfg/params.cfg
  # )

  ###################################
  ## catkin specific configuration ##
  ###################################
  ## The catkin_package macro generates cmake config files for your package
  ## Declare things to be passed to dependent projects
  ## INCLUDE_DIRS: uncomment this if your package contains header files
  ## LIBRARIES: libraries you create in this project that dependent projects also need
  ## CATKIN_DEPENDS: catkin_packages dependent projects also need
  ## DEPENDS: system dependencies of this project that dependent projects also need
  catkin_package(
    INCLUDE_DIRS include
    LIBRARIES ${PROJECT_NAME}
    CATKIN_DEPENDS
      nodelet
      roscpp
      std_msgs
      topic_tools
    DEPENDS PahoMqttCpp
  )

  ###########
  ## Build ##
  ###########

  ## Specify additional locations of header files
  ## Your package locations should be listed before other locations
  include_directories(
    include
    ${catkin_INCLUDE_DIRS}
  )

  ## Declare a C++ library
  add_library(${PROJECT_NAME}
    src/MqttClient.cpp
  )

  ## Add cmake target dependencies of the library
  ## as an example, code may need to be generated before libraries
  ## either from message generation or dynamic reconfigure
  # add_dependencies(${PROJECT_NAME} ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

  ## Declare a C++ executable
  ## With catkin_make all packages are built within a single CMake context
  ## The recommended prefix ensures that target names across packages don't collide
  # add_executable(${PROJECT_NAME}_node src/hx_testmanager_node.cpp)

  ## Rename C++ executable without prefix
  ## The above recommended prefix causes long target names, the following renames the
  ## target back to the shorter version for ease of user use
  ## e.g. "rosrun someones_pkg node" instead of "rosrun someones_pkg someones_pkg_node"
  # set_target_properties(${PROJECT_NAME}_node PROPERTIES OUTPUT_NAME node PREFIX "")

  ## Add cmake target dependencies of the executable
  ## same as for the library above
  add_dependencies(${PROJECT_NAME} ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

  ## Specify libraries to link a library or executable target against
  target_link_libraries(${PROJECT_NAME}
    ${catkin_LIBRARIES}
    ${PahoMqttCpp_LIBRARIES}
  )

  #############
  ## Install ##
  #############

  # all install targets should use catkin DESTINATION variables
  # See http://ros.org/doc/api/catkin/html/adv_user_guide/variables.html

  ## Mark executable scripts (Python etc.) for installation
  ## in contrast to setup.py, you can choose the destination
  # install(PROGRAMS
  #   scripts/my_python_script
  #   DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
  # )

  ## Mark executables and/or libraries for installation
  install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}
    ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
  )

  ## Mark cpp header files for installation
  install(DIRECTORY include/${PROJECT_NAME}/
    DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
  #   FILES_MATCHING PATTERN "*.h"
  #   PATTERN ".svn" EXCLUDE
  )

  ## Mark other files for installation (e.g. launch and bag files, etc.)
  install(FILES
    nodelet_plugins.xml
    DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
  )
  install(DIRECTORY
    launch
    DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
    PATTERN "*ros2*" EXCLUDE
  )
  install(DIRECTORY
    config
    DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
    PATTERN "*ros2*" EXCLUDE
  )

  #############
  ## Testing ##
  #############

  ## Add gtest based cpp test target and link libraries
  # catkin_add_gtest(${PROJECT_NAME}-test test/test_hx_testmanager.cpp)
  #if(CATKIN_ENABLE_TESTING)
  #  find_package(rostest REQUIRED)
  #  add_rostest_gtest(test_ika_dogm test/ika_dogm.test test/UnitTest.cpp)
  #  catkin_add_gtest(test_ika_dogm test/UnitTest.cpp)
  #  target_link_libraries(test_ika_dogm ${catkin_LIBRARIES} ${PROJECT_NAME}_dogm_creation)
  #endif()
  # if(TARGET ${PROJECT_NAME}-test)
  #   target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME})
  # endif()

  ## Add folders to be run by python nosetests
  # catkin_add_nosetests(test)

endif()
