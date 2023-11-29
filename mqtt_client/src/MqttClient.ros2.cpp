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


#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include <mqtt_client/MqttClient.ros2.hpp>
#include <mqtt_client_interfaces/msg/ros_msg_type.hpp>
#include <rcpputils/env.hpp>
#include <std_msgs/msg/bool.hpp>
#include <std_msgs/msg/char.hpp>
#include <std_msgs/msg/float32.hpp>
#include <std_msgs/msg/float64.hpp>
#include <std_msgs/msg/int16.hpp>
#include <std_msgs/msg/int32.hpp>
#include <std_msgs/msg/int64.hpp>
#include <std_msgs/msg/int8.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/u_int16.hpp>
#include <std_msgs/msg/u_int32.hpp>
#include <std_msgs/msg/u_int64.hpp>
#include <std_msgs/msg/u_int8.hpp>

#include <rclcpp_components/register_node_macro.hpp>
RCLCPP_COMPONENTS_REGISTER_NODE(mqtt_client::MqttClient)


namespace mqtt_client {


const std::string MqttClient::kRosMsgTypeMqttTopicPrefix =
  "mqtt_client/ros_msg_type/";

const std::string MqttClient::kLatencyRosTopicPrefix = "~/latencies/";


/**
 * @brief Extracts string of primitive data types from ROS message.
 *
 * This is helpful to extract the actual data payload of a primitive ROS
 * message. If e.g. an std_msgs/msg/String is serialized to a string
 * representation, it also contains the field name 'data'. This function
 * instead returns the underlying value as string.
 *
 * @param [in]  serialized_msg  generic serialized ROS message
 * @param [in]  msg_type        ROS message type, e.g. `std_msgs/msg/String`
 * @param [out] primitive       string representation of primitive message data
 *
 * @return true  if primitive ROS message type was found
 * @return false if ROS message type is not primitive
 */
bool primitiveRosMessageToString(
  const std::shared_ptr<rclcpp::SerializedMessage>& serialized_msg,
  const std::string& msg_type, std::string& primitive) {

  bool found_primitive = true;

  if (msg_type == "std_msgs/msg/String") {
    std_msgs::msg::String msg;
    deserializeRosMessage(*serialized_msg, msg);
    primitive = msg.data;
  } else if (msg_type == "std_msgs/msg/Bool") {
    std_msgs::msg::Bool msg;
    deserializeRosMessage(*serialized_msg, msg);
    primitive = msg.data ? "true" : "false";
  } else if (msg_type == "std_msgs/msg/Char") {
    std_msgs::msg::Char msg;
    deserializeRosMessage(*serialized_msg, msg);
    primitive = std::to_string(msg.data);
  } else if (msg_type == "std_msgs/msg/UInt8") {
    std_msgs::msg::UInt8 msg;
    deserializeRosMessage(*serialized_msg, msg);
    primitive = std::to_string(msg.data);
  } else if (msg_type == "std_msgs/msg/UInt16") {
    std_msgs::msg::UInt16 msg;
    deserializeRosMessage(*serialized_msg, msg);
    primitive = std::to_string(msg.data);
  } else if (msg_type == "std_msgs/msg/UInt32") {
    std_msgs::msg::UInt32 msg;
    deserializeRosMessage(*serialized_msg, msg);
    primitive = std::to_string(msg.data);
  } else if (msg_type == "std_msgs/msg/UInt64") {
    std_msgs::msg::UInt64 msg;
    deserializeRosMessage(*serialized_msg, msg);
    primitive = std::to_string(msg.data);
  } else if (msg_type == "std_msgs/msg/Int8") {
    std_msgs::msg::Int8 msg;
    deserializeRosMessage(*serialized_msg, msg);
    primitive = std::to_string(msg.data);
  } else if (msg_type == "std_msgs/msg/Int16") {
    std_msgs::msg::Int16 msg;
    deserializeRosMessage(*serialized_msg, msg);
    primitive = std::to_string(msg.data);
  } else if (msg_type == "std_msgs/msg/Int32") {
    std_msgs::msg::Int32 msg;
    deserializeRosMessage(*serialized_msg, msg);
    primitive = std::to_string(msg.data);
  } else if (msg_type == "std_msgs/msg/Int64") {
    std_msgs::msg::Int64 msg;
    deserializeRosMessage(*serialized_msg, msg);
    primitive = std::to_string(msg.data);
  } else if (msg_type == "std_msgs/msg/Float32") {
    std_msgs::msg::Float32 msg;
    deserializeRosMessage(*serialized_msg, msg);
    primitive = std::to_string(msg.data);
  } else if (msg_type == "std_msgs/msg/Float64") {
    std_msgs::msg::Float64 msg;
    deserializeRosMessage(*serialized_msg, msg);
    primitive = std::to_string(msg.data);
  } else {
    found_primitive = false;
  }

  return found_primitive;
}


MqttClient::MqttClient(const rclcpp::NodeOptions& options) : Node("mqtt_client", options) {

  loadParameters();
  setup();
}


void MqttClient::loadParameters() {

  rcl_interfaces::msg::ParameterDescriptor param_desc;

  param_desc.description = "IP address or hostname of the machine running the MQTT broker";
  declare_parameter("broker.host", rclcpp::ParameterType::PARAMETER_STRING, param_desc);
  param_desc.description = "port the MQTT broker is listening on";
  declare_parameter("broker.port", rclcpp::ParameterType::PARAMETER_INTEGER, param_desc);
  param_desc.description = "username used for authenticating to the broker (if empty, will try to connect anonymously)";
  declare_parameter("broker.user", rclcpp::ParameterType::PARAMETER_STRING, param_desc);
  param_desc.description = "password used for authenticating to the broker";
  declare_parameter("broker.pass", rclcpp::ParameterType::PARAMETER_STRING, param_desc);
  param_desc.description = "whether to connect via SSL/TLS";
  declare_parameter("broker.tls.enabled", rclcpp::ParameterType::PARAMETER_BOOL, param_desc);
  param_desc.description = "CA certificate file trusted by client (relative to ROS_HOME)";
  declare_parameter("broker.tls.ca_certificate", rclcpp::ParameterType::PARAMETER_STRING, param_desc);

  param_desc.description = "unique ID used to identify the client (broker may allow empty ID and automatically generate one)";
  declare_parameter("client.id", rclcpp::ParameterType::PARAMETER_STRING, param_desc);
  param_desc.description = "maximum number of messages buffered by the bridge when not connected to broker (only available if client ID is not empty)";
  declare_parameter("client.buffer.size", rclcpp::ParameterType::PARAMETER_INTEGER, param_desc);
  param_desc.description = "directory used to buffer messages when not connected to broker (relative to ROS_HOME)";
  declare_parameter("client.buffer.directory", rclcpp::ParameterType::PARAMETER_STRING, param_desc);
  param_desc.description = "topic used for this client's last-will message (no last will, if not specified)";
  declare_parameter("client.last_will.topic", rclcpp::ParameterType::PARAMETER_STRING, param_desc);
  param_desc.description = "last-will message";
  declare_parameter("client.last_will.message", rclcpp::ParameterType::PARAMETER_STRING, param_desc);
  param_desc.description = "QoS value for last-will message";
  declare_parameter("client.last_will.qos", rclcpp::ParameterType::PARAMETER_INTEGER, param_desc);
  param_desc.description = "whether to retain last-will message";
  declare_parameter("client.last_will.retained", rclcpp::ParameterType::PARAMETER_BOOL, param_desc);
  param_desc.description = "whether to use a clean session for this client";
  declare_parameter("client.clean_session", rclcpp::ParameterType::PARAMETER_BOOL, param_desc);
  param_desc.description = "keep-alive interval in seconds";
  declare_parameter("client.keep_alive_interval", rclcpp::ParameterType::PARAMETER_DOUBLE, param_desc);
  param_desc.description = "maximum number of inflight messages";
  declare_parameter("client.max_inflight", rclcpp::ParameterType::PARAMETER_INTEGER, param_desc);
  param_desc.description = "client certificate file (only needed if broker requires client certificates; relative to ROS_HOME)";
  declare_parameter("client.tls.certificate", rclcpp::ParameterType::PARAMETER_STRING, param_desc);
  param_desc.description = "client private key file (relative to ROS_HOME)";
  declare_parameter("client.tls.key", rclcpp::ParameterType::PARAMETER_STRING, param_desc);
  param_desc.description = "client private key password";
  declare_parameter("client.tls.password", rclcpp::ParameterType::PARAMETER_STRING, param_desc);

  param_desc.description = "The list of topics to bridge from ROS to MQTT";
  const auto ros2mqtt_ros_topics = declare_parameter<std::vector<std::string>>("bridge.ros2mqtt.ros_topics", std::vector<std::string>(), param_desc);
  for (const auto& ros_topic : ros2mqtt_ros_topics)
  {
    param_desc.description = "MQTT topic on which the corresponding ROS messages are sent to the broker";
    declare_parameter(fmt::format("bridge.ros2mqtt.{}.mqtt_topic", ros_topic), rclcpp::ParameterType::PARAMETER_STRING, param_desc);
    param_desc.description = "whether to publish as primitive message";
    declare_parameter(fmt::format("bridge.ros2mqtt.{}.primitive", ros_topic), rclcpp::ParameterType::PARAMETER_BOOL, param_desc);
    param_desc.description = "whether to attach a timestamp to a ROS2MQTT payload (for latency computation on receiver side)";
    declare_parameter(fmt::format("bridge.ros2mqtt.{}.inject_timestamp", ros_topic), rclcpp::ParameterType::PARAMETER_BOOL, param_desc);
    param_desc.description = "ROS subscriber queue size";
    declare_parameter(fmt::format("bridge.ros2mqtt.{}.advanced.ros.queue_size", ros_topic), rclcpp::ParameterType::PARAMETER_INTEGER, param_desc);
    param_desc.description = "MQTT QoS value";
    declare_parameter(fmt::format("bridge.ros2mqtt.{}.advanced.mqtt.qos", ros_topic), rclcpp::ParameterType::PARAMETER_INTEGER, param_desc);
    param_desc.description = "whether to retain MQTT message";
    declare_parameter(fmt::format("bridge.ros2mqtt.{}.advanced.mqtt.retained", ros_topic), rclcpp::ParameterType::PARAMETER_BOOL, param_desc);
  }

  param_desc.description = "The list of topics to bridge from MQTT to ROS";
  const auto mqtt2ros_mqtt_topics = declare_parameter<std::vector<std::string>>("bridge.mqtt2ros.mqtt_topics", std::vector<std::string>(), param_desc);
  for (const auto& mqtt_topic : mqtt2ros_mqtt_topics)
  {
    param_desc.description = "ROS topic on which corresponding MQTT messages are published";
    declare_parameter(fmt::format("bridge.mqtt2ros.{}.ros_topic", mqtt_topic), rclcpp::ParameterType::PARAMETER_STRING, param_desc);
    param_desc.description = "whether to publish as primitive message (if coming from non-ROS MQTT client)";
    declare_parameter(fmt::format("bridge.mqtt2ros.{}.primitive", mqtt_topic), rclcpp::ParameterType::PARAMETER_BOOL, param_desc);
    param_desc.description = "MQTT QoS value";
    declare_parameter(fmt::format("bridge.mqtt2ros.{}.advanced.mqtt.qos", mqtt_topic), rclcpp::ParameterType::PARAMETER_INTEGER, param_desc);
    param_desc.description = "ROS publisher queue size";
    declare_parameter(fmt::format("bridge.mqtt2ros.{}.advanced.ros.queue_size", mqtt_topic), rclcpp::ParameterType::PARAMETER_INTEGER, param_desc);
    param_desc.description = "whether to latch ROS message";
    declare_parameter(fmt::format("bridge.mqtt2ros.{}.advanced.ros.latched", mqtt_topic), rclcpp::ParameterType::PARAMETER_BOOL, param_desc);
  }

  // load broker parameters from parameter server
  std::string broker_tls_ca_certificate;
  loadParameter("broker.host", broker_config_.host, "localhost");
  loadParameter("broker.port", broker_config_.port, 1883);
  if (loadParameter("broker.user", broker_config_.user)) {
    loadParameter("broker.pass", broker_config_.pass, "");
  }
  if (loadParameter("broker.tls.enabled", broker_config_.tls.enabled, false)) {
    loadParameter("broker.tls.ca_certificate", broker_tls_ca_certificate,
                  "/etc/ssl/certs/ca-certificates.crt");
  }

  // load client parameters from parameter server
  std::string client_buffer_directory, client_tls_certificate, client_tls_key;
  loadParameter("client.id", client_config_.id, "");
  client_config_.buffer.enabled = !client_config_.id.empty();
  if (client_config_.buffer.enabled) {
    loadParameter("client.buffer.size", client_config_.buffer.size, 0);
    loadParameter("client.buffer.directory", client_buffer_directory, "buffer");
  } else {
    RCLCPP_WARN(get_logger(),
                "Client buffer can not be enabled when client ID is empty");
  }
  if (loadParameter("client.last_will.topic", client_config_.last_will.topic)) {
    loadParameter("client.last_will.message", client_config_.last_will.message,
                  "offline");
    loadParameter("client.last_will.qos", client_config_.last_will.qos, 0);
    loadParameter("client.last_will.retained",
                  client_config_.last_will.retained, false);
  }
  loadParameter("client.clean_session", client_config_.clean_session, true);
  loadParameter("client.keep_alive_interval",
                client_config_.keep_alive_interval, 60.0);
  loadParameter("client.max_inflight", client_config_.max_inflight, 65535);
  if (broker_config_.tls.enabled) {
    if (loadParameter("client.tls.certificate", client_tls_certificate)) {
      loadParameter("client.tls.key", client_tls_key);
      loadParameter("client.tls.password", client_config_.tls.password);
      loadParameter("client.tls.version", client_config_.tls.version);
      loadParameter("client.tls.verify", client_config_.tls.verify);
      loadParameter("client.tls.alpn_protos", client_config_.tls.alpn_protos);
    }
  }

  // resolve filepaths
  broker_config_.tls.ca_certificate = resolvePath(broker_tls_ca_certificate);
  client_config_.buffer.directory = resolvePath(client_buffer_directory);
  client_config_.tls.certificate = resolvePath(client_tls_certificate);
  client_config_.tls.key = resolvePath(client_tls_key);

  // parse bridge parameters

  // ros2mqtt
  for (const auto& ros_topic : ros2mqtt_ros_topics) {

    rclcpp::Parameter mqtt_topic_param;
    if (get_parameter(fmt::format("bridge.ros2mqtt.{}.mqtt_topic", ros_topic), mqtt_topic_param)) {

      // ros2mqtt[k]/ros_topic and ros2mqtt[k]/mqtt_topic
      const std::string mqtt_topic = mqtt_topic_param.as_string();
      Ros2MqttInterface& ros2mqtt = ros2mqtt_[ros_topic];
      ros2mqtt.mqtt.topic = mqtt_topic;

      // ros2mqtt[k]/primitive
      rclcpp::Parameter primitive_param;
      if (get_parameter(fmt::format("bridge.ros2mqtt.{}.primitive", ros_topic), primitive_param))
        ros2mqtt.primitive = primitive_param.as_bool();

      // ros2mqtt[k]/inject_timestamp
      rclcpp::Parameter stamped_param;
      if (get_parameter(fmt::format("bridge.ros2mqtt.{}.inject_timestamp", ros_topic), stamped_param))
        ros2mqtt.stamped = stamped_param.as_bool();
      if (ros2mqtt.stamped && ros2mqtt.primitive) {
        RCLCPP_WARN(
          get_logger(),
          "Timestamp will not be injected into primitive messages on ROS "
          "topic '%s'",
          ros_topic.c_str());
        ros2mqtt.stamped = false;
      }

      // ros2mqtt[k]/advanced/ros/queue_size
      rclcpp::Parameter queue_size_param;
      if (get_parameter(fmt::format("bridge.ros2mqtt.{}.advanced.ros.queue_size", ros_topic),
                        queue_size_param))
        ros2mqtt.ros.queue_size = queue_size_param.as_int();

      // ros2mqtt[k]/advanced/mqtt/qos
      rclcpp::Parameter qos_param;
      if (get_parameter(fmt::format("bridge.ros2mqtt.{}.advanced.mqtt.qos", ros_topic), qos_param))
        ros2mqtt.mqtt.qos = qos_param.as_int();

      // ros2mqtt[k]/advanced/mqtt/retained
      rclcpp::Parameter retained_param;
      if (get_parameter(fmt::format("bridge.ros2mqtt.{}.advanced.mqtt.retained", ros_topic), retained_param))
        ros2mqtt.mqtt.retained = retained_param.as_bool();

      RCLCPP_INFO(get_logger(), "Bridging %sROS topic '%s' to MQTT topic '%s' %s",
                  ros2mqtt.primitive ? "primitive " : "", ros_topic.c_str(),
                  ros2mqtt.mqtt.topic.c_str(),
                  ros2mqtt.stamped ? "and measuring latency" : "");
    } else {
      RCLCPP_WARN(get_logger(),
                  fmt::format("Parameter 'bridge.ros2mqtt.{}' is missing subparameter "
                  "'mqtt_topic', will be ignored", ros_topic).c_str());
    }
  }

  // mqtt2ros
  for (const auto& mqtt_topic : mqtt2ros_mqtt_topics) {

    rclcpp::Parameter ros_topic_param;
    if (get_parameter(fmt::format("bridge.mqtt2ros.{}.ros_topic", mqtt_topic), ros_topic_param)) {

      // mqtt2ros[k]/mqtt_topic and mqtt2ros[k]/ros_topic
      const std::string ros_topic = ros_topic_param.as_string();
      Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_topic];
      mqtt2ros.ros.topic = ros_topic;

      // mqtt2ros[k]/primitive
      rclcpp::Parameter primitive_param;
      if (get_parameter(fmt::format("bridge.mqtt2ros.{}.primitive", mqtt_topic), primitive_param))
        mqtt2ros.primitive = primitive_param.as_bool();

      // mqtt2ros[k]/advanced/mqtt/qos
      rclcpp::Parameter qos_param;
      if (get_parameter(fmt::format("bridge.mqtt2ros.{}.advanced.mqtt.qos", mqtt_topic), qos_param))
        mqtt2ros.mqtt.qos = qos_param.as_int();

      // mqtt2ros[k]/advanced/ros/queue_size
      rclcpp::Parameter queue_size_param;
      if (get_parameter(fmt::format("bridge.mqtt2ros.{}.advanced.ros.queue_size", mqtt_topic),
                        queue_size_param))
        mqtt2ros.ros.queue_size = queue_size_param.as_int();

      // mqtt2ros[k]/advanced/ros/latched
      rclcpp::Parameter latched_param;
      if (get_parameter(fmt::format("bridge.mqtt2ros.{}.advanced.ros.latched", mqtt_topic), latched_param)) {
        mqtt2ros.ros.latched = latched_param.as_bool();
        RCLCPP_WARN(get_logger(),
                    fmt::format("Parameter 'bridge.mqtt2ros.{}.advanced.ros.latched' is ignored "
                    "since ROS 2 does not easily support latched topics.", mqtt_topic).c_str());
      }

      RCLCPP_INFO(get_logger(), "Bridging MQTT topic '%s' to %sROS topic '%s'",
                  mqtt_topic.c_str(), mqtt2ros.primitive ? "primitive " : "",
                  mqtt2ros.ros.topic.c_str());
    }
    else {
      RCLCPP_WARN(get_logger(),
                fmt::format("Parameter 'bridge.ros2mqtt.{}' is missing subparameter "
                "'ros_topic', will be ignored", mqtt_topic).c_str());
    }
  }

  if (ros2mqtt_.empty() && mqtt2ros_.empty()) {
    RCLCPP_ERROR(get_logger(),
                 "No valid ROS-MQTT bridge found in parameter 'bridge'");
    exit(EXIT_FAILURE);
  }
}


bool MqttClient::loadParameter(const std::string& key, std::string& value) {
  bool found = get_parameter(key, value);
  if (found)
    RCLCPP_DEBUG(get_logger(), "Retrieved parameter '%s' = '%s'", key.c_str(),
                 value.c_str());
  return found;
}


bool MqttClient::loadParameter(const std::string& key, std::string& value,
                               const std::string& default_value) {
  bool found = get_parameter_or(key, value, default_value);
  if (!found)
    RCLCPP_WARN(get_logger(), "Parameter '%s' not set, defaulting to '%s'",
                key.c_str(), default_value.c_str());
  if (found)
    RCLCPP_DEBUG(get_logger(), "Retrieved parameter '%s' = '%s'", key.c_str(),
                 value.c_str());
  return found;
}


std::filesystem::path MqttClient::resolvePath(const std::string& path_string) {

  std::filesystem::path path(path_string);
  if (path_string.empty()) return path;
  if (!path.has_root_path()) {
    std::string ros_home = rcpputils::get_env_var("ROS_HOME");
    if (ros_home.empty())
      ros_home = std::string(std::filesystem::current_path());
    path = std::filesystem::path(ros_home);
    path.append(path_string);
  }
  if (!std::filesystem::exists(path))
    RCLCPP_WARN(get_logger(), "Requested path '%s' does not exist",
                std::string(path).c_str());
  return path;
}


void MqttClient::setup() {

  // pre-compute timestamp length
  builtin_interfaces::msg::Time tmp_stamp;
  rclcpp::SerializedMessage tmp_serialized_stamp;
  serializeRosMessage(tmp_stamp, tmp_serialized_stamp);
  stamp_length_ = tmp_serialized_stamp.size();

  // initialize MQTT client
  setupClient();

  // connect to MQTT broker
  connect();

  // create ROS service server
  is_connected_service_ =
    create_service<mqtt_client_interfaces::srv::IsConnected>(
      "~/is_connected", std::bind(&MqttClient::isConnectedService, this,
                                std::placeholders::_1, std::placeholders::_2));

  // create dynamic mappings services
  new_ros2mqtt_bridge_service_ =
  create_service<mqtt_client_interfaces::srv::NewRos2MqttBridge>(
      "~/new_ros2mqtt_bridge", std::bind(&MqttClient::newRos2MqttBridge, this,
                                std::placeholders::_1, std::placeholders::_2));
  new_mqtt2ros_bridge_service_ =
  create_service<mqtt_client_interfaces::srv::NewMqtt2RosBridge>(
      "~/new_mqtt2ros_bridge", std::bind(&MqttClient::newMqtt2RosBridge, this,
                                std::placeholders::_1, std::placeholders::_2));

  // setup subscribers; check for new types every second
  check_subscriptions_timer_ =
    create_wall_timer(std::chrono::duration<double>(1.0),
                      std::bind(&MqttClient::setupSubscriptions, this));
}


void MqttClient::setupSubscriptions() {

  // get info of all topics
  const auto all_topics_and_types = get_topic_names_and_types();

  // check for ros2mqtt topics
  for (auto& [ros_topic, ros2mqtt] : ros2mqtt_) {
    if (all_topics_and_types.count(ros_topic)) {

      // check if message type has changed or if mapping is stale
      const std::string& msg_type = all_topics_and_types.at(ros_topic)[0];
      if (msg_type == ros2mqtt.ros.msg_type && !ros2mqtt.ros.is_stale) continue;
      ros2mqtt.ros.is_stale = false;
      ros2mqtt.ros.msg_type = msg_type;

      // create new generic subscription, if message type has changed
      std::function<void(const std::shared_ptr<rclcpp::SerializedMessage> msg)>
        bound_callback_func = std::bind(&MqttClient::ros2mqtt, this,
                                        std::placeholders::_1, ros_topic);
      try {
        ros2mqtt.ros.subscriber = create_generic_subscription(
          ros_topic, msg_type, ros2mqtt.ros.queue_size, bound_callback_func);
      } catch (rclcpp::exceptions::RCLError& e) {
        RCLCPP_ERROR(get_logger(), "Failed to create generic subscriber: %s",
                     e.what());
        return;
      }
      RCLCPP_INFO(get_logger(), "Subscribed ROS topic '%s' of type '%s'",
                  ros_topic.c_str(), msg_type.c_str());
    }
  }
}


void MqttClient::setupClient() {

  // basic client connection options
  connect_options_.set_automatic_reconnect(true);
  connect_options_.set_clean_session(client_config_.clean_session);
  connect_options_.set_keep_alive_interval(client_config_.keep_alive_interval);
  connect_options_.set_max_inflight(client_config_.max_inflight);

  // user authentication
  if (!broker_config_.user.empty()) {
    connect_options_.set_user_name(broker_config_.user);
    connect_options_.set_password(broker_config_.pass);
  }

  // last will
  if (!client_config_.last_will.topic.empty()) {
    mqtt::will_options will(
      client_config_.last_will.topic, client_config_.last_will.message,
      client_config_.last_will.qos, client_config_.last_will.retained);
    connect_options_.set_will(will);
  }

  // SSL/TLS
  if (broker_config_.tls.enabled) {
    mqtt::ssl_options ssl;
    ssl.set_trust_store(broker_config_.tls.ca_certificate);
    if (!client_config_.tls.certificate.empty() &&
        !client_config_.tls.key.empty()) {
      ssl.set_key_store(client_config_.tls.certificate);
      ssl.set_private_key(client_config_.tls.key);
      if (!client_config_.tls.password.empty())
        ssl.set_private_key_password(client_config_.tls.password);
    }
    ssl.set_ssl_version(client_config_.tls.version);
    ssl.set_verify(client_config_.tls.verify);
    ssl.set_alpn_protos(client_config_.tls.alpn_protos);
    connect_options_.set_ssl(ssl);
  }

  // create MQTT client
  const std::string protocol = broker_config_.tls.enabled ? "ssl" : "tcp";
  const std::string uri = fmt::format("{}://{}:{}", protocol, broker_config_.host,
                                      broker_config_.port);
  try {
    if (client_config_.buffer.enabled) {
      client_ = std::shared_ptr<mqtt::async_client>(new mqtt::async_client(
        uri, client_config_.id, client_config_.buffer.size,
        client_config_.buffer.directory));
    } else {
      client_ = std::shared_ptr<mqtt::async_client>(
        new mqtt::async_client(uri, client_config_.id));
    }
  } catch (const mqtt::exception& e) {
    RCLCPP_ERROR(get_logger(), "Client could not be initialized: %s", e.what());
    exit(EXIT_FAILURE);
  }

  // setup MQTT callbacks
  client_->set_callback(*this);
}


void MqttClient::connect() {

  std::string as_client =
    client_config_.id.empty()
      ? ""
      : std::string(" as '") + client_config_.id + std::string("'");
  RCLCPP_INFO(get_logger(), "Connecting to broker at '%s'%s ...",
              client_->get_server_uri().c_str(), as_client.c_str());

  try {
    client_->connect(connect_options_, nullptr, *this);
  } catch (const mqtt::exception& e) {
    RCLCPP_ERROR(get_logger(), "Connection to broker failed: %s", e.what());
    exit(EXIT_FAILURE);
  }
}


void MqttClient::ros2mqtt(
  const std::shared_ptr<rclcpp::SerializedMessage>& serialized_msg,
  const std::string& ros_topic) {

  Ros2MqttInterface& ros2mqtt = ros2mqtt_[ros_topic];
  std::string mqtt_topic = ros2mqtt.mqtt.topic;
  std::vector<uint8_t> payload_buffer;

  // gather information on ROS message type in special ROS message
  mqtt_client_interfaces::msg::RosMsgType ros_msg_type;
  ros_msg_type.name = ros2mqtt.ros.msg_type;
  ros_msg_type.stamped = ros2mqtt.stamped;

  RCLCPP_DEBUG(get_logger(), "Received ROS message of type '%s' on topic '%s'",
               ros_msg_type.name.c_str(), ros_topic.c_str());

  if (ros2mqtt.primitive) {  // publish as primitive (string) message

    // resolve ROS messages to primitive strings if possible
    std::string payload;
    bool found_primitive =
      primitiveRosMessageToString(serialized_msg, ros_msg_type.name, payload);
    if (found_primitive) {
      payload_buffer = std::vector<uint8_t>(payload.begin(), payload.end());
    } else {
      RCLCPP_WARN(get_logger(),
                  "Cannot send ROS message of type '%s' as primitive message, "
                  "check supported primitive types",
                  ros_msg_type.name.c_str());
      return;
    }

  } else {  // publish as complete ROS message incl. ROS message type

    // serialize ROS message type
    rclcpp::SerializedMessage serialized_ros_msg_type;
    serializeRosMessage(ros_msg_type, serialized_ros_msg_type);
    uint32_t msg_type_length = serialized_ros_msg_type.size();
    std::vector<uint8_t> msg_type_buffer = std::vector<uint8_t>(
      serialized_ros_msg_type.get_rcl_serialized_message().buffer,
      serialized_ros_msg_type.get_rcl_serialized_message().buffer + msg_type_length);

    // send ROS message type information to MQTT broker
    mqtt_topic = kRosMsgTypeMqttTopicPrefix + ros2mqtt.mqtt.topic;
    try {
      RCLCPP_DEBUG(get_logger(),
                   "Sending ROS message type to MQTT broker on topic '%s' ...",
                   mqtt_topic.c_str());
      mqtt::message_ptr mqtt_msg =
        mqtt::make_message(mqtt_topic, msg_type_buffer.data(),
                           msg_type_buffer.size(), ros2mqtt.mqtt.qos, true);
      client_->publish(mqtt_msg);
    } catch (const mqtt::exception& e) {
      RCLCPP_WARN(
        get_logger(),
        "Publishing ROS message type information to MQTT topic '%s' failed: %s",
        mqtt_topic.c_str(), e.what());
    }

    // build MQTT payload for ROS message (R) as [R]
    // or [S, R] if timestamp (S) is included
    uint32_t msg_length = serialized_msg->size();
    uint32_t payload_length = msg_length;
    uint32_t msg_offset = 0;
    if (ros2mqtt.stamped) {

      // allocate buffer with appropriate size to hold [S, R]
      msg_offset += stamp_length_;
      payload_length += stamp_length_;
      payload_buffer.resize(payload_length);

      // copy serialized ROS message to payload [-, R]
      std::copy(serialized_msg->get_rcl_serialized_message().buffer,
                serialized_msg->get_rcl_serialized_message().buffer + msg_length,
                payload_buffer.begin() + msg_offset);
    } else {

      // directly build payload buffer on top of serialized message
      payload_buffer = std::vector<uint8_t>(
        serialized_msg->get_rcl_serialized_message().buffer,
        serialized_msg->get_rcl_serialized_message().buffer + msg_length);
    }

    // inject timestamp as final step
    if (ros2mqtt.stamped) {

      // take current timestamp
      builtin_interfaces::msg::Time stamp(rclcpp::Clock(RCL_SYSTEM_TIME).now());

      // copy serialized timestamp to payload [S, R]
      rclcpp::SerializedMessage serialized_stamp;
      serializeRosMessage(stamp, serialized_stamp);
      std::copy(
        serialized_stamp.get_rcl_serialized_message().buffer,
        serialized_stamp.get_rcl_serialized_message().buffer + stamp_length_,
        payload_buffer.begin());
    }
  }

  // send ROS message to MQTT broker
  mqtt_topic = ros2mqtt.mqtt.topic;
  try {
    RCLCPP_DEBUG(
      get_logger(),
      "Sending ROS message of type '%s' to MQTT broker on topic '%s' ...",
      ros_msg_type.name.c_str(), mqtt_topic.c_str());
    mqtt::message_ptr mqtt_msg = mqtt::make_message(
      mqtt_topic, payload_buffer.data(), payload_buffer.size(),
      ros2mqtt.mqtt.qos, ros2mqtt.mqtt.retained);
    client_->publish(mqtt_msg);
  } catch (const mqtt::exception& e) {
    RCLCPP_WARN(
      get_logger(),
      "Publishing ROS message type information to MQTT topic '%s' failed: %s",
      mqtt_topic.c_str(), e.what());
  }
}


void MqttClient::mqtt2ros(mqtt::const_message_ptr mqtt_msg,
                          const rclcpp::Time& arrival_stamp) {

  std::string mqtt_topic = mqtt_msg->get_topic();
  Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_topic];
  auto& payload = mqtt_msg->get_payload_ref();
  uint32_t payload_length = static_cast<uint32_t>(payload.size());

  // read MQTT payload for ROS message (R) as [R]
  // or [S, R] if timestamp (S) is included
  uint32_t msg_length = payload_length;
  uint32_t msg_offset = 0;

  // if stamped, compute latency
  if (mqtt2ros.stamped) {

    // copy stamp to generic message buffer
    rclcpp::SerializedMessage serialized_stamp(stamp_length_);
    std::memcpy(serialized_stamp.get_rcl_serialized_message().buffer,
                &(payload[msg_offset]), stamp_length_);
    serialized_stamp.get_rcl_serialized_message().buffer_length = stamp_length_;

    // deserialize stamp
    builtin_interfaces::msg::Time stamp;
    deserializeRosMessage(serialized_stamp, stamp);

    // compute ROS2MQTT latency
    rclcpp::Duration latency = arrival_stamp - stamp;
    std_msgs::msg::Float64 latency_msg;
    latency_msg.data = latency.seconds();

    // publish latency
    if (!mqtt2ros.ros.latency_publisher) {
      std::string latency_topic = kLatencyRosTopicPrefix + mqtt2ros.ros.topic;
      latency_topic.replace(latency_topic.find("//"), 2, "/");
      mqtt2ros.ros.latency_publisher =
        create_publisher<std_msgs::msg::Float64>(latency_topic, 1);
    }
    mqtt2ros.ros.latency_publisher->publish(latency_msg);

    msg_length -= stamp_length_;
    msg_offset += stamp_length_;
  }

  // copy ROS message from MQTT message to generic message buffer
  rclcpp::SerializedMessage serialized_msg(msg_length);
  std::memcpy(serialized_msg.get_rcl_serialized_message().buffer,
              &(payload[msg_offset]), msg_length);
  serialized_msg.get_rcl_serialized_message().buffer_length = msg_length;

  // publish generic ROS message
  RCLCPP_DEBUG(
    get_logger(),
    "Sending ROS message of type '%s' from MQTT broker to ROS topic '%s' ...",
    mqtt2ros.ros.msg_type.c_str(), mqtt2ros.ros.topic.c_str());
  mqtt2ros.ros.publisher->publish(serialized_msg);
}


void MqttClient::mqtt2primitive(mqtt::const_message_ptr mqtt_msg) {

  std::string mqtt_topic = mqtt_msg->get_topic();
  Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_topic];
  const std::string str_msg = mqtt_msg->to_string();

  bool found_primitive = false;
  std::string ros_msg_type = "std_msgs/msg/String";
  rclcpp::SerializedMessage serialized_msg;

  // check for bool
  if (!found_primitive) {
    std::string bool_str = str_msg;
    std::transform(str_msg.cbegin(), str_msg.cend(), bool_str.begin(),
                   ::tolower);
    if (bool_str == "true" || bool_str == "false") {

      bool bool_msg = (bool_str == "true");

      // construct and serialize ROS message
      std_msgs::msg::Bool msg;
      msg.data = bool_msg;
      serializeRosMessage(msg, serialized_msg);

      ros_msg_type = "std_msgs/msg/Bool";
      found_primitive = true;
    }
  }

  // check for int
  if (!found_primitive) {
    std::size_t pos;
    try {
      const int int_msg = std::stoi(str_msg, &pos);
      if (pos == str_msg.size()) {

        // construct and serialize ROS message
        std_msgs::msg::Int32 msg;
        msg.data = int_msg;
        serializeRosMessage(msg, serialized_msg);

        ros_msg_type = "std_msgs/msg/Int32";
        found_primitive = true;
      }
    } catch (const std::invalid_argument& ex) {
    } catch (const std::out_of_range& ex) {
    }
  }

  // check for float
  if (!found_primitive) {
    std::size_t pos;
    try {
      const float float_msg = std::stof(str_msg, &pos);
      if (pos == str_msg.size()) {

        // construct and serialize ROS message
        std_msgs::msg::Float32 msg;
        msg.data = float_msg;
        serializeRosMessage(msg, serialized_msg);

        ros_msg_type = "std_msgs/msg/Float32";
        found_primitive = true;
      }
    } catch (const std::invalid_argument& ex) {
    } catch (const std::out_of_range& ex) {
    }
  }

  // fall back to string
  if (!found_primitive) {

    // construct and serialize ROS message
    std_msgs::msg::String msg;
    msg.data = str_msg;
    serializeRosMessage(msg, serialized_msg);
  }

  // if ROS message type has changed or if mapping is stale
  if (ros_msg_type != mqtt2ros.ros.msg_type || mqtt2ros.ros.is_stale) {

    mqtt2ros.ros.msg_type = ros_msg_type;
    RCLCPP_INFO(get_logger(),
                "ROS publisher message type on topic '%s' set to '%s'",
                mqtt2ros.ros.topic.c_str(), ros_msg_type.c_str());

    // recreate generic publisher
    try {
      mqtt2ros.ros.publisher = create_generic_publisher(
        mqtt2ros.ros.topic, ros_msg_type, mqtt2ros.ros.queue_size);
    } catch (rclcpp::exceptions::RCLError& e) {
      RCLCPP_ERROR(get_logger(), "Failed to create generic publisher: %s",
                   e.what());
      return;
    }
    mqtt2ros.ros.is_stale = false;
  }

  // publish
  RCLCPP_DEBUG(
    get_logger(),
    "Sending ROS message of type '%s' from MQTT broker to ROS topic '%s' ...",
    mqtt2ros.ros.msg_type.c_str(), mqtt2ros.ros.topic.c_str());
  mqtt2ros.ros.publisher->publish(serialized_msg);
}


void MqttClient::connected(const std::string& cause) {

  (void) cause; // Avoid compiler warning for unused parameter.

  is_connected_ = true;
  std::string as_client =
    client_config_.id.empty()
      ? ""
      : std::string(" as '") + client_config_.id + std::string("'");
  RCLCPP_INFO(get_logger(), "Connected to broker at '%s'%s",
              client_->get_server_uri().c_str(), as_client.c_str());

  // subscribe MQTT topics
  for (const auto& [mqtt_topic, mqtt2ros] : mqtt2ros_) {
    std::string mqtt_topic_to_subscribe = mqtt_topic;
    if (!mqtt2ros.primitive)  // subscribe topics for ROS message types first
      mqtt_topic_to_subscribe = kRosMsgTypeMqttTopicPrefix + mqtt_topic;
    client_->subscribe(mqtt_topic_to_subscribe, mqtt2ros.mqtt.qos);
    RCLCPP_INFO(get_logger(), "Subscribed MQTT topic '%s'", mqtt_topic_to_subscribe.c_str());
  }
}


void MqttClient::connection_lost(const std::string& cause) {

  (void) cause; // Avoid compiler warning for unused parameter.

  RCLCPP_ERROR(get_logger(),
               "Connection to broker lost, will try to reconnect...");
  is_connected_ = false;
  connect();
}


bool MqttClient::isConnected() {

  return is_connected_;
}


void MqttClient::isConnectedService(
  mqtt_client_interfaces::srv::IsConnected::Request::SharedPtr request,
  mqtt_client_interfaces::srv::IsConnected::Response::SharedPtr response) {

  (void) request; // Avoid compiler warning for unused parameter.
  response->connected = isConnected();
}

void MqttClient::newRos2MqttBridge(
  mqtt_client_interfaces::srv::NewRos2MqttBridge::Request::SharedPtr request,
  mqtt_client_interfaces::srv::NewRos2MqttBridge::Response::SharedPtr response){

  // add mapping definition to ros2mqtt_
  Ros2MqttInterface& ros2mqtt = ros2mqtt_[request->ros_topic];
  ros2mqtt.ros.is_stale = true;
  ros2mqtt.mqtt.topic = request->mqtt_topic;
  ros2mqtt.primitive = request->primitive;
  ros2mqtt.stamped = request->inject_timestamp;
  ros2mqtt.ros.queue_size = request->ros_queue_size;
  ros2mqtt.mqtt.qos = request->mqtt_qos;
  ros2mqtt.mqtt.retained = request->mqtt_retained;

  if (ros2mqtt.stamped && ros2mqtt.primitive) {
        RCLCPP_WARN(
          get_logger(),
          "Timestamp will not be injected into primitive messages on ROS "
          "topic '%s'",
          request->ros_topic.c_str());
        ros2mqtt.stamped = false;
  }

  RCLCPP_INFO(get_logger(), "Bridging %sROS topic '%s' to MQTT topic '%s' %s",
                  ros2mqtt.primitive ? "primitive " : "", request->ros_topic.c_str(),
                  ros2mqtt.mqtt.topic.c_str(),
                  ros2mqtt.stamped ? "and measuring latency" : "");

  // (re-)setup ROS subscriptions
  setupSubscriptions();

  response->success = true;
}

void MqttClient::newMqtt2RosBridge(
  mqtt_client_interfaces::srv::NewMqtt2RosBridge::Request::SharedPtr request,
  mqtt_client_interfaces::srv::NewMqtt2RosBridge::Response::SharedPtr response){

  // add mapping definition to mqtt2ros_
  Mqtt2RosInterface& mqtt2ros = mqtt2ros_[request->mqtt_topic];
  mqtt2ros.ros.is_stale = true;
  mqtt2ros.ros.topic = request->ros_topic;
  mqtt2ros.primitive = request->primitive;
  mqtt2ros.mqtt.qos = request->mqtt_qos;
  mqtt2ros.ros.queue_size = request->ros_queue_size;
  mqtt2ros.ros.latched = request->ros_latched;
  if (mqtt2ros.ros.latched) {
    RCLCPP_WARN(get_logger(),
                    fmt::format("Parameter 'bridge.mqtt2ros.{}.advanced.ros.latched' is ignored "
                    "since ROS 2 does not easily support latched topics.", request->mqtt_topic).c_str());

  }

  RCLCPP_INFO(get_logger(), "Bridging MQTT topic '%s' to %sROS topic '%s'",
                  request->mqtt_topic.c_str(), mqtt2ros.primitive ? "primitive " : "",
                  mqtt2ros.ros.topic.c_str());

  // subscribe to the MQTT topic
  std::string mqtt_topic_to_subscribe = request->mqtt_topic;
  if (!mqtt2ros.primitive)
    mqtt_topic_to_subscribe = kRosMsgTypeMqttTopicPrefix + request->mqtt_topic;
  client_->subscribe(mqtt_topic_to_subscribe, mqtt2ros.mqtt.qos);
  RCLCPP_INFO(get_logger(), "Subscribed MQTT topic '%s'", mqtt_topic_to_subscribe.c_str());

  response->success = true;
}

void MqttClient::message_arrived(mqtt::const_message_ptr mqtt_msg) {

  // instantly take arrival timestamp
  rclcpp::Time arrival_stamp(
    builtin_interfaces::msg::Time(rclcpp::Clock(RCL_SYSTEM_TIME).now()));

  std::string mqtt_topic = mqtt_msg->get_topic();
  RCLCPP_DEBUG(get_logger(), "Received MQTT message on topic '%s'",
               mqtt_topic.c_str());

  // publish directly if primitive
  if (mqtt2ros_.count(mqtt_topic) > 0) {
    Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_topic];
    if (mqtt2ros.primitive) {
      mqtt2primitive(mqtt_msg);
      return;
    }
  }

  // else first check for ROS message type
  bool msg_contains_ros_msg_type =
    mqtt_topic.find(kRosMsgTypeMqttTopicPrefix) != std::string::npos;
  if (msg_contains_ros_msg_type) {

    // copy message type to generic message buffer
    auto& payload = mqtt_msg->get_payload_ref();
    uint32_t payload_length = static_cast<uint32_t>(payload.size());
    rclcpp::SerializedMessage serialized_ros_msg_type(payload_length);
    std::memcpy(serialized_ros_msg_type.get_rcl_serialized_message().buffer,
                &(payload[0]), payload_length);
    serialized_ros_msg_type.get_rcl_serialized_message().buffer_length = payload_length;

    // deserialize ROS message type
    mqtt_client_interfaces::msg::RosMsgType ros_msg_type;
    deserializeRosMessage(serialized_ros_msg_type, ros_msg_type);

    // reconstruct corresponding MQTT data topic
    std::string mqtt_data_topic = mqtt_topic;
    mqtt_data_topic.erase(mqtt_data_topic.find(kRosMsgTypeMqttTopicPrefix),
                          kRosMsgTypeMqttTopicPrefix.length());
    Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_data_topic];

    // if ROS message type has changed or if mapping is stale
    if (ros_msg_type.name != mqtt2ros.ros.msg_type || mqtt2ros.ros.is_stale) {

      mqtt2ros.ros.msg_type = ros_msg_type.name;
      mqtt2ros.stamped = ros_msg_type.stamped;
      RCLCPP_INFO(get_logger(),
                  "ROS publisher message type on topic '%s' set to '%s'",
                  mqtt2ros.ros.topic.c_str(), ros_msg_type.name.c_str());

      // recreate generic publisher
      try {
        mqtt2ros.ros.publisher = create_generic_publisher(
          mqtt2ros.ros.topic, ros_msg_type.name, mqtt2ros.ros.queue_size);
      } catch (rclcpp::exceptions::RCLError& e) {
        RCLCPP_ERROR(get_logger(), "Failed to create generic publisher: %s",
                     e.what());
        return;
      }
      mqtt2ros.ros.is_stale = false;

      // subscribe to MQTT topic with actual ROS messages
      client_->subscribe(mqtt_data_topic, mqtt2ros.mqtt.qos);
      RCLCPP_DEBUG(get_logger(), "Subscribed MQTT topic '%s'",
                   mqtt_data_topic.c_str());
    }

  } else {

    // publish ROS message, if publisher initialized
    if (!mqtt2ros_[mqtt_topic].ros.msg_type.empty()) {
      mqtt2ros(mqtt_msg, arrival_stamp);
    } else {
      RCLCPP_WARN(
        get_logger(),
        "ROS publisher for data from MQTT topic '%s' is not yet initialized: "
        "ROS message type not yet known",
        mqtt_topic.c_str());
    }
  }
}


void MqttClient::delivery_complete(mqtt::delivery_token_ptr token) {

  (void) token; // Avoid compiler warning for unused parameter.
}


void MqttClient::on_success(const mqtt::token& token) {

  (void) token; // Avoid compiler warning for unused parameter.
  is_connected_ = true;
}


void MqttClient::on_failure(const mqtt::token& token) {

  RCLCPP_ERROR(
    get_logger(),
    "Connection to broker failed (return code %d), will automatically "
    "retry...",
    token.get_return_code());
  is_connected_ = false;
}

}  // namespace mqtt_client
