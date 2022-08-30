/*
==============================================================================
MIT License

Copyright 2022 Institute for Automotive Engineering of RWTH Aachen University.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
==============================================================================
*/

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "mqtt_client/MqttClient.h"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include "std_msgs/msg/string.hpp"


namespace mqtt_client{

MqttClient::MqttClient() : Node("mqtt_client", rclcpp::NodeOptions()){

 //constructor
 declare_parameter("broker.host");
 declare_parameter("broker.port");
 declare_parameter("bridge.ros2mqtt.ros_topic");
 declare_parameter("bridge.ros2mqtt.mqtt_topic");
 declare_parameter("bridge.mqtt2ros.mqtt_topic");
 declare_parameter("bridge.mqtt2ros.ros_topic");

//  auto broker_host = get_parameter("broker.host").as_string();
//  auto broker_port = get_parameter("broker.port").as_int();
//  auto ros2mqtt_ros_topic = get_parameter("bridge.ros2mqtt.ros_topic").as_string();
//  auto ros2mqtt_mqtt_topic = get_parameter("bridge.ros2mqtt.mqtt_topic").as_string();
//  auto mqtt2ros_mqtt_topic = get_parameter("bridge.mqtt2ros.mqtt_topic").as_string();
//  auto mqtt2ros_ros_topic = get_parameter("bridge.mqtt2ros.ros_topic").as_string();
 //std::string params[4] = {ros2mqtt_ros_topic, ros2mqtt_mqtt_topic, mqtt2ros_ros_topic, mqtt2ros_mqtt_topic};
 //this->params=params;

 //RCLCPP_INFO(get_logger(), "Nested Int param: %ld", broker_port);
 //RCLCPP_INFO(get_logger(), "Nested String param: %s", ros2mqtt_ros_topic.c_str());



}

// functions
const std::string MqttClient::kRosMsgTypeMqttTopicPrefix =
  "mqtt_client/ros_msg_type/";

const std::string MqttClient::kLatencyRosTopicPrefix = "latencies/";

void MqttClient::loadParameters(){
  
  // load broker parameters from parameter server
  std::string broker_tls_ca_certificate;
  loadParameter("broker/host", broker_config_.host, "localhost");
  loadParameter("broker/port", broker_config_.port, 1883);
  if (loadParameter("broker/user", broker_config_.user)) {
    loadParameter("broker/pass", broker_config_.pass, "");
  }
  if (loadParameter("broker/tls/enabled", broker_config_.tls.enabled, false)) {
    loadParameter("broker/tls/ca_certificate", broker_tls_ca_certificate, "/etc/ssl/certs/ca-certificates.crt");
  }

  // load client parameters from parameter server
  std::string client_buffer_directory, client_tls_certificate, client_tls_key;
  loadParameter("client/id", client_config_.id, "");
  client_config_.buffer.enabled = !client_config_.id.empty();
  if (client_config_.buffer.enabled) {
    loadParameter("client/buffer/size", client_config_.buffer.size, 0);
    loadParameter("client/buffer/directory", client_buffer_directory, "buffer");
  } else {
    RCLCPP_WARN(rclcpp::get_logger("rclcpp"), "Client buffer can not be enabled when client ID is empty");
  }
  if (loadParameter("client/last_will/topic", client_config_.last_will.topic)) {
    loadParameter("client/last_will/message", client_config_.last_will.message,
                  "offline");
    loadParameter("client/last_will/qos", client_config_.last_will.qos, 0);
    loadParameter("client/last_will/retained",
                  client_config_.last_will.retained, false);
  }
  loadParameter("client/clean_session", client_config_.clean_session, true);
  loadParameter("client/keep_alive_interval",
                client_config_.keep_alive_interval, 60.0);
  loadParameter("client/max_inflight", client_config_.max_inflight, 65535);
  if (broker_config_.tls.enabled) {
    if (loadParameter("client/tls/certificate", client_tls_certificate)) {
      loadParameter("client/tls/key", client_tls_key);
      loadParameter("client/tls/password", client_config_.tls.password);
    }
  }

  // resolve filepaths
  broker_config_.tls.ca_certificate = resolvePath(broker_tls_ca_certificate);
  client_config_.buffer.directory = resolvePath(client_buffer_directory);
  client_config_.tls.certificate = resolvePath(client_tls_certificate);
  client_config_.tls.key = resolvePath(client_tls_key);

  try{
    auto ros2mqtt_ros_topic = get_parameter("bridge.ros2mqtt.ros_topic").as_string();
    auto ros2mqtt_mqtt_topic = get_parameter("bridge.ros2mqtt.mqtt_topic").as_string();
    auto mqtt2ros_mqtt_topic = get_parameter("bridge.mqtt2ros.mqtt_topic").as_string();
    auto mqtt2ros_ros_topic = get_parameter("bridge.mqtt2ros.ros_topic").as_string();
    std::string params[4] = {ros2mqtt_ros_topic, ros2mqtt_mqtt_topic, mqtt2ros_ros_topic, mqtt2ros_mqtt_topic};
    
    for (int k = 0; k < (int)sizeof(params); k++) {
      if (params[k].find("ros2mqtt_ros_topic") != std::string::npos) {
        std::string& ros_topic = params[k];
        Ros2MqttInterface& ros2mqtt = ros2mqtt_[ros_topic];
        for (int j = 0; j < (int)sizeof(params); j++) {
          if (params[j].find("ros2mqtt_mqtt_topic") != std::string::npos) {
          ros2mqtt.mqtt.topic = std::string(params[j]);
          }
        }

        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Bridging ROS topic '%s' to MQTT topic '%s'", ros_topic.c_str(), ros2mqtt.mqtt.topic.c_str());
      } else {
        RCLCPP_WARN(rclcpp::get_logger("rclcpp"), "Parameter 'params[%d]' is missing subparameter "
            "'ros_topic' or 'mqtt_topic', will be ignored", k);
      } 
    }

    for (int k = 0; k < (int)sizeof(params); k++) {
      if (params[k].find("mqtt2ros_mqtt_topic") != std::string::npos) {
        std::string& mqtt_topic = params[k];
        Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_topic];
        for (int j = 0; j < (int)sizeof(params); j++) {
          if (params[j].find("mqtt2ros_ros_topic") != std::string::npos) {
          mqtt2ros.ros.topic = std::string(params[j]);
          }
        }

        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Bridging MQTT topic '%s' to ROS topic '%s'", mqtt_topic.c_str(), mqtt2ros.ros.topic.c_str());
      } else {
        RCLCPP_WARN(rclcpp::get_logger("rclcpp"), "Parameter 'params[%d]' is missing subparameter "
            "'mqtt_topic' or 'ros_topic', will be ignored", k);
      }
    }

    if (ros2mqtt_.empty() && mqtt2ros_.empty()) {
      RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "No valid ROS-MQTT bridge found in parameter array");
      exit(EXIT_FAILURE);
    }

  } catch(const std::exception& e){
    RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Parameter could not be parsed");
    exit(EXIT_FAILURE);
  }
}

bool MqttClient::loadParameter(const std::string& key, std::string& value) {
  bool found = MqttClient::get_parameter(key, value);
  if (found)
    RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), "Retrieved parameter '%s' = '%s'", key.c_str(), value.c_str());
  
  return found;
}

bool MqttClient::loadParameter(const std::string& key, std::string& value, const std::string& default_value) {
  bool found = MqttClient::get_parameter_or(key, value, default_value);
  if (!found)
    RCLCPP_WARN(rclcpp::get_logger("rclcpp"), "Parameter '%s' not set, defaulting to '%s'", key.c_str(), default_value.c_str());

  if (found)
    RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), "Retrieved parameter '%s' = '%s'", key.c_str(), value.c_str());

  return found;
}

rcpputils::fs::path MqttClient::resolvePath(const std::string& path_string) {

  rcpputils::fs::path path(path_string);
  if (path_string.empty()) return path;
  if (!path.is_absolute()) {
    std::string ros_home;
    ros_home = rcpputils::get_env_var("ROS_HOME");
    if (ros_home.empty())
      ros_home = rcpputils::fs::current_path().string();
    path = rcpputils::fs::path(ros_home);
    path.operator/=	(path_string);
  }
  if (!rcpputils::fs::exists(path))
    RCLCPP_WARN(rclcpp::get_logger("rclcpp"), "Requested path '%s' does not exist", path.string().c_str());
  return path;
}


} //Namespace

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<mqtt_client::MqttClient>());
  rclcpp::shutdown();
  return 0;
}

