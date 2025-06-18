cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
project(mqtt_client_interfaces)

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

