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

//#include <rclcpp/rclcpp.hpp>
#include <cstdint>
#include <cstring>
#include <vector>
#include <iostream>

#include <mqtt_client/MqttClient.h>
//#include <mqtt_client/RosMsgType.h>
//#include <pluginlib/class_list_macros.hpp>
#include <std_msgs/msg/float64.hpp>
#include <std_msgs/msg/string.hpp>
//#include <xmlrpcpp/XmlRpcException.h>
//#include <xmlrpcpp/XmlRpcValue.h>
#include "rclcpp/serialization.hpp"
#include "rclcpp/time.hpp"



// PLUGINLIB_EXPORT_CLASS(mqtt_client::MqttClient, rclcpp::Node)


namespace mqtt_client {


const std::string MqttClient::kRosMsgTypeMqttTopicPrefix =
  "mqtt_client/ros_msg_type/";

const std::string MqttClient::kLatencyRosTopicPrefix = "latencies/";


MqttClient::MqttClient() : MqttClient::Node("mqtt_client") {

  // get nodehandles
  //node_handle_ = this->getMTNodeHandle();
  //private_node_handle_ = this->getMTPrivateNodeHandle();
  
  loadParameters();
  setup();
}


void MqttClient::loadParameters() {

  // load broker parameters from parameter server
  std::string broker_tls_ca_certificate;
  loadParameter("broker/host", broker_config_.host, "localhost");
  loadParameter("broker/port", broker_config_.port, 1883);
  if (loadParameter("broker/user", broker_config_.user)) {
    loadParameter("broker/pass", broker_config_.pass, "");
  }
  if (loadParameter("broker/tls/enabled", broker_config_.tls.enabled, false)) {
    loadParameter("broker/tls/ca_certificate", broker_tls_ca_certificate,
                  "/etc/ssl/certs/ca-certificates.crt");
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

  //--------------------------------------------------------------------------
  // Set topic parameters without xmlrlcpp

  loadParameter("bridge/ros2mqtt_ros_topic", bridge_config_.ros2mqtt_ros_topic, "/ping");
  loadParameter("bridge/ros2mqtt_mqtt_topic", bridge_config_.ros2mqtt_mqtt_topic, "pingpong");
  loadParameter("bridge/ros2mqtt_inject_timestamp", bridge_config_.ros2mqtt_inject_timestamp, false);
  loadParameter("bridge/mqtt2ros_mqtt_topic", bridge_config_.mqtt2ros_mqtt_topic, "pingpong");
  loadParameter("bridge/mqtt2ros_ros_topic", bridge_config_.mqtt2ros_ros_topic, "/pong");

  // ros2mqtt[k]/ros_topic and ros2mqtt[k]/mqtt_topic
  std::string& ros_topic = bridge_config_.ros2mqtt_ros_topic;
  Ros2MqttInterface& ros2mqtt = ros2mqtt_[ros_topic];
  ros2mqtt.mqtt.topic = std::string(bridge_config_.ros2mqtt_mqtt_topic);

  // mqtt2ros[k]/mqtt_topic and mqtt2ros[k]/ros_topic
  std::string& mqtt_topic = bridge_config_.mqtt2ros_mqtt_topic;
  Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_topic];
  mqtt2ros.ros.topic = std::string(bridge_config_.mqtt2ros_ros_topic);

  // ros2mqtt[k]/inject_timestamp
  ros2mqtt.stamped = bridge_config_.ros2mqtt_inject_timestamp;


  /*
  // load bridge parameters from parameter server
  XmlRpc::XmlRpcValue bridge;
  if (!private_node_handle_.getParam("bridge", bridge)) {
    NODELET_ERROR("Parameter 'bridge' is required");
    exit(EXIT_FAILURE);
  }

  // parse bridge parameters
  try {

    // ros2mqtt
    if (bridge.hasMember("ros2mqtt")) {

      // loop over array
      XmlRpc::XmlRpcValue& ros2mqtt_params = bridge["ros2mqtt"];
      for (int k = 0; k < ros2mqtt_params.size(); k++) {

        if (ros2mqtt_params[k].hasMember("ros_topic") &&
            ros2mqtt_params[k].hasMember("mqtt_topic")) {

          // ros2mqtt[k]/ros_topic and ros2mqtt[k]/mqtt_topic
          std::string& ros_topic = ros2mqtt_params[k]["ros_topic"];
          Ros2MqttInterface& ros2mqtt = ros2mqtt_[ros_topic];
          ros2mqtt.mqtt.topic = std::string(ros2mqtt_params[k]["mqtt_topic"]);

          // ros2mqtt[k]/inject_timestamp
          if (ros2mqtt_params[k].hasMember("inject_timestamp"))
            ros2mqtt.stamped = ros2mqtt_params[k]["inject_timestamp"];

          // ros2mqtt[k]/advanced
          if (ros2mqtt_params[k].hasMember("advanced")) {
            XmlRpc::XmlRpcValue& advanced_params =
              ros2mqtt_params[k]["advanced"];
            if (advanced_params.hasMember("ros")) {
              if (advanced_params["ros"].hasMember("queue_size"))
                ros2mqtt.ros.queue_size = advanced_params["ros"]["queue_size"];
            }
            if (advanced_params.hasMember("mqtt")) {
              if (advanced_params["mqtt"].hasMember("qos"))
                ros2mqtt.mqtt.qos = advanced_params["mqtt"]["qos"];
              if (advanced_params["mqtt"].hasMember("retained"))
                ros2mqtt.mqtt.retained = advanced_params["mqtt"]["retained"];
            }
          }

          NODELET_INFO("Bridging ROS topic '%s' to MQTT topic '%s'",
                       ros_topic.c_str(), ros2mqtt.mqtt.topic.c_str());
        } else {
          NODELET_WARN(
            "Parameter 'bridge/ros2mqtt[%d]' is missing subparameter "
            "'ros_topic' or 'mqtt_topic', will be ignored",
            k);
        }
      }
    }

    // mqtt2ros
    if (bridge.hasMember("mqtt2ros")) {

      // loop over array
      XmlRpc::XmlRpcValue& mqtt2ros_params = bridge["mqtt2ros"];
      for (int k = 0; k < mqtt2ros_params.size(); k++) {


        if (mqtt2ros_params[k].hasMember("mqtt_topic") &&
            mqtt2ros_params[k].hasMember("ros_topic")) {

          // mqtt2ros[k]/mqtt_topic and mqtt2ros[k]/ros_topic
          std::string& mqtt_topic = mqtt2ros_params[k]["mqtt_topic"];
          Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_topic];
          mqtt2ros.ros.topic = std::string(mqtt2ros_params[k]["ros_topic"]);

          // mqtt2ros[k]/advanced
          if (mqtt2ros_params[k].hasMember("advanced")) {
            XmlRpc::XmlRpcValue& advanced_params =
              mqtt2ros_params[k]["advanced"];
            if (advanced_params.hasMember("mqtt")) {
              if (advanced_params["mqtt"].hasMember("qos"))
                mqtt2ros.mqtt.qos = advanced_params["mqtt"]["qos"];
            }
            if (advanced_params.hasMember("ros")) {
              if (advanced_params["ros"].hasMember("queue_size"))
                mqtt2ros.ros.queue_size = advanced_params["ros"]["queue_size"];
              if (advanced_params["ros"].hasMember("latched"))
                mqtt2ros.ros.latched = advanced_params["ros"]["latched"];
            }
          }

          NODELET_INFO("Bridging MQTT topic '%s' to ROS topic '%s'",
                       mqtt_topic.c_str(), mqtt2ros.ros.topic.c_str());
        } else {
          NODELET_WARN(
            "Parameter 'bridge/mqtt2ros[%d]' is missing subparameter "
            "'mqtt_topic' or 'ros_topic', will be ignored",
            k);
        }
      }
    }

    if (ros2mqtt_.empty() && mqtt2ros_.empty()) {
      NODELET_ERROR("No valid ROS-MQTT bridge found in parameter 'bridge'");
      exit(EXIT_FAILURE);
    }

  } catch (XmlRpc::XmlRpcException e) {
    NODELET_ERROR("Parameter 'bridge' could not be parsed (XmlRpc %d: %s)",
                  e.getCode(), e.getMessage().c_str());
    exit(EXIT_FAILURE);
  }
  */
}


bool MqttClient::loadParameter(const std::string& key, std::string& value) {
  bool found = private_node_handle_.get_parameter(key, value);
  if (found)
    RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), "Retrieved parameter '%s' = '%s'", key.c_str(),
                  value.c_str());
  return found;
}


bool MqttClient::loadParameter(const std::string& key, std::string& value,
                               const std::string& default_value) {
  bool found =
    private_node_handle_.get_parameter_or(key, value, default_value);
  if (!found)
    RCLCPP_WARN(rclcpp::get_logger("rclcpp"), "Parameter '%s' not set, defaulting to '%s'", key.c_str(),
                 default_value.c_str());
  if (found)
    RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), "Retrieved parameter '%s' = '%s'", key.c_str(),
                  value.c_str());
  return found;
}


//std::filesystem::path MqttClient::resolvePath(const std::string& path_string) {
rcpputils::fs::path MqttClient::resolvePath(const std::string& path_string) {

  rcpputils::fs::path path(path_string);
  if (path_string.empty()) return path;
  // path functions are missing in ROS2
  /*if (!path.has_root_path()) {
    std::string ros_home;
    rclcpp::get_environment_variable(ros_home, "ROS_HOME");
    if (ros_home.empty())
      ros_home = rcpputils::fs::current_path().string();
    path = rcpputils::fs::path(ros_home);
    path.append(path_string);
  }*/
  if (!rcpputils::fs::exists(path))
    RCLCPP_WARN(rclcpp::get_logger("rclcpp"), "Requested path '%s' does not exist",
                 path.string());
  return path;
}


void MqttClient::setup() {

  // initialize MQTT client
  setupClient();

  // connect to MQTT broker
  connect();

  // create ROS service server
  /*is_connected_service_ = private_node_handle_.advertiseService(
    "is_connected", &MqttClient::isConnectedService, this);*/
  
  rclcpp::Service<srv::IsConnected>::SharedPtr service =
    is_connected_service_->create_service<srv::IsConnected>("is_connected", &MqttClient::isConnectedService);

  // create ROS subscribers
  for (auto& ros2mqtt_p : ros2mqtt_) {
    const std::string& ros_topic = ros2mqtt_p.first;
    Ros2MqttInterface& ros2mqtt = ros2mqtt_p.second;
    const std::string& mqtt_topic = ros2mqtt.mqtt.topic;
    /*ros2mqtt.ros.subscriber =
      private_node_handle_.subscribe<topic_tools::ShapeShifter>(
        ros_topic, ros2mqtt.ros.queue_size,
        std::bind(&MqttClient::ros2mqtt, this, std::placeholder::_1, ros_topic));
    RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), "Subscribed ROS topic '%s'", ros_topic.c_str());*/
    ros2mqtt.ros.subscriber = create_subscription<std_msgs::msg::String>(ros_topic, rclcpp::QoS(), std::bind(&MqttClient::ros2mqtt, this, std::placeholders::_1, ros_topic)); //TODO qos value
    RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), "Subscribed ROS topic '%s'", ros_topic.c_str());
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
  //TODO why is tls.key used and not keys
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

  // create MQTT client
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


void MqttClient::connect() {

  std::string as_client =
    client_config_.id.empty()
      ? ""
      : std::string(" as '") + client_config_.id + std::string("'");
  RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Connecting to broker at '%s'%s ...",
               client_->get_server_uri().c_str(), as_client.c_str());

  try {
    client_->connect(connect_options_, nullptr, *this);
  } catch (const mqtt::exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Connection to broker failed: %s", e.what());
    exit(EXIT_FAILURE);
  }
}


void MqttClient::ros2mqtt(
                          const std::string& ros_topic) {

  Ros2MqttInterface& ros2mqtt = ros2mqtt_[ros_topic];

  // gather information on ROS message type in special ROS message
  /*RosMsgType ros_msg_type;
  ros_msg_type.md5 = ros_msg->getMD5Sum();
  ros_msg_type.name = ros_msg->getDataType();
  ros_msg_type.definition = ros_msg->getMessageDefinition();*/

  std::string ros_msg_type = "String";
  RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), "Received ROS message of type '%s' on topic '%s'",
                ros_msg_type, ros_topic.c_str());

  // serialize ROS message type to buffer
  //uint32_t msg_type_length =
  //  rclcpp::serialization::serializationLength(std_msgs::msg::String);
  std::vector<uint8_t> msg_type_buffer;
  //msg_type_buffer.resize(msg_type_length);
  //rclcpp::serialization::OStream msg_type_stream(msg_type_buffer.data(),
  //                                            msg_type_length);
  //rclcpp::serialization::serialize(msg_type_stream, std_msgs::msg::String);

  // ### ROS2 Version of Serialization
  rclcpp::SerializedMessage serialized_msg_type_;
  auto string_msg = std::make_shared<std_msgs::msg::String>();
  //string_msg->data = "Hello World:" + std::to_string(count_++);

  auto message_header_length = 8u;
  auto message_payload_length = static_cast<size_t>(string_msg->data.size());
  serialized_msg_type_.reserve(message_header_length + message_payload_length);

  static rclcpp::Serialization<std_msgs::msg::String> serializer;
  serializer.serialize_message(msg_type_buffer.data(), &serialized_msg_type_);
  // ###

  // send ROS message type information to MQTT broker
  std::string mqtt_topic = kRosMsgTypeMqttTopicPrefix + ros2mqtt.mqtt.topic;
  uint32_t msg_type_length = (uint32_t) message_payload_length;
  try {
    RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), "Sending ROS message type to MQTT broker on topic '%s' ...",
                  mqtt_topic.c_str());
    mqtt::message_ptr mqtt_msg =
      mqtt::make_message(mqtt_topic, msg_type_buffer.data(), msg_type_length,
                         ros2mqtt.mqtt.qos, true);
    client_->publish(mqtt_msg);
  } catch (const mqtt::exception& e) {
    RCLCPP_WARN(rclcpp::get_logger("rclcpp"), 
      "Publishing ROS message type information to MQTT topic '%s' failed: %s",
      mqtt_topic.c_str(), e.what());
  }

  // serialize ROS message to buffer
  //uint32_t msg_length = rclcpp::serialization::serializationLength(*ros_msg);
  std::vector<uint8_t> msg_buffer;
  //msg_buffer.resize(msg_length);
  //rclcpp::serialization::OStream msg_stream(msg_buffer.data(), msg_length);
  //rclcpp::serialization::serialize(msg_stream, *ros_msg);
  
  // ### ROS2 Version of Serialization
  rclcpp::SerializedMessage serialized_msg_;
  //auto string_msg = std::make_shared<std_msgs::msg::String>();
  //string_msg->data = "Hello World:" + std::to_string(count_++);

  //auto message_header_length = 8u;
  //auto message_payload_length = static_cast<size_t>(string_msg->data.size());
  serialized_msg_.reserve(message_header_length + message_payload_length);

  //static rclcpp::Serialization<std_msgs::msg::String> serializer;
  serializer.serialize_message(msg_buffer.data(), &serialized_msg_);
  // ###

  // build MQTT payload for ROS message (R) as [1, S, R] or [0, R]
  // where first item = 1 if timestamp (S) is included
  uint32_t payload_length = 1 + message_payload_length;
  uint32_t msg_offset = 1;
  std::vector<uint8_t> payload_buffer;
  if (ros2mqtt.stamped) {
    /*
    // serialize current timestamp
    rclcpp::Time stamp_wall = rclcpp::Node::now();
    rclcpp::Time stamp(stamp_wall.seconds(), stamp_wall.nanoseconds());
    rclcpp::Time time_zero(0, 0);
    if (stamp == time_zero)
      RCLCPP_WARN(rclcpp::get_logger("rclcpp"), 
        "Injected ROS time 0 into MQTT payload on topic %s, is a ROS clock "
        "running?",
        ros2mqtt.mqtt.topic.c_str());
    
    //builtin_interfaces::msg::Time time_converted;
    //time_converted.sec = stamp.seconds();
    //time_converted.nanosec = stamp.nanoseconds();
    //Converting to a msg/Time ROS1 message also does not work for the serialization
    rclcpp::SerializedMessage stamp_stream;
    auto stamp_length = static_cast<size_t>(stamp.size());
    stamp_stream.reserve(stamp_length);
    //static rclcpp::Serialization<std_msgs::msg::String> serializer;
    serializer.serialize_message(stamp, &stamp_stream);
    // rclcpp::time cant be serialized (needs to be a ros message)

    //uint32_t stamp_length = rclcpp::serialization::serializationLength(stamp);
    //std::vector<uint8_t> stamp_buffer;
    //stamp_buffer.resize(stamp_length);
    //rclcpp::serialization::OStream stamp_stream(stamp_buffer.data(), stamp_length);
    //rclcpp::serialization::serialize(stamp_stream, stamp);
    */
    /*
    // inject timestamp into payload
    payload_length += stamp_length;
    msg_offset += stamp_length;
    payload_buffer.resize(payload_length);
    payload_buffer[0] = 1;
    payload_buffer.insert(payload_buffer.begin() + 1,
                          std::make_move_iterator(stamp_buffer.begin()),
                          std::make_move_iterator(stamp_buffer.end()));
    */
    payload_buffer.resize(payload_length);
    payload_buffer[0] = 1;

  } else {

    payload_buffer.resize(payload_length);
    payload_buffer[0] = 0;
  }
  // add ROS message to payload
  payload_buffer.insert(payload_buffer.begin() + msg_offset,
                        std::make_move_iterator(msg_buffer.begin()),
                        std::make_move_iterator(msg_buffer.end()));

  // send ROS message to MQTT broker
  mqtt_topic = ros2mqtt.mqtt.topic;
  try {
    RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), 
      "Sending ROS message of type '%s' to MQTT broker on topic '%s' ...",
      ros_msg_type, mqtt_topic.c_str());
    mqtt::message_ptr mqtt_msg =
      mqtt::make_message(mqtt_topic, payload_buffer.data(), payload_length,
                         ros2mqtt.mqtt.qos, ros2mqtt.mqtt.retained);
    client_->publish(mqtt_msg);
  } catch (const mqtt::exception& e) {
    RCLCPP_WARN(rclcpp::get_logger("rclcpp"), 
      "Publishing ROS message type information to MQTT topic '%s' failed: %s",
      mqtt_topic.c_str(), e.what());
  }
}


void MqttClient::mqtt2ros(mqtt::const_message_ptr mqtt_msg) {

  std::string mqtt_topic = mqtt_msg->get_topic();
  Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_topic];
  auto& payload = mqtt_msg->get_payload_ref();
  uint32_t payload_length = static_cast<uint32_t>(payload.size());

  // determine whether timestamp is injected by reading first element
  bool stamped = (static_cast<uint8_t>(payload[0]) > 0);

  // read MQTT payload for ROS message (R) as [1, S, R] or [0, R]
  // where first item = 1 if timestamp (S) is included
  uint32_t msg_length = payload_length - 1;
  uint32_t msg_offset = 1;

  /*
  // if stamped, compute latency
  if (stamped) {

    // copy timestamp from MQTT message to buffer
    rclcpp::Time stamp;
    uint32_t stamp_length = rclcpp::serialization::serializationLength(stamp);
    std::vector<uint8_t> stamp_buffer;
    stamp_buffer.resize(stamp_length);
    std::memcpy(stamp_buffer.data(), &(payload[1]), stamp_length);

    // deserialize stamp
    rclcpp::serialization::IStream stamp_stream(stamp_buffer.data(), stamp_length);
    rclcpp::serialization::deserialize(stamp_stream, stamp);

    // compute ROS2MQTT latency
    // rclcpp::WallTimer now_wall = rclcpp::WallTime::now();
    //rclcpp::Time now_wall = std::chrono::system_clock;
    //rclcpp::Time now(now_wall.sec, now_wall.nsec);#
    rclcpp::Time now = rclcpp::Clock().now();
    if (now.isZero())
      RCLCPP_WARN(rclcpp::get_logger("rclcpp"), 
        "Cannot compute latency for MQTT topic %s when ROS time is 0, is a ROS "
        "clock running?",
        mqtt_topic.c_str());
    rclcpp::Duration latency = now - stamp;
    std_msgs::msg::Float64 latency_msg;
    latency_msg.data = latency.seconds();

    // publish latency
    if (mqtt2ros.ros.latency_publisher.getTopic().empty()) {
      std::string latency_topic = kLatencyRosTopicPrefix + mqtt2ros.ros.topic;
      mqtt2ros.ros.latency_publisher =
        private_node_handle_.advertise<std_msgs::Float64>(latency_topic, 1);
    }
    std::string latency_topic = kLatencyRosTopicPrefix + mqtt2ros.ros.topic;
    mqtt2ros.ros.latency_publisher = create_publisher<std_msgs::msg::String>(latency_topic, 1);
    mqtt2ros.ros.latency_publisher->publish(latency_msg);

    msg_length -= stamp_length;
    msg_offset += stamp_length;
  }*/


  // copy ROS message from MQTT message to buffer
  std::vector<uint8_t> msg_buffer;
  msg_buffer.resize(msg_length);
  std::memcpy(msg_buffer.data(), &(payload[msg_offset]), msg_length);

  // publish via ShapeShifter
  RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), 
    "Sending ROS message of type '%s' from MQTT broker to ROS topic '%s' ...",
    // mqtt2ros.ros.shape_shifter.getDataType().c_str(),
    mqtt2ros.ros.topic.c_str());
  // rclcpp::serialization::OStream msg_stream(msg_buffer.data(), msg_length);
  rclcpp::SerializedMessage serialized_msg_;
  static rclcpp::Serialization<std_msgs::msg::String> serializer;
  serializer.serialize_message(msg_buffer.data(), &serialized_msg_);
  // mqtt2ros.ros.shape_shifter.read(msg_stream);
  // mqtt2ros.ros.publisher.publish(mqtt2ros.ros.shape_shifter);
  mqtt2ros.ros.publisher = create_publisher<std_msgs::msg::String>(mqtt2ros.ros.topic, 1);
  mqtt2ros.ros.publisher->publish(serialized_msg_);
}


void MqttClient::connected(const std::string& cause) {

  is_connected_ = true;
  std::string as_client =
    client_config_.id.empty()
      ? ""
      : std::string(" as '") + client_config_.id + std::string("'");
  RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Connected to broker at '%s'%s",
               client_->get_server_uri().c_str(), as_client.c_str());

  // subscribe MQTT topics
  for (auto& mqtt2ros_p : mqtt2ros_) {
    Mqtt2RosInterface& mqtt2ros = mqtt2ros_p.second;
    std::string mqtt_topic = kRosMsgTypeMqttTopicPrefix + mqtt2ros_p.first;
    client_->subscribe(mqtt_topic, mqtt2ros.mqtt.qos);
    RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), "Subscribed MQTT topic '%s'", mqtt_topic.c_str());
  }
}


void MqttClient::connection_lost(const std::string& cause) {

  RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Connection to broker lost, will try to reconnect...");
  is_connected_ = false;
  connect();
}


bool MqttClient::isConnected() {

  return is_connected_;
}


bool MqttClient::isConnectedService(IsConnected::Request& request,
                                    IsConnected::Response& response) { //TODO Problem is the missing header in MqttClient.h

  response.connected = isConnected();
  return true;
}


void MqttClient::message_arrived(mqtt::const_message_ptr mqtt_msg) {

  //RosMsgType ros_msg_type = std_msgs::msg::String;

  std::string mqtt_topic = mqtt_msg->get_topic();
  RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), "Received MQTT message on topic '%s'", mqtt_topic.c_str());
  auto& payload = mqtt_msg->get_payload_ref();
  uint32_t payload_length = static_cast<uint32_t>(payload.size());

  bool msg_contains_ros_msg_type =
    mqtt_topic.find(kRosMsgTypeMqttTopicPrefix) != std::string::npos;
  if (msg_contains_ros_msg_type) {

    // copy ROS message type from MQTT message to buffer
    //RosMsgType ros_msg_type;
    std::vector<uint8_t> msg_type_buffer;
    msg_type_buffer.resize(payload_length);
    std::memcpy(msg_type_buffer.data(), &(payload[0]), payload_length);

    rclcpp::SerializedMessage serialized_msg_;
    std::memcpy(&serialized_msg_, &(payload[0]), payload_length);

    // deserialize ROS message type
    //rclcpp::serialization::IStream msg_type_stream(msg_type_buffer.data(),
    //                                            payload_length);
    std_msgs::msg::String string_msg;
    static rclcpp::Serialization<std_msgs::msg::String> serializer;
    serializer.deserialize_message(&serialized_msg_, &string_msg);

    // reconstruct corresponding MQTT data topic
    std::string mqtt_data_topic = mqtt_topic;
    mqtt_data_topic.erase(mqtt_data_topic.find(kRosMsgTypeMqttTopicPrefix),
                          kRosMsgTypeMqttTopicPrefix.length());
    Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_data_topic];

    // if ROS message type has changed
    /*if (ros_msg_type.md5 != mqtt2ros.ros.shape_shifter.getMD5Sum()) {

      // configure ShapeShifter
      mqtt2ros.ros.shape_shifter.morph(ros_msg_type.md5, ros_msg_type.name,
                                       ros_msg_type.definition, "");

      // advertise with ROS publisher
      mqtt2ros.ros.publisher.shutdown();
      mqtt2ros.ros.publisher = mqtt2ros.ros.shape_shifter.advertise(
        node_handle_, mqtt2ros.ros.topic, mqtt2ros.ros.queue_size,
        mqtt2ros.ros.latched);
      //mqtt2ros.ros.publisher = create_publisher<std_msgs::msg::String>(mqtt2ros.ros.topic, mqtt2ros.ros.queue_size);

      RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "ROS publisher message type on topic '%s' set to '%s'",
                   mqtt2ros.ros.topic.c_str(), ros_msg_type.name.c_str());

      // subscribe to MQTT topic with actual ROS messages
      client_->subscribe(mqtt_data_topic, mqtt2ros.mqtt.qos);
      RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), "Subscribed MQTT topic '%s'", mqtt_data_topic.c_str());
      
  }*/
      mqtt2ros.ros.publisher = create_publisher<std_msgs::msg::String>(mqtt2ros.ros.topic, 1);
      mqtt2ros.ros.publisher->publish(serialized_msg_);
      //RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "ROS publisher message type on topic '%s' set to '%s'",
      //             mqtt2ros.ros.topic.c_str(), std_msgs::msg::String.c_str());

      RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "ROS publisher message type on topic '%s' set to '%s'",
                   mqtt2ros.ros.topic.c_str());


      // subscribe to MQTT topic with actual ROS messages
      client_->subscribe(mqtt_data_topic, mqtt2ros.mqtt.qos);
      RCLCPP_DEBUG(rclcpp::get_logger("rclcpp"), "Subscribed MQTT topic '%s'", mqtt_data_topic.c_str());

    

  } else {
    /*
    // publish ROS message, if publisher initialized
    if (!mqtt2ros_[mqtt_topic].ros.publisher.get_topic_name().empty()) {
      mqtt2ros(mqtt_msg);
    } else {
      RCLCPP_WARN(rclcpp::get_logger("rclcpp"),
        "ROS publisher for data from MQTT topic '%s' is not yet initialized: "
        "ROS message type not yet known",
        mqtt_topic.c_str());
    }*/
  }
}


void MqttClient::delivery_complete(mqtt::delivery_token_ptr token) {}


void MqttClient::on_success(const mqtt::token& token) {}


void MqttClient::on_failure(const mqtt::token& token) {

  RCLCPP_ERROR(rclcpp::get_logger("rclcpp"),
    "Connection to broker failed (return code %d), will automatically "
    "retry...",
    token.get_return_code());
}

}  // namespace mqtt_client
