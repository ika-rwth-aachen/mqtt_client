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
#include "rclcpp/serialization.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"

using std::placeholders::_1;

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

  loadParameters();
  setup();



}

// Parameter
const std::string MqttClient::kRosMsgTypeMqttTopicPrefix =
  "mqtt_client/ros_msg_type/";

const std::string MqttClient::kLatencyRosTopicPrefix = "latencies/";

// functions
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
    // Alternative: Use list_parameters() function 

    // ros2mqtt
    rclcpp::Parameter ros_topic;
    rclcpp::Parameter mqtt_topic;
    if (get_parameter("bridge.ros2mqtt.ros_topic", ros_topic) && get_parameter("bridge.ros2mqtt.mqtt_topic", mqtt_topic)) {

      Ros2MqttInterface& ros2mqtt = ros2mqtt_[ros_topic.as_string()];
      ros2mqtt.mqtt.topic = mqtt_topic.as_string();

      rclcpp::Parameter stamped;
      if (get_parameter("bridge.ros2mqtt.inject_timestamp", stamped)) {
        ros2mqtt.stamped = stamped.as_bool();
      }

      rclcpp::Parameter queue_size;
      if (get_parameter("bridge.ros2mqtt.advanced.ros.queue_size", queue_size)) {
        ros2mqtt.ros.queue_size = queue_size.as_int();
      }

      rclcpp::Parameter qos;
      if (get_parameter("bridge.ros2mqtt.advanced.mqtt.qos", qos)) {
        ros2mqtt.mqtt.qos = qos.as_int();
      }

      rclcpp::Parameter retained;
      if (get_parameter("bridge.ros2mqtt.advanced.mqtt.retained", retained)) {
        ros2mqtt.mqtt.retained = retained.as_bool();
      }

      RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Bridging ROS topic '%s' to MQTT topic '%s'", ros_topic.as_string().c_str(), ros2mqtt.mqtt.topic.c_str());
    } else {
      RCLCPP_WARN(rclcpp::get_logger("rclcpp"), "Parameter struct 'bridge.ros2mqtt' is missing subparameter "
       "'ros_topic' or 'mqtt_topic', will be ignored");
    }
    
    // mqtt2ros
    if (get_parameter("bridge.mqtt2ros.ros_topic", ros_topic) && get_parameter("bridge.mqtt2ros.mqtt_topic", mqtt_topic)) {

      Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_topic.as_string()];
      mqtt2ros.ros.topic = ros_topic.as_string();

      rclcpp::Parameter qos;
      if (get_parameter("bridge.mqtt2ros.advanced.mqtt.qos", qos)) {
        mqtt2ros.mqtt.qos = qos.as_int();
      }

      rclcpp::Parameter queue_size;
      if (get_parameter("bridge.mqtt2ros.advanced.ros.queue_size", queue_size)) {
        mqtt2ros.ros.queue_size = queue_size.as_int();
      }

      rclcpp::Parameter latched;
      if (get_parameter("bridge.mqtt2ros.advanced.ros.latched", latched)) {
        mqtt2ros.ros.latched = latched.as_bool();
      }

      RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Bridging MQTT topic '%s' to ROS topic '%s'", mqtt_topic.as_string().c_str(), mqtt2ros.ros.topic.c_str());
    } else {
      RCLCPP_WARN(rclcpp::get_logger("rclcpp"), "Parameter struct 'bridge.mqtt2ros' is missing subparameter "
       "'mqtt_topic' or 'ros_topic', will be ignored");
    }   

    if (ros2mqtt_.empty() && mqtt2ros_.empty()) {
      RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "No valid ROS-MQTT bridge found in parameter struct 'bridge'");
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

void MqttClient::setup() {

  setupClient();

  //connect();

  for (auto& ros2mqtt_p : ros2mqtt_) {
    const std::string& ros_topic = ros2mqtt_p.first;
    Ros2MqttInterface& ros2mqtt = ros2mqtt_p.second;
    //const std::string& mqtt_topic = ros2mqtt.mqtt.topic;

    // std::function<void(const std_msgs::msg::String::SharedPtr msg)> bound_callback_func = std::bind(&MqttClient::ros2mqtt, this, _1, ros_topic);
    // ros2mqtt.ros.subscription = create_subscription<sensor_msgs::msg::PointCloud2>(ros_topic, ros2mqtt.ros.queue_size, bound_callback_func);
    //ros2mqtt.ros.subscription = create_subscription<sensor_msgs::msg::PointCloud2>(ros_topic, ros2mqtt.ros.queue_size, std::bind(&MqttClient::ros2mqtt, this, _1, ros_topic));
    
    RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), "Subscribed ROS topic '%s'", ros_topic.c_str());
  }
}

void MqttClient::setupClient() {

  connect_options_.set_automatic_reconnect(true);
  connect_options_.set_clean_session(client_config_.clean_session);
  connect_options_.set_keep_alive_interval(client_config_.keep_alive_interval);
  connect_options_.set_max_inflight(client_config_.max_inflight);

  if (!broker_config_.user.empty()) {
    connect_options_.set_user_name(broker_config_.user);
    connect_options_.set_password(broker_config_.pass);
  }

  if (!client_config_.last_will.topic.empty()) {
    mqtt::will_options will(
      client_config_.last_will.topic, client_config_.last_will.message,
      client_config_.last_will.qos, client_config_.last_will.retained);
    connect_options_.set_will(will);
  }

  if (broker_config_.tls.enabled) {
    mqtt::ssl_options ssl;
    ssl.set_trust_store(broker_config_.tls.ca_certificate.string());
    if (!client_config_.tls.certificate.empty() &&
        !client_config_.tls.key.empty()) {
      ssl.set_key_store(client_config_.tls.certificate.string());
      ssl.set_private_key(client_config_.tls.key.string());
      if (!client_config_.tls.password.empty())
        ssl.set_private_key_password(client_config_.tls.password);
    }
    connect_options_.set_ssl(ssl);
  }

  std::string protocol = broker_config_.tls.enabled ? "ssl" : "tcp";
  std::string uri = protocol + std::string("://") + broker_config_.host +
                    std::string(":") + std::to_string(broker_config_.port);
  try {
    if (client_config_.buffer.enabled) {
      client_ = std::shared_ptr<mqtt::async_client>(new mqtt::async_client(
        uri, client_config_.id, client_config_.buffer.size,
        client_config_.buffer.directory.string()));
    } else {
      client_ = std::shared_ptr<mqtt::async_client>(
        new mqtt::async_client(uri, client_config_.id));
    }
  } catch (const mqtt::exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Client could not be initialized: %s", e.what());
    exit(EXIT_FAILURE);
  }

  // setup MQTT callbacks
  client_->set_callback(*this);

}

void MqttClient::ros2mqtt(const sensor_msgs::msg::PointCloud2 ros_msg, const std::string& ros_topic){
  
  Ros2MqttInterface& ros2mqtt = ros2mqtt_[ros_topic];
  std::string mqtt_topic = kRosMsgTypeMqttTopicPrefix + ros2mqtt.mqtt.topic;
  
  RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), "Received ROS message on topic '%s'", ros_topic.c_str());

  // serialize ROS message to buffer
  //sensor_msgs::msg::PointCloud2 ros_msg;
  rclcpp::SerializedMessage serialized_msg; 
  uint32_t msg_length = static_cast<size_t>(sizeof(ros_msg));
  std::vector<uint8_t> msg_buffer;
  msg_buffer.resize(msg_length);

  serialized_msg.reserve(msg_length);

  static rclcpp::Serialization<sensor_msgs::msg::PointCloud2> serializer;
  serializer.serialize_message(msg_buffer.data(), &serialized_msg);

  // build MQTT payload for ROS message (R) as [0, R]
  uint32_t payload_length = 1 + msg_length; 
  uint32_t msg_offset = 1;
  std::vector<uint8_t> payload_buffer;
  if (ros2mqtt.stamped) {
    RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Timestamp injection is not supported in this application yet.");
  } else {
    payload_buffer.resize(payload_length);
    payload_buffer[0] = 0;
  }

  payload_buffer.insert(payload_buffer.begin() + msg_offset, std::make_move_iterator(msg_buffer.begin()), std::make_move_iterator(msg_buffer.end()));
  mqtt_topic = ros2mqtt.mqtt.topic;
  try {
    RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), 
      "Sending ROS message of type 'PointCloud2' to MQTT broker on topic '%s' ...", mqtt_topic.c_str());
    mqtt::message_ptr mqtt_msg =
      mqtt::make_message(mqtt_topic, payload_buffer.data(), payload_length,
                         ros2mqtt.mqtt.qos, ros2mqtt.mqtt.retained);
    client_->publish(mqtt_msg);
  } catch (const mqtt::exception& e) {
    RCLCPP_WARN(rclcpp::get_logger("rclcpp"), "Publishing ROS message type information to MQTT topic '%s' failed: %s", mqtt_topic.c_str(), e.what());
  }
}

void MqttClient::mqtt2ros(mqtt::const_message_ptr mqtt_msg) {

  std::string mqtt_topic = mqtt_msg->get_topic();
  Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_topic];
  auto& payload = mqtt_msg->get_payload_ref();
  uint32_t payload_length = static_cast<uint32_t>(payload.size());

  // determine whether timestamp is injected by reading first element
  bool stamped = (static_cast<uint8_t>(payload[0]) > 0);

  uint32_t msg_length = payload_length - 1;
  uint32_t msg_offset = 1;

  if (stamped) {
    RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Timestamp injection is not supported in this application yet.");
    exit(EXIT_FAILURE);
  }

  std::vector<uint8_t> msg_buffer;
  msg_buffer.resize(msg_length);
  std::memcpy(msg_buffer.data(), &(payload[msg_offset]), msg_length);
  
  rclcpp::SerializedMessage serialized_msg; 
  serialized_msg.reserve(msg_length);

  RCLCPP_DEBUG(rclcpp::get_logger("rclcppp"), "Sending ROS message from MQTT broker to ROS topic '%s' ...", mqtt2ros.ros.topic.c_str());

  static rclcpp::Serialization<sensor_msgs::msg::PointCloud2> serializer;
  serializer.serialize_message(msg_buffer.data(), &serialized_msg);

  mqtt2ros.ros.publisher = create_publisher<std_msgs::msg::String>(mqtt2ros.ros.topic, mqtt2ros.ros.queue_size); //TODO: pub definition in constructor or setup func?
  mqtt2ros.ros.publisher->publish(serialized_msg);
}

void MqttClient::message_arrived(mqtt::const_message_ptr mqtt_msg) {

  std::string mqtt_topic = mqtt_msg->get_topic();
  RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), "Received MQTT message on topic '%s'", mqtt_topic.c_str());
  // auto& payload = mqtt_msg->get_payload_ref();
  // uint32_t payload_length = static_cast<uint32_t>(payload.size());

  bool msg_contains_ros_msg_type = mqtt_topic.find(kRosMsgTypeMqttTopicPrefix) != std::string::npos;
  if (msg_contains_ros_msg_type) {
    RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Changing ROS message type is not supported in this application yet."
      "Only PointCloud2 message type known");
    exit(EXIT_FAILURE);
  } else {

    if (mqtt2ros_[mqtt_topic].ros.publisher->get_topic_name() && !mqtt2ros_[mqtt_topic].ros.publisher->get_topic_name()[0]){
      mqtt2ros(mqtt_msg);
    } else {
      RCLCPP_WARN(rclcpp::get_logger("rclcpp"), "ROS publisher for data from MQTT topic '%s' is not yet initialized: "
        "ROS message type not yet known",
        mqtt_topic.c_str());
    }
  }
}



} //Namespace

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<mqtt_client::MqttClient>());
  rclcpp::shutdown();
  return 0;
}

