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

#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>

#include <mqtt/async_client.h>
#include "rclcpp/rclcpp.hpp"

#include "rclcpp/logger.hpp"
#include "rcutils/logging_macros.h"
#include "rcpputils/filesystem_helper.hpp"
#include "rcpputils/get_env.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_msgs/msg/float64.hpp"


/**
 * Namespace for the mqtt_client package
 */
namespace mqtt_client {


/**
 * ROS Nodelet for sending and receiving ROS messages via MQTT
 *
 * The MqttClient enables connected ROS-based devices or robots to
 * exchange ROS messages via an MQTT broker using the MQTT protocol.
 * This works generically for any ROS message, i.e. there is no need
 * to specify the ROS message type for ROS messages you wish to
 * exchange via the MQTT broker.
 */
class MqttClient : public rclcpp::Node {

public:
  MqttClient();

 protected:

  void loadParameters();

  bool loadParameter(const std::string& key, std::string& value);

  bool loadParameter(const std::string& key, std::string& value, const std::string& default_value);

  template <typename T>
  bool loadParameter(const std::string& key, T& value);

  template <typename T>
  bool loadParameter(const std::string& key, T& value, const T& default_value);

  rcpputils::fs::path resolvePath(const std::string& path_string);

  void ros2mqtt(const std::string& ros_topic);

  void mqtt2ros(mqtt::const_message_ptr mqtt_msg);

  void message_arrived(mqtt::const_message_ptr mqtt_msg); //override

 protected:
  
  struct BrokerConfig {
    std::string host;  ///< broker host
    int port;          ///< broker port
    std::string user;  ///< username
    std::string pass;  ///< password
    struct {
      bool enabled;  ///< whether to connect via SSL/TLS
      rcpputils::fs::path ca_certificate;  ///< public CA certificate trusted by client
    } tls;               ///< SSL/TLS-related variables
  };

  struct ClientConfig {
    std::string id;  ///< client unique ID
    struct {
      bool enabled;                     ///< whether client buffer is enabled
      int size;                         ///< client buffer size
      rcpputils::fs::path directory;  ///< client buffer directory
    } buffer;                           ///< client buffer-related variables
    struct {
      std::string topic;         ///< last-will topic
      std::string message;       ///< last-will message
      int qos;                   ///< last-will QoS value
      bool retained;             ///< whether last-will is retained
    } last_will;                 ///< last-will-related variables
    bool clean_session;          ///< whether client requests clean session
    double keep_alive_interval;  ///< keep-alive interval
    int max_inflight;            ///< maximum number of inflight messages
    struct {
      rcpputils::fs::path certificate;  ///< client certificate
      rcpputils::fs::path key;          ///< client private keyfile
      std::string password;  ///< decryption password for private key
    } tls;                   ///< SSL/TLS-related variables
  };

  struct Ros2MqttInterface {
    struct {
      rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscriber;
      int queue_size = 1;
    } ros;
    struct{
      std::string topic;
      int qos = 0;
      bool retained = false;
    } mqtt;
    bool stamped = false;
  };

  struct Mqtt2RosInterface {
    struct {
      int qos = 0;
    } mqtt;
    struct {
      std::string topic;
      rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher;
      rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr latency_publisher;
      int queue_size = 1;
      bool latched = false;
    } ros;
  };


 protected:

  static const std::string kRosMsgTypeMqttTopicPrefix;

  static const std::string kLatencyRosTopicPrefix;

  BrokerConfig broker_config_;

  ClientConfig client_config_;

  std::map<std::string, Ros2MqttInterface> ros2mqtt_;

  std::map<std::string, Mqtt2RosInterface> mqtt2ros_;


};

template <typename T>
bool MqttClient::loadParameter(const std::string& key, T& value) {
  bool found = MqttClient::get_parameter(key, value);
  if (found)
    RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), "Retrieved parameter '%s' = '%s'", key.c_str(), std::to_string(value).c_str());
  
  return found;
}

template <typename T>
bool MqttClient::loadParameter(const std::string& key, T& value, const T& default_value) {
  bool found = MqttClient::get_parameter_or(key, value, default_value);
  if (!found)
    RCLCPP_WARN(rclcpp::get_logger("rclcpp"), "Parameter '%s' not set, defaulting to '%s'", key.c_str(), std::to_string(default_value).c_str());

  if (found)
    RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), "Retrieved parameter '%s' = '%s'", key.c_str(), std::to_string(value).c_str());

  return found;
}

} //Namespace