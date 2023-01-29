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

#include <mqtt_client/MqttClient.h>
#include <mqtt_client/RosMsgType.h>
#include <pluginlib/class_list_macros.h>
#include <ros/message_traits.h>
#include <std_msgs/Bool.h>
#include <std_msgs/Char.h>
#include <std_msgs/Float32.h>
#include <std_msgs/Float64.h>
#include <std_msgs/Int16.h>
#include <std_msgs/Int32.h>
#include <std_msgs/Int64.h>
#include <std_msgs/Int8.h>
#include <std_msgs/String.h>
#include <std_msgs/UInt16.h>
#include <std_msgs/UInt32.h>
#include <std_msgs/UInt64.h>
#include <std_msgs/UInt8.h>
#include <xmlrpcpp/XmlRpcException.h>
#include <xmlrpcpp/XmlRpcValue.h>


PLUGINLIB_EXPORT_CLASS(mqtt_client::MqttClient, nodelet::Nodelet)


namespace mqtt_client {


const std::string MqttClient::kRosMsgTypeMqttTopicPrefix =
  "mqtt_client/ros_msg_type/";

const std::string MqttClient::kLatencyRosTopicPrefix = "latencies/";


/**
 * @brief Extracts string of primitive data types from ROS message.
 *
 * This is helpful to extract the actual data payload of a primitive ROS
 * message. If e.g. an std_msgs/String is serialized to a string
 * representation, it also contains the field name 'data'. This function
 * instead returns the underlying value as string.
 *
 * The following primitive ROS message types are supported:
 *   std_msgs/String
 *   std_msgs/Bool
 *   std_msgs/Char
 *   std_msgs/UInt8
 *   std_msgs/UInt16
 *   std_msgs/UInt32
 *   std_msgs/UInt64
 *   std_msgs/Int8
 *   std_msgs/Int16
 *   std_msgs/Int32
 *   std_msgs/Int64
 *   std_msgs/Float32
 *   std_msgs/Float64
 *
 * @param [in]  msg        generic ShapeShifter ROS message
 * @param [out] primitive  string representation of primitive message data
 *
 * @return true  if primitive ROS message type was found
 * @return false if ROS message type is not primitive
 */
bool primitiveRosMessageToString(const topic_tools::ShapeShifter::ConstPtr& msg,
                                 std::string& primitive) {

  bool found_primitive = true;
  const std::string& msg_type_md5 = msg->getMD5Sum();

  if (msg_type_md5 == ros::message_traits::MD5Sum<std_msgs::String>::value()) {
    primitive = msg->instantiate<std_msgs::String>()->data;
  } else if (msg_type_md5 ==
             ros::message_traits::MD5Sum<std_msgs::Bool>::value()) {
    primitive = msg->instantiate<std_msgs::Bool>()->data ? "true" : "false";
  } else if (msg_type_md5 ==
             ros::message_traits::MD5Sum<std_msgs::Char>::value()) {
    primitive = std::to_string(msg->instantiate<std_msgs::Char>()->data);
  } else if (msg_type_md5 ==
             ros::message_traits::MD5Sum<std_msgs::UInt8>::value()) {
    primitive = std::to_string(msg->instantiate<std_msgs::UInt8>()->data);
  } else if (msg_type_md5 ==
             ros::message_traits::MD5Sum<std_msgs::UInt16>::value()) {
    primitive = std::to_string(msg->instantiate<std_msgs::UInt16>()->data);
  } else if (msg_type_md5 ==
             ros::message_traits::MD5Sum<std_msgs::UInt32>::value()) {
    primitive = std::to_string(msg->instantiate<std_msgs::UInt32>()->data);
  } else if (msg_type_md5 ==
             ros::message_traits::MD5Sum<std_msgs::UInt64>::value()) {
    primitive = std::to_string(msg->instantiate<std_msgs::UInt64>()->data);
  } else if (msg_type_md5 ==
             ros::message_traits::MD5Sum<std_msgs::Int8>::value()) {
    primitive = std::to_string(msg->instantiate<std_msgs::Int8>()->data);
  } else if (msg_type_md5 ==
             ros::message_traits::MD5Sum<std_msgs::Int16>::value()) {
    primitive = std::to_string(msg->instantiate<std_msgs::Int16>()->data);
  } else if (msg_type_md5 ==
             ros::message_traits::MD5Sum<std_msgs::Int32>::value()) {
    primitive = std::to_string(msg->instantiate<std_msgs::Int32>()->data);
  } else if (msg_type_md5 ==
             ros::message_traits::MD5Sum<std_msgs::Int64>::value()) {
    primitive = std::to_string(msg->instantiate<std_msgs::Int64>()->data);
  } else if (msg_type_md5 ==
             ros::message_traits::MD5Sum<std_msgs::Float32>::value()) {
    primitive = std::to_string(msg->instantiate<std_msgs::Float32>()->data);
  } else if (msg_type_md5 ==
             ros::message_traits::MD5Sum<std_msgs::Float64>::value()) {
    primitive = std::to_string(msg->instantiate<std_msgs::Float64>()->data);
  } else {
    found_primitive = false;
  }

  return found_primitive;
}


void MqttClient::onInit() {

  // get nodehandles
  node_handle_ = this->getMTNodeHandle();
  private_node_handle_ = this->getMTPrivateNodeHandle();

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
    NODELET_WARN("Client buffer can not be enabled when client ID is empty");
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

          // ros2mqtt[k]/primitive
          if (ros2mqtt_params[k].hasMember("primitive"))
            ros2mqtt.primitive = ros2mqtt_params[k]["primitive"];

          // ros2mqtt[k]/inject_timestamp
          if (ros2mqtt_params[k].hasMember("inject_timestamp"))
            ros2mqtt.stamped = ros2mqtt_params[k]["inject_timestamp"];
          if (ros2mqtt.stamped && ros2mqtt.primitive) {
            NODELET_WARN(
              "Timestamp will not be injected into primitive messages on ROS "
              "topic '%s'",
              ros_topic.c_str());
            ros2mqtt.stamped = false;
          }

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

          NODELET_INFO("Bridging %sROS topic '%s' to MQTT topic '%s' %s",
                       ros2mqtt.primitive ? "primitive " : "",
                       ros_topic.c_str(), ros2mqtt.mqtt.topic.c_str(),
                       ros2mqtt.stamped ? "and measuring latency" : "");
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

          // mqtt2ros[k]/primitive
          if (mqtt2ros_params[k].hasMember("primitive"))
            mqtt2ros.primitive = mqtt2ros_params[k]["primitive"];

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

          NODELET_INFO(
            "Bridging MQTT topic '%s' to %sROS topic '%s'", mqtt_topic.c_str(),
            mqtt2ros.primitive ? "primitive " : "", mqtt2ros.ros.topic.c_str());
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
}


bool MqttClient::loadParameter(const std::string& key, std::string& value) {
  bool found = private_node_handle_.getParam(key, value);
  if (found)
    NODELET_DEBUG("Retrieved parameter '%s' = '%s'", key.c_str(),
                  value.c_str());
  return found;
}


bool MqttClient::loadParameter(const std::string& key, std::string& value,
                               const std::string& default_value) {
  bool found =
    private_node_handle_.param<std::string>(key, value, default_value);
  if (!found)
    NODELET_WARN("Parameter '%s' not set, defaulting to '%s'", key.c_str(),
                 default_value.c_str());
  if (found)
    NODELET_DEBUG("Retrieved parameter '%s' = '%s'", key.c_str(),
                  value.c_str());
  return found;
}


std::filesystem::path MqttClient::resolvePath(const std::string& path_string) {

  std::filesystem::path path(path_string);
  if (path_string.empty()) return path;
  if (!path.has_root_path()) {
    std::string ros_home;
    ros::get_environment_variable(ros_home, "ROS_HOME");
    if (ros_home.empty())
      ros_home = std::string(std::filesystem::current_path());
    path = std::filesystem::path(ros_home);
    path.append(path_string);
  }
  if (!std::filesystem::exists(path))
    NODELET_WARN("Requested path '%s' does not exist",
                 std::string(path).c_str());
  return path;
}


void MqttClient::setup() {

  // initialize MQTT client
  setupClient();

  // connect to MQTT broker
  connect();

  // create ROS service server
  is_connected_service_ = private_node_handle_.advertiseService(
    "is_connected", &MqttClient::isConnectedService, this);

  // create ROS subscribers
  for (auto& ros2mqtt_p : ros2mqtt_) {
    const std::string& ros_topic = ros2mqtt_p.first;
    Ros2MqttInterface& ros2mqtt = ros2mqtt_p.second;
    const std::string& mqtt_topic = ros2mqtt.mqtt.topic;
    ros2mqtt.ros.subscriber =
      private_node_handle_.subscribe<topic_tools::ShapeShifter>(
        ros_topic, ros2mqtt.ros.queue_size,
        boost::bind(&MqttClient::ros2mqtt, this, _1, ros_topic));
    NODELET_DEBUG("Subscribed ROS topic '%s'", ros_topic.c_str());
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
        client_config_.buffer.directory));
    } else {
      client_ = std::shared_ptr<mqtt::async_client>(
        new mqtt::async_client(uri, client_config_.id));
    }
  } catch (const mqtt::exception& e) {
    ROS_ERROR("Client could not be initialized: %s", e.what());
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
  NODELET_INFO("Connecting to broker at '%s'%s ...",
               client_->get_server_uri().c_str(), as_client.c_str());

  try {
    client_->connect(connect_options_, nullptr, *this);
  } catch (const mqtt::exception& e) {
    ROS_ERROR("Connection to broker failed: %s", e.what());
    exit(EXIT_FAILURE);
  }
}


void MqttClient::ros2mqtt(const topic_tools::ShapeShifter::ConstPtr& ros_msg,
                          const std::string& ros_topic) {

  Ros2MqttInterface& ros2mqtt = ros2mqtt_[ros_topic];
  std::string mqtt_topic = ros2mqtt.mqtt.topic;
  std::vector<uint8_t> payload_buffer;

  // gather information on ROS message type in special ROS message
  RosMsgType ros_msg_type;
  ros_msg_type.md5 = ros_msg->getMD5Sum();
  ros_msg_type.name = ros_msg->getDataType();
  ros_msg_type.definition = ros_msg->getMessageDefinition();

  NODELET_DEBUG("Received ROS message of type '%s' on topic '%s'",
                ros_msg_type.name.c_str(), ros_topic.c_str());

  if (ros2mqtt.primitive) {  // publish as primitive (string) message

    // resolve ROS messages to primitive strings if possible, else serialize
    // entire message to string
    std::string payload;
    bool found_primitive = primitiveRosMessageToString(ros_msg, payload);
    if (found_primitive)
      payload_buffer = std::vector<uint8_t>(payload.begin(), payload.end());
    else
      serializeRosMessage(*ros_msg, payload_buffer);

  } else {  // publish as complete ROS message incl. ROS message type

    // serialize ROS message type to buffer
    uint32_t msg_type_length =
      ros::serialization::serializationLength(ros_msg_type);
    std::vector<uint8_t> msg_type_buffer;
    msg_type_buffer.resize(msg_type_length);
    ros::serialization::OStream msg_type_stream(msg_type_buffer.data(),
                                                msg_type_length);
    ros::serialization::serialize(msg_type_stream, ros_msg_type);

    // send ROS message type information to MQTT broker
    mqtt_topic = kRosMsgTypeMqttTopicPrefix + ros2mqtt.mqtt.topic;
    try {
      NODELET_DEBUG("Sending ROS message type to MQTT broker on topic '%s' ...",
                    mqtt_topic.c_str());
      mqtt::message_ptr mqtt_msg =
        mqtt::make_message(mqtt_topic, msg_type_buffer.data(),
                           msg_type_buffer.size(), ros2mqtt.mqtt.qos, true);
      client_->publish(mqtt_msg);
    } catch (const mqtt::exception& e) {
      NODELET_WARN(
        "Publishing ROS message type information to MQTT topic '%s' failed: %s",
        mqtt_topic.c_str(), e.what());
    }

    // build MQTT payload for ROS message (R) as [1, S, R] or [0, R]
    // where first item = 1 if timestamp (S) is included
    uint32_t msg_length = ros::serialization::serializationLength(*ros_msg);
    uint32_t payload_length = 1 + msg_length;
    uint32_t stamp_length =
      ros::serialization::serializationLength(ros::Time());
    uint32_t msg_offset = 1;
    if (ros2mqtt.stamped) {
      // allocate buffer with appropriate size to hold [1, S, R]
      msg_offset += stamp_length;
      payload_length += stamp_length;
      payload_buffer.resize(payload_length);
      payload_buffer[0] = 1;
    } else {
      // allocate buffer with appropriate size to hold [0, R]
      payload_buffer.resize(payload_length);
      payload_buffer[0] = 0;
    }

    // serialize ROS message to payload [0/1, -, R]
    ros::serialization::OStream msg_stream(&payload_buffer[msg_offset],
                                           msg_length);
    ros::serialization::serialize(msg_stream, *ros_msg);

    // inject timestamp as final step
    if (ros2mqtt.stamped) {

      // take current timestamp
      ros::WallTime stamp_wall = ros::WallTime::now();
      ros::Time stamp(stamp_wall.sec, stamp_wall.nsec);
      if (stamp.isZero())
        NODELET_WARN(
          "Injected ROS time 0 into MQTT payload on topic %s, is a ROS clock "
          "running?",
          ros2mqtt.mqtt.topic.c_str());

      // serialize timestamp to payload [1, S, R]
      ros::serialization::OStream stamp_stream(&payload_buffer[1],
                                               stamp_length);
      ros::serialization::serialize(stamp_stream, stamp);
    }
  }

  // send ROS message to MQTT broker
  mqtt_topic = ros2mqtt.mqtt.topic;
  try {
    NODELET_DEBUG(
      "Sending ROS message of type '%s' to MQTT broker on topic '%s' ...",
      ros_msg->getDataType().c_str(), mqtt_topic.c_str());
    mqtt::message_ptr mqtt_msg = mqtt::make_message(
      mqtt_topic, payload_buffer.data(), payload_buffer.size(),
      ros2mqtt.mqtt.qos, ros2mqtt.mqtt.retained);
    client_->publish(mqtt_msg);
  } catch (const mqtt::exception& e) {
    NODELET_WARN(
      "Publishing ROS message type information to MQTT topic '%s' failed: %s",
      mqtt_topic.c_str(), e.what());
  }
}


void MqttClient::mqtt2ros(mqtt::const_message_ptr mqtt_msg,
                          const ros::WallTime& arrival_stamp) {

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

  // if stamped, compute latency
  if (stamped) {

    // create ROS message buffer on top of MQTT message payload
    char* non_const_payload = const_cast<char*>(&payload[1]);
    uint8_t* stamp_buffer = reinterpret_cast<uint8_t*>(non_const_payload);

    // deserialize stamp
    ros::Time stamp;
    uint32_t stamp_length = ros::serialization::serializationLength(stamp);
    ros::serialization::IStream stamp_stream(stamp_buffer, stamp_length);
    ros::serialization::deserialize(stamp_stream, stamp);

    // compute ROS2MQTT latency
    ros::Time now(arrival_stamp.sec, arrival_stamp.nsec);
    if (now.isZero())
      NODELET_WARN(
        "Cannot compute latency for MQTT topic %s when ROS time is 0, is a ROS "
        "clock running?",
        mqtt_topic.c_str());
    ros::Duration latency = now - stamp;
    std_msgs::Float64 latency_msg;
    latency_msg.data = latency.toSec();

    // publish latency
    if (mqtt2ros.ros.latency_publisher.getTopic().empty()) {
      std::string latency_topic = kLatencyRosTopicPrefix + mqtt2ros.ros.topic;
      mqtt2ros.ros.latency_publisher =
        private_node_handle_.advertise<std_msgs::Float64>(latency_topic, 1);
    }
    mqtt2ros.ros.latency_publisher.publish(latency_msg);

    msg_length -= stamp_length;
    msg_offset += stamp_length;
  }

  // create ROS message buffer on top of MQTT message payload
  char* non_const_payload = const_cast<char*>(&payload[msg_offset]);
  uint8_t* msg_buffer = reinterpret_cast<uint8_t*>(non_const_payload);
  ros::serialization::OStream msg_stream(msg_buffer, msg_length);

  // publish via ShapeShifter
  NODELET_DEBUG(
    "Sending ROS message of type '%s' from MQTT broker to ROS topic '%s' ...",
    mqtt2ros.ros.shape_shifter.getDataType().c_str(),
    mqtt2ros.ros.topic.c_str());
  mqtt2ros.ros.shape_shifter.read(msg_stream);
  mqtt2ros.ros.publisher.publish(mqtt2ros.ros.shape_shifter);
}


void MqttClient::mqtt2primitive(mqtt::const_message_ptr mqtt_msg) {

  std::string mqtt_topic = mqtt_msg->get_topic();
  Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_topic];
  const std::string str_msg = mqtt_msg->to_string();

  bool found_primitive = false;
  std::string msg_type_md5;
  std::string msg_type_name;
  std::string msg_type_definition;
  std::vector<uint8_t> msg_buffer;

  // check for bool
  if (!found_primitive) {
    std::string bool_str = str_msg;
    std::transform(str_msg.cbegin(), str_msg.cend(), bool_str.begin(),
                   ::tolower);
    if (bool_str == "true" || bool_str == "false") {

      bool bool_msg = (bool_str == "true");

      // construct and serialize ROS message
      std_msgs::Bool msg;
      msg.data = bool_msg;
      serializeRosMessage(msg, msg_buffer);

      // collect ROS message type information
      msg_type_md5 = ros::message_traits::MD5Sum<std_msgs::Bool>::value();
      msg_type_name = ros::message_traits::DataType<std_msgs::Bool>::value();
      msg_type_definition =
        ros::message_traits::Definition<std_msgs::Bool>::value();

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
        std_msgs::Int32 msg;
        msg.data = int_msg;
        serializeRosMessage(msg, msg_buffer);

        // collect ROS message type information
        msg_type_md5 = ros::message_traits::MD5Sum<std_msgs::Int32>::value();
        msg_type_name = ros::message_traits::DataType<std_msgs::Int32>::value();
        msg_type_definition =
          ros::message_traits::Definition<std_msgs::Int32>::value();

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
        std_msgs::Float32 msg;
        msg.data = float_msg;
        serializeRosMessage(msg, msg_buffer);

        // collect ROS message type information
        msg_type_md5 = ros::message_traits::MD5Sum<std_msgs::Float32>::value();
        msg_type_name =
          ros::message_traits::DataType<std_msgs::Float32>::value();
        msg_type_definition =
          ros::message_traits::Definition<std_msgs::Float32>::value();

        found_primitive = true;
      }
    } catch (const std::invalid_argument& ex) {
    } catch (const std::out_of_range& ex) {
    }
  }

  // fall back to string
  if (!found_primitive) {

    // construct and serialize ROS message
    std_msgs::String msg;
    msg.data = str_msg;
    serializeRosMessage(msg, msg_buffer);

    // collect ROS message type information
    msg_type_md5 = ros::message_traits::MD5Sum<std_msgs::String>::value();
    msg_type_name = ros::message_traits::DataType<std_msgs::String>::value();
    msg_type_definition =
      ros::message_traits::Definition<std_msgs::String>::value();
  }

  // if ROS message type has changed
  if (msg_type_md5 != mqtt2ros.ros.shape_shifter.getMD5Sum()) {

    // configure ShapeShifter
    mqtt2ros.ros.shape_shifter.morph(msg_type_md5, msg_type_name,
                                     msg_type_definition, "");

    // advertise with ROS publisher
    mqtt2ros.ros.publisher.shutdown();
    mqtt2ros.ros.publisher = mqtt2ros.ros.shape_shifter.advertise(
      node_handle_, mqtt2ros.ros.topic, mqtt2ros.ros.queue_size,
      mqtt2ros.ros.latched);

    NODELET_INFO("ROS publisher message type on topic '%s' set to '%s'",
                 mqtt2ros.ros.topic.c_str(), msg_type_name.c_str());
  }

  // publish via ShapeShifter
  ros::serialization::OStream msg_stream(msg_buffer.data(), msg_buffer.size());
  NODELET_DEBUG(
    "Sending ROS message of type '%s' from MQTT broker to ROS topic '%s' ...",
    mqtt2ros.ros.shape_shifter.getDataType().c_str(),
    mqtt2ros.ros.topic.c_str());
  mqtt2ros.ros.shape_shifter.read(msg_stream);
  mqtt2ros.ros.publisher.publish(mqtt2ros.ros.shape_shifter);
}


void MqttClient::connected(const std::string& cause) {

  is_connected_ = true;
  std::string as_client =
    client_config_.id.empty()
      ? ""
      : std::string(" as '") + client_config_.id + std::string("'");
  NODELET_INFO("Connected to broker at '%s'%s",
               client_->get_server_uri().c_str(), as_client.c_str());

  // subscribe MQTT topics
  for (auto& mqtt2ros_p : mqtt2ros_) {
    Mqtt2RosInterface& mqtt2ros = mqtt2ros_p.second;
    std::string mqtt_topic = mqtt2ros_p.first;
    if (!mqtt2ros.primitive)  // subscribe topics for ROS message types first
      mqtt_topic = kRosMsgTypeMqttTopicPrefix + mqtt_topic;
    client_->subscribe(mqtt_topic, mqtt2ros.mqtt.qos);
    NODELET_DEBUG("Subscribed MQTT topic '%s'", mqtt_topic.c_str());
  }
}


void MqttClient::connection_lost(const std::string& cause) {

  NODELET_ERROR("Connection to broker lost, will try to reconnect...");
  is_connected_ = false;
  connect();
}


bool MqttClient::isConnected() {

  return is_connected_;
}


bool MqttClient::isConnectedService(IsConnected::Request& request,
                                    IsConnected::Response& response) {

  response.connected = isConnected();
  return true;
}


void MqttClient::message_arrived(mqtt::const_message_ptr mqtt_msg) {

  // instantly take arrival timestamp
  ros::WallTime arrival_stamp = ros::WallTime::now();

  std::string mqtt_topic = mqtt_msg->get_topic();
  NODELET_DEBUG("Received MQTT message on topic '%s'", mqtt_topic.c_str());

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

    // create ROS message buffer on top of MQTT message payload
    auto& payload = mqtt_msg->get_payload_ref();
    uint32_t payload_length = static_cast<uint32_t>(payload.size());
    char* non_const_payload = const_cast<char*>(&payload[0]);
    uint8_t* msg_type_buffer = reinterpret_cast<uint8_t*>(non_const_payload);

    // deserialize ROS message type
    RosMsgType ros_msg_type;
    ros::serialization::IStream msg_type_stream(msg_type_buffer,
                                                payload_length);
    try {
      ros::serialization::deserialize(msg_type_stream, ros_msg_type);
    } catch (ros::serialization::StreamOverrunException) {
      NODELET_ERROR(
        "Failed to deserialize ROS message type from MQTT message received on "
        "topic '%s', got message:\n%s",
        mqtt_topic.c_str(), mqtt_msg->to_string().c_str());
      return;
    }

    // reconstruct corresponding MQTT data topic
    std::string mqtt_data_topic = mqtt_topic;
    mqtt_data_topic.erase(mqtt_data_topic.find(kRosMsgTypeMqttTopicPrefix),
                          kRosMsgTypeMqttTopicPrefix.length());
    Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_data_topic];

    // if ROS message type has changed
    if (ros_msg_type.md5 != mqtt2ros.ros.shape_shifter.getMD5Sum()) {

      // configure ShapeShifter
      mqtt2ros.ros.shape_shifter.morph(ros_msg_type.md5, ros_msg_type.name,
                                       ros_msg_type.definition, "");

      // advertise with ROS publisher
      mqtt2ros.ros.publisher.shutdown();
      mqtt2ros.ros.publisher = mqtt2ros.ros.shape_shifter.advertise(
        node_handle_, mqtt2ros.ros.topic, mqtt2ros.ros.queue_size,
        mqtt2ros.ros.latched);

      NODELET_INFO("ROS publisher message type on topic '%s' set to '%s'",
                   mqtt2ros.ros.topic.c_str(), ros_msg_type.name.c_str());

      // subscribe to MQTT topic with actual ROS messages
      client_->subscribe(mqtt_data_topic, mqtt2ros.mqtt.qos);
      NODELET_DEBUG("Subscribed MQTT topic '%s'", mqtt_data_topic.c_str());
    }

  } else {

    // publish ROS message, if publisher initialized
    if (!mqtt2ros_[mqtt_topic].ros.publisher.getTopic().empty()) {
      mqtt2ros(mqtt_msg, arrival_stamp);
    } else {
      NODELET_WARN(
        "ROS publisher for data from MQTT topic '%s' is not yet initialized: "
        "ROS message type not yet known",
        mqtt_topic.c_str());
    }
  }
}


void MqttClient::delivery_complete(mqtt::delivery_token_ptr token) {}


void MqttClient::on_success(const mqtt::token& token) {}


void MqttClient::on_failure(const mqtt::token& token) {

  ROS_ERROR(
    "Connection to broker failed (return code %d), will automatically "
    "retry...",
    token.get_return_code());
}

}  // namespace mqtt_client
