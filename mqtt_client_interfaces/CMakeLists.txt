cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(mqtt_client_interfaces)

find_package(ros_environment REQUIRED QUIET)
set(ROS_VERSION $ENV{ROS_VERSION})

# === ROS2 (AMENT) =============================================================
if(${ROS_VERSION} EQUAL 2)

  find_package(ament_cmake REQUIRED)
  find_package(rosidl_default_generators REQUIRED)
  find_package(std_msgs REQUIRED)

  rosidl_generate_interfaces(${PROJECT_NAME}
    msg/RosMsgType.msg
    srv/IsConnected.srv
    srv/ros2/NewMqtt2RosBridge.srv
    srv/ros2/NewRos2MqttBridge.srv
    DEPENDENCIES std_msgs
  )

  ament_package()

# === ROS1 (CATKIN) ============================================================
elseif(${ROS_VERSION} EQUAL 1)

  find_package(catkin REQUIRED COMPONENTS
    message_generation
    std_msgs
  )

  add_message_files(
    FILES
    RosMsgType.msg
  )

  add_service_files(
    FILES
    IsConnected.srv
  )

  generate_messages(
    DEPENDENCIES
    std_msgs
  )

  catkin_package(
    CATKIN_DEPENDS
      message_runtime
      std_msgs
  )

endif()
