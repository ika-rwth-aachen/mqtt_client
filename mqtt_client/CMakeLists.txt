cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(mqtt_client)

## Compile as C++17
add_compile_options(-std=c++17)
link_libraries("$<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:-lstdc++fs>")
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(fmt REQUIRED)
find_package(mqtt_client_interfaces REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(std_msgs REQUIRED)

# Paho MQTT C++ apt package doesn't include CMake config
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

ament_package()

