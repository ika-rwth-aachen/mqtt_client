cmake_minimum_required(VERSION 3.12.0)
project(mqtt_client)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

## Compile as C++17
add_compile_options(-std=c++17)


find_package(PahoMqttCpp REQUIRED)
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(mqtt_client REQUIRED)

set(PahoMqttCpp_LIBRARIES PahoMqttCpp::paho-mqttpp3)

set(INCLUDE_DIRS ${ament_cmake_INCLUDE_DIRS} ${REQUIRED_INCLUDE_DIRS}
  ${PahoMqttCpp_INCLUDE_DIRS} ${std_msgs_INCLUDE_DIRS} ${sensor_msgs_INCLUDE_DIRS} 
  ${rosidl_default_generators_INCLUDE_DIRS}
  ${rclcpp_INCLUDE_DIRS})
set(LIBRARY_DIRS ${ament_cmake_LIBRARIES} ${REQUIRED_LIBRARIES}
  ${PahoMqttCpp_LIBRARIES} ${std_msgs_LIBRARIES} ${sensor_msgs_LIBRARIES} 
  ${rosidl_default_generators_LIBRARIES}
  ${rclcpp_LIBRARIES})
set(LIBS ${ament_cmake_LIBRARIES} ${REQUIRED_LIBRARIES} ${PahoMqttCpp_LIBRARIES}
  ${std_msgs_LIBRARIES} ${sensor_msgs_LIBRARIES} ${rosidl_default_generators_LIBRARIES}
  ${rclcpp_LIBRARIES})


if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()


###########
## Build ##
###########

include_directories(
  include
  ${INCLUDE_DIRS}
)

rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/IsConnected.srv"
)

add_library(mqtt_client_lib src/MqttClient.cpp)
target_link_libraries(mqtt_client_lib ${LIBS})

ament_target_dependencies(mqtt_client_lib rclcpp std_msgs sensor_msgs PahoMqttCpp ${PROJECT_NAME})

add_executable(${PROJECT_NAME}_exe src/MqttClient.cpp)

ament_target_dependencies(${PROJECT_NAME}_exe rclcpp std_msgs sensor_msgs PahoMqttCpp ${PROJECT_NAME})

rosidl_target_interfaces(${PROJECT_NAME}_exe
  ${PROJECT_NAME} "rosidl_typesupport_cpp")

#############
## Install ##
#############

install(DIRECTORY 
  include/${PROJECT_NAME}/
  DESTINATION include
)

install(DIRECTORY 
  launch
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY 
  config
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY 
  srv
  DESTINATION share/${PROJECT_NAME}
)

install(FILES
  srv/IsConnected.srv
  DESTINATION srv)

install(TARGETS
  ${PROJECT_NAME}_exe
  DESTINATION lib/${PROJECT_NAME})

ament_export_dependencies(rosidl_default_runtime)

ament_package()