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

#include <mqtt_client/MqttClient_ros2.hpp>
#include <mqtt_client_interfaces/msg/ros_msg_type.hpp>
#include <rcpputils/get_env.hpp>


namespace mqtt_client {


const std::string MqttClient::kRosMsgTypeMqttTopicPrefix =
  "mqtt_client/ros_msg_type/";

const std::string MqttClient::kLatencyRosTopicPrefix = "latencies/";


/**
 * @brief Extracts string of primitive data types from ROS message.
 *
 * TODO
 *
 * @param [in]  msg        generic ShapeShifter ROS message
 * @param [out] primitive  string representation of primitive message data
 *
 * @return true  if primitive ROS message type was found
 * @return false if ROS message type is not primitive
 */
// bool primitiveRosMessageToString(const topic_tools::ShapeShifter::ConstPtr& msg,
//                                  std::string& primitive) {

//   bool found_primitive = true;
//   const std::string& msg_type_md5 = msg->getMD5Sum();

//   if (msg_type_md5 == ros::message_traits::MD5Sum<std_msgs::String>::value()) {
//     primitive = msg->instantiate<std_msgs::String>()->data;
//   } else if (msg_type_md5 ==
//              ros::message_traits::MD5Sum<std_msgs::Bool>::value()) {
//     primitive = msg->instantiate<std_msgs::Bool>()->data ? "true" : "false";
//   } else if (msg_type_md5 ==
//              ros::message_traits::MD5Sum<std_msgs::Char>::value()) {
//     primitive = std::to_string(msg->instantiate<std_msgs::Char>()->data);
//   } else if (msg_type_md5 ==
//              ros::message_traits::MD5Sum<std_msgs::UInt8>::value()) {
//     primitive = std::to_string(msg->instantiate<std_msgs::UInt8>()->data);
//   } else if (msg_type_md5 ==
//              ros::message_traits::MD5Sum<std_msgs::UInt16>::value()) {
//     primitive = std::to_string(msg->instantiate<std_msgs::UInt16>()->data);
//   } else if (msg_type_md5 ==
//              ros::message_traits::MD5Sum<std_msgs::UInt32>::value()) {
//     primitive = std::to_string(msg->instantiate<std_msgs::UInt32>()->data);
//   } else if (msg_type_md5 ==
//              ros::message_traits::MD5Sum<std_msgs::UInt64>::value()) {
//     primitive = std::to_string(msg->instantiate<std_msgs::UInt64>()->data);
//   } else if (msg_type_md5 ==
//              ros::message_traits::MD5Sum<std_msgs::Int8>::value()) {
//     primitive = std::to_string(msg->instantiate<std_msgs::Int8>()->data);
//   } else if (msg_type_md5 ==
//              ros::message_traits::MD5Sum<std_msgs::Int16>::value()) {
//     primitive = std::to_string(msg->instantiate<std_msgs::Int16>()->data);
//   } else if (msg_type_md5 ==
//              ros::message_traits::MD5Sum<std_msgs::Int32>::value()) {
//     primitive = std::to_string(msg->instantiate<std_msgs::Int32>()->data);
//   } else if (msg_type_md5 ==
//              ros::message_traits::MD5Sum<std_msgs::Int64>::value()) {
//     primitive = std::to_string(msg->instantiate<std_msgs::Int64>()->data);
//   } else if (msg_type_md5 ==
//              ros::message_traits::MD5Sum<std_msgs::Float32>::value()) {
//     primitive = std::to_string(msg->instantiate<std_msgs::Float32>()->data);
//   } else if (msg_type_md5 ==
//              ros::message_traits::MD5Sum<std_msgs::Float64>::value()) {
//     primitive = std::to_string(msg->instantiate<std_msgs::Float64>()->data);
//   } else {
//     found_primitive = false;
//   }

//   return found_primitive;
// }


MqttClient::MqttClient() : Node("mqtt_client") {

  loadParameters();
  setup();
}


void MqttClient::loadParameters() {

  declare_parameter("broker/host");
  declare_parameter("broker/port");
  declare_parameter("bridge/ros2mqtt/ros_topic");
  declare_parameter("bridge/ros2mqtt/mqtt_topic");
  declare_parameter("bridge/mqtt2ros/mqtt_topic");
  declare_parameter("bridge/mqtt2ros/ros_topic");

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
    RCLCPP_WARN(get_logger(), "Client buffer can not be enabled when client ID is empty");
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

  // parse bridge parameters

  // ros2mqtt
  rclcpp::Parameter ros_topic_param, mqtt_topic_param;
  if (get_parameter("bridge/ros2mqtt/ros_topic", ros_topic_param) &&
      get_parameter("bridge/ros2mqtt/mqtt_topic", mqtt_topic_param)) {

    // ros2mqtt[k]/ros_topic and ros2mqtt[k]/mqtt_topic
    std::string ros_topic = ros_topic_param.as_string();
    std::string mqtt_topic = mqtt_topic_param.as_string();
    Ros2MqttInterface& ros2mqtt = ros2mqtt_[ros_topic];
    ros2mqtt.mqtt.topic = mqtt_topic;

    // ros2mqtt[k]/primitive
    rclcpp::Parameter primitive_param;
    if (get_parameter("bridge/ros2mqtt/primitive", primitive_param))
      ros2mqtt.primitive = primitive_param.as_bool();

    // ros2mqtt[k]/inject_timestamp
    rclcpp::Parameter stamped_param;
    if (get_parameter("bridge/ros2mqtt/inject_timestamp", stamped_param))
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
    if (get_parameter("bridge/ros2mqtt/advanced/ros/queue_size", queue_size_param))
      ros2mqtt.ros.queue_size = queue_size_param.as_int();

    // ros2mqtt[k]/advanced/mqtt/qos
    rclcpp::Parameter qos_param;
    if (get_parameter("bridge/ros2mqtt/advanced/mqtt/qos", qos_param))
      ros2mqtt.mqtt.qos = qos_param.as_int();

    // ros2mqtt[k]/advanced/mqtt/retained
    rclcpp::Parameter retained_param;
    if (get_parameter("bridge/ros2mqtt/advanced/mqtt/retained", retained_param))
      ros2mqtt.mqtt.retained = retained_param.as_bool();

    RCLCPP_INFO(get_logger(),
                "Bridging %sROS topic '%s' to MQTT topic '%s' %s",
                ros2mqtt.primitive ? "primitive " : "",
                ros_topic.c_str(), ros2mqtt.mqtt.topic.c_str(),
                ros2mqtt.stamped ? "and measuring latency" : "");
  } else {
    RCLCPP_WARN(
      get_logger(),
      "Parameter 'bridge/ros2mqtt' is missing subparameter "
      "'ros_topic' or 'mqtt_topic', will be ignored");
  }

  // mqtt2ros
  if (get_parameter("bridge/mqtt2ros/mqtt_topic", mqtt_topic_param) &&
      get_parameter("bridge/mqtt2ros/ros_topic", ros_topic_param)) {

    // mqtt2ros[k]/mqtt_topic and mqtt2ros[k]/ros_topic
    std::string mqtt_topic = mqtt_topic_param.as_string();
    std::string ros_topic = ros_topic_param.as_string();
    Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_topic];
    mqtt2ros.ros.topic = ros_topic;

    // mqtt2ros[k]/advanced/mqtt/qos
    rclcpp::Parameter qos_param;
    if (get_parameter("bridge/mqtt2ros/advanced/mqtt/qos", qos_param))
      mqtt2ros.mqtt.qos = qos_param.as_int();

    // mqtt2ros[k]/advanced/ros/queue_size
    rclcpp::Parameter queue_size_param;
    if (get_parameter("bridge/mqtt2ros/advanced/ros/queue_size", queue_size_param))
      mqtt2ros.ros.queue_size = queue_size_param.as_int();

    // mqtt2ros[k]/advanced/ros/latched
    rclcpp::Parameter latched_param;
    if (get_parameter("bridge/mqtt2ros/advanced/ros/latched", latched_param))
      mqtt2ros.ros.latched = latched_param.as_bool();

    RCLCPP_INFO(get_logger(),
                "Bridging MQTT topic '%s' to ROS topic '%s'",
                mqtt_topic.c_str(), mqtt2ros.ros.topic.c_str());
  } else {
    RCLCPP_WARN(
      get_logger(),
      "Parameter 'bridge/mqtt2ros' is missing subparameter "
      "'mqtt_topic' or 'ros_topic', will be ignored");
  }

  if (ros2mqtt_.empty() && mqtt2ros_.empty()) {
    RCLCPP_ERROR(
      get_logger(),
      "No valid ROS-MQTT bridge found in parameter 'bridge'");
    exit(EXIT_FAILURE);
  }
}


bool MqttClient::loadParameter(const std::string& key, std::string& value) {
  bool found = get_parameter(key, value);
  if (found)
    RCLCPP_DEBUG(get_logger(),
                 "Retrieved parameter '%s' = '%s'", key.c_str(), value.c_str());
  return found;
}


bool MqttClient::loadParameter(const std::string& key, std::string& value,
                               const std::string& default_value) {
  bool found = get_parameter_or(key, value, default_value);
  if (!found)
    RCLCPP_WARN(get_logger(),
                "Parameter '%s' not set, defaulting to '%s'", key.c_str(),
                default_value.c_str());
  if (found)
    RCLCPP_DEBUG(get_logger(),
                 "Retrieved parameter '%s' = '%s'", key.c_str(), value.c_str());
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
    RCLCPP_WARN(get_logger(),
                "Requested path '%s' does not exist",
                std::string(path).c_str());
  return path;
}


void MqttClient::setup() {

  // initialize MQTT client
  setupClient();

  // connect to MQTT broker
  connect();

  // TODO
  // create ROS service server
  // is_connected_service_ = private_node_handle_.advertiseService(
  //   "is_connected", &MqttClient::isConnectedService, this);

  // setup subscribers; check for new types every second
  check_subscriptions_timer_ =
    create_wall_timer(std::chrono::duration<double>(1.0),
                      std::bind(&MqttClient::setupSubscriptions, this));
}


void MqttClient::setupSubscriptions() {

  // get info of all topics
  const auto all_topics_and_types = get_topic_names_and_types();

  // check for ros2mqtt topics
  for (auto& ros2mqtt_p : ros2mqtt_) {
    const std::string& ros_topic = ros2mqtt_p.first;
    Ros2MqttInterface& ros2mqtt = ros2mqtt_p.second;
    if (all_topics_and_types.count(ros_topic)) {

      // check if message type has changed
      const std::string& msg_type = all_topics_and_types.at(ros_topic)[0];
      if (msg_type == ros2mqtt.ros.msg_type) continue;
      ros2mqtt.ros.msg_type = msg_type;
    
      // create new generic subscription, if message type has changed
      std::function<void(const std::shared_ptr<rclcpp::SerializedMessage> msg)> bound_callback_func =
        std::bind(&MqttClient::ros2mqtt, this, std::placeholders::_1, ros_topic);
      ros2mqtt.ros.subscriber = create_generic_subscription(
        ros_topic, msg_type, ros2mqtt.ros.queue_size, bound_callback_func);
      RCLCPP_INFO(get_logger(),
                  "Subscribed ROS topic '%s' of type '%s'",
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
    RCLCPP_ERROR(get_logger(),
                 "Client could not be initialized: %s", e.what());
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
  RCLCPP_INFO(get_logger(),
              "Connecting to broker at '%s'%s ...",
              client_->get_server_uri().c_str(), as_client.c_str());

  try {
    client_->connect(connect_options_, nullptr, *this);
  } catch (const mqtt::exception& e) {
    RCLCPP_ERROR(get_logger(),
                 "Connection to broker failed: %s", e.what());
    exit(EXIT_FAILURE);
  }
}


void MqttClient::ros2mqtt(const std::shared_ptr<rclcpp::SerializedMessage>& serialized_msg,
                          const std::string& ros_topic) {

  Ros2MqttInterface& ros2mqtt = ros2mqtt_[ros_topic];
  std::string mqtt_topic = ros2mqtt.mqtt.topic;
  std::vector<uint8_t> payload_buffer;

  // gather information on ROS message type in special ROS message
  mqtt_client_interfaces::msg::RosMsgType ros_msg_type;
  ros_msg_type.name = ros2mqtt.ros.msg_type;

  RCLCPP_DEBUG(get_logger(),
               "Received ROS message of type '%s' on topic '%s'",
               ros_msg_type.name.c_str(), ros_topic.c_str());

  if (ros2mqtt.primitive) {  // publish as primitive (string) message

    // TODO

  } else {  // publish as complete ROS message incl. ROS message type

    // TODO
    // serialize ROS message type to buffer
    // uint32_t msg_type_length =
    //   ros::serialization::serializationLength(ros_msg_type);
    // std::vector<uint8_t> msg_type_buffer;
    // msg_type_buffer.resize(msg_type_length);
    // ros::serialization::OStream msg_type_stream(msg_type_buffer.data(),
    //                                             msg_type_length);
    // ros::serialization::serialize(msg_type_stream, ros_msg_type);

    // TODO
    // send ROS message type information to MQTT broker
    // mqtt_topic = kRosMsgTypeMqttTopicPrefix + ros2mqtt.mqtt.topic;
    // try {
    //   NODELET_DEBUG("Sending ROS message type to MQTT broker on topic '%s' ...",
    //                 mqtt_topic.c_str());
    //   mqtt::message_ptr mqtt_msg =
    //     mqtt::make_message(mqtt_topic, msg_type_buffer.data(),
    //                        msg_type_buffer.size(), ros2mqtt.mqtt.qos, true);
    //   client_->publish(mqtt_msg);
    // } catch (const mqtt::exception& e) {
    //   NODELET_WARN(
    //     "Publishing ROS message type information to MQTT topic '%s' failed: %s",
    //     mqtt_topic.c_str(), e.what());
    // }

    // TODO
    // build MQTT payload for ROS message (R) as [1, S, R] or [0, R]
    // where first item = 1 if timestamp (S) is included
    uint32_t msg_length = serialized_msg->size();
    // uint32_t payload_length = 1 + msg_length;
    // uint32_t stamp_length = ros::serialization::serializationLength(ros::Time());
    // uint32_t msg_offset = 1;
    // if (ros2mqtt.stamped) {
    //   // allocate buffer with appropriate size to hold [1, S, R]
    //   msg_offset += stamp_length;
    //   payload_length += stamp_length;
    //   payload_buffer.resize(payload_length);
    //   payload_buffer[0] = 1;
    // } else {
    //   // allocate buffer with appropriate size to hold [0, R]
    //   payload_buffer.resize(payload_length);
    //   payload_buffer[0] = 0;
    // }

    // build payload construct around serialized ROS Message
    payload_buffer = std::vector<uint8_t>(
      serialized_msg->get_rcl_serialized_message().buffer,
      serialized_msg->get_rcl_serialized_message().buffer + msg_length);

    // TODO
    // inject timestamp as final step
    // if (ros2mqtt.stamped) {

    //   // take current timestamp
    //   ros::WallTime stamp_wall = ros::WallTime::now();
    //   ros::Time stamp(stamp_wall.sec, stamp_wall.nsec);
    //   if (stamp.isZero())
    //     NODELET_WARN(
    //       "Injected ROS time 0 into MQTT payload on topic %s, is a ROS clock "
    //       "running?",
    //       ros2mqtt.mqtt.topic.c_str());

    //   // serialize timestamp to payload [1, S, R]
    //   ros::serialization::OStream stamp_stream(&payload_buffer[1],
    //                                            stamp_length);
    //   ros::serialization::serialize(stamp_stream, stamp);
    // }
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


void MqttClient::mqtt2ros(mqtt::const_message_ptr mqtt_msg) {
                          // TODO: const ros::WallTime& arrival_stamp) {

  std::string mqtt_topic = mqtt_msg->get_topic();
  Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_topic];
  auto& payload = mqtt_msg->get_payload_ref();
  uint32_t payload_length = static_cast<uint32_t>(payload.size());

  // TODO
  // determine whether timestamp is injected by reading first element
  bool stamped = false; // (static_cast<uint8_t>(payload[0]) > 0);

  // TODO
  // read MQTT payload for ROS message (R) as [1, S, R] or [0, R]
  // where first item = 1 if timestamp (S) is included
  uint32_t msg_length = payload_length; // - 1;
  uint32_t msg_offset = 0; // TODO: 1;

  // TODO
  // if stamped, compute latency
  if (stamped) {

    // // create ROS message buffer on top of MQTT message payload
    // char* non_const_payload = const_cast<char*>(&payload[1]);
    // uint8_t* stamp_buffer = reinterpret_cast<uint8_t*>(non_const_payload);

    // // deserialize stamp
    // ros::Time stamp;
    // uint32_t stamp_length = ros::serialization::serializationLength(stamp);
    // ros::serialization::IStream stamp_stream(stamp_buffer, stamp_length);
    // ros::serialization::deserialize(stamp_stream, stamp);

    // // compute ROS2MQTT latency
    // ros::Time now(arrival_stamp.sec, arrival_stamp.nsec);
    // if (now.isZero())
    //   NODELET_WARN(
    //     "Cannot compute latency for MQTT topic %s when ROS time is 0, is a ROS "
    //     "clock running?",
    //     mqtt_topic.c_str());
    // ros::Duration latency = now - stamp;
    // std_msgs::Float64 latency_msg;
    // latency_msg.data = latency.toSec();

    // // publish latency
    // if (mqtt2ros.ros.latency_publisher.getTopic().empty()) {
    //   std::string latency_topic = kLatencyRosTopicPrefix + mqtt2ros.ros.topic;
    //   mqtt2ros.ros.latency_publisher =
    //     private_node_handle_.advertise<std_msgs::Float64>(latency_topic, 1);
    // }
    // mqtt2ros.ros.latency_publisher.publish(latency_msg);

    // msg_length -= stamp_length;
    // msg_offset += stamp_length;
  }

  // copy ROS message from MQTT message to generic message buffer
  rclcpp::SerializedMessage serialized_msg(msg_length);
  std::memcpy(serialized_msg.get_rcl_serialized_message().buffer, &(payload[msg_offset]), msg_length);
  serialized_msg.get_rcl_serialized_message().buffer_length = msg_length;

  // TODO: receive type from RosMsgType
  // TODO: don't create new publisher for every message
  // publish generic ROS message
  RCLCPP_DEBUG(
    get_logger(),
    "Sending ROS message of type '%s' from MQTT broker to ROS topic '%s' ...",
    "std_msgs/msg/String",
    mqtt2ros.ros.topic.c_str());
  mqtt2ros.ros.publisher =
    create_generic_publisher(mqtt2ros.ros.topic, "std_msgs/msg/String", mqtt2ros.ros.queue_size);
  mqtt2ros.ros.publisher->publish(serialized_msg);
}


void MqttClient::mqtt2primitive(mqtt::const_message_ptr mqtt_msg) {

  // TODO

  // std::string mqtt_topic = mqtt_msg->get_topic();
  // Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_topic];
  // const std::string str_msg = mqtt_msg->to_string();

  // bool found_primitive = false;
  // std::string msg_type_md5;
  // std::string msg_type_name;
  // std::string msg_type_definition;
  // std::vector<uint8_t> msg_buffer;

  // // check for bool
  // if (!found_primitive) {
  //   std::string bool_str = str_msg;
  //   std::transform(str_msg.cbegin(), str_msg.cend(), bool_str.begin(),
  //                  ::tolower);
  //   if (bool_str == "true" || bool_str == "false") {

  //     bool bool_msg = (bool_str == "true");

  //     // construct and serialize ROS message
  //     std_msgs::Bool msg;
  //     msg.data = bool_msg;
  //     serializeRosMessage(msg, msg_buffer);

  //     // collect ROS message type information
  //     msg_type_md5 = ros::message_traits::MD5Sum<std_msgs::Bool>::value();
  //     msg_type_name = ros::message_traits::DataType<std_msgs::Bool>::value();
  //     msg_type_definition =
  //       ros::message_traits::Definition<std_msgs::Bool>::value();

  //     found_primitive = true;
  //   }
  // }

  // // check for int
  // if (!found_primitive) {
  //   std::size_t pos;
  //   try {
  //     const int int_msg = std::stoi(str_msg, &pos);
  //     if (pos == str_msg.size()) {

  //       // construct and serialize ROS message
  //       std_msgs::Int32 msg;
  //       msg.data = int_msg;
  //       serializeRosMessage(msg, msg_buffer);

  //       // collect ROS message type information
  //       msg_type_md5 = ros::message_traits::MD5Sum<std_msgs::Int32>::value();
  //       msg_type_name = ros::message_traits::DataType<std_msgs::Int32>::value();
  //       msg_type_definition =
  //         ros::message_traits::Definition<std_msgs::Int32>::value();

  //       found_primitive = true;
  //     }
  //   } catch (const std::invalid_argument& ex) {
  //   } catch (const std::out_of_range& ex) {
  //   }
  // }

  // // check for float
  // if (!found_primitive) {
  //   std::size_t pos;
  //   try {
  //     const float float_msg = std::stof(str_msg, &pos);
  //     if (pos == str_msg.size()) {

  //       // construct and serialize ROS message
  //       std_msgs::Float32 msg;
  //       msg.data = float_msg;
  //       serializeRosMessage(msg, msg_buffer);

  //       // collect ROS message type information
  //       msg_type_md5 = ros::message_traits::MD5Sum<std_msgs::Float32>::value();
  //       msg_type_name =
  //         ros::message_traits::DataType<std_msgs::Float32>::value();
  //       msg_type_definition =
  //         ros::message_traits::Definition<std_msgs::Float32>::value();

  //       found_primitive = true;
  //     }
  //   } catch (const std::invalid_argument& ex) {
  //   } catch (const std::out_of_range& ex) {
  //   }
  // }

  // // fall back to string
  // if (!found_primitive) {

  //   // construct and serialize ROS message
  //   std_msgs::String msg;
  //   msg.data = str_msg;
  //   serializeRosMessage(msg, msg_buffer);

  //   // collect ROS message type information
  //   msg_type_md5 = ros::message_traits::MD5Sum<std_msgs::String>::value();
  //   msg_type_name = ros::message_traits::DataType<std_msgs::String>::value();
  //   msg_type_definition =
  //     ros::message_traits::Definition<std_msgs::String>::value();
  // }

  // // if ROS message type has changed
  // if (msg_type_md5 != mqtt2ros.ros.shape_shifter.getMD5Sum()) {

  //   // configure ShapeShifter
  //   mqtt2ros.ros.shape_shifter.morph(msg_type_md5, msg_type_name,
  //                                    msg_type_definition, "");

  //   // advertise with ROS publisher
  //   mqtt2ros.ros.publisher.shutdown();
  //   mqtt2ros.ros.publisher = mqtt2ros.ros.shape_shifter.advertise(
  //     node_handle_, mqtt2ros.ros.topic, mqtt2ros.ros.queue_size,
  //     mqtt2ros.ros.latched);

  //   NODELET_INFO("ROS publisher message type on topic '%s' set to '%s'",
  //                mqtt2ros.ros.topic.c_str(), msg_type_name.c_str());
  // }

  // // publish via ShapeShifter
  // ros::serialization::OStream msg_stream(msg_buffer.data(), msg_buffer.size());
  // NODELET_DEBUG(
  //   "Sending ROS message of type '%s' from MQTT broker to ROS topic '%s' ...",
  //   mqtt2ros.ros.shape_shifter.getDataType().c_str(),
  //   mqtt2ros.ros.topic.c_str());
  // mqtt2ros.ros.shape_shifter.read(msg_stream);
  // mqtt2ros.ros.publisher.publish(mqtt2ros.ros.shape_shifter);
}


void MqttClient::connected(const std::string& cause) {

  is_connected_ = true;
  std::string as_client =
    client_config_.id.empty()
      ? ""
      : std::string(" as '") + client_config_.id + std::string("'");
  RCLCPP_INFO(get_logger(),
              "Connected to broker at '%s'%s",
              client_->get_server_uri().c_str(), as_client.c_str());

  // subscribe MQTT topics
  for (auto& mqtt2ros_p : mqtt2ros_) {
    Mqtt2RosInterface& mqtt2ros = mqtt2ros_p.second;
    std::string mqtt_topic = mqtt2ros_p.first;
    if (!mqtt2ros.primitive)  // subscribe topics for ROS message types first
      mqtt_topic = kRosMsgTypeMqttTopicPrefix + mqtt_topic;
    client_->subscribe(mqtt_topic, mqtt2ros.mqtt.qos);
    RCLCPP_INFO(get_logger(), "Subscribed MQTT topic '%s'", mqtt_topic.c_str());
  }
}


void MqttClient::connection_lost(const std::string& cause) {

  RCLCPP_ERROR(get_logger(), "Connection to broker lost, will try to reconnect...");
  is_connected_ = false;
  connect();
}


bool MqttClient::isConnected() {

  return is_connected_;
}


bool MqttClient::isConnectedService(mqtt_client_interfaces::srv::IsConnected::Request& request,
                                    mqtt_client_interfaces::srv::IsConnected::Response& response) {

  response.connected = isConnected();
  return true;
}


void MqttClient::message_arrived(mqtt::const_message_ptr mqtt_msg) {

  // TODO
  // instantly take arrival timestamp
  // ros::WallTime arrival_stamp = ros::WallTime::now();

  std::string mqtt_topic = mqtt_msg->get_topic();
  RCLCPP_DEBUG(get_logger(), "Received MQTT message on topic '%s'", mqtt_topic.c_str());

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

    // TODO

    // // create ROS message buffer on top of MQTT message payload
    // auto& payload = mqtt_msg->get_payload_ref();
    // uint32_t payload_length = static_cast<uint32_t>(payload.size());
    // char* non_const_payload = const_cast<char*>(&payload[0]);
    // uint8_t* msg_type_buffer = reinterpret_cast<uint8_t*>(non_const_payload);

    // // deserialize ROS message type
    // RosMsgType ros_msg_type;
    // ros::serialization::IStream msg_type_stream(msg_type_buffer,
    //                                             payload_length);
    // try {
    //   ros::serialization::deserialize(msg_type_stream, ros_msg_type);
    // } catch (ros::serialization::StreamOverrunException) {
    //   NODELET_ERROR(
    //     "Failed to deserialize ROS message type from MQTT message received on "
    //     "topic '%s', got message:\n%s",
    //     mqtt_topic.c_str(), mqtt_msg->to_string().c_str());
    //   return;
    // }

    // // reconstruct corresponding MQTT data topic
    // std::string mqtt_data_topic = mqtt_topic;
    // mqtt_data_topic.erase(mqtt_data_topic.find(kRosMsgTypeMqttTopicPrefix),
    //                       kRosMsgTypeMqttTopicPrefix.length());
    // Mqtt2RosInterface& mqtt2ros = mqtt2ros_[mqtt_data_topic];

    // // if ROS message type has changed
    // if (ros_msg_type.md5 != mqtt2ros.ros.shape_shifter.getMD5Sum()) {

    //   // configure ShapeShifter
    //   mqtt2ros.ros.shape_shifter.morph(ros_msg_type.md5, ros_msg_type.name,
    //                                    ros_msg_type.definition, "");

    //   // advertise with ROS publisher
    //   mqtt2ros.ros.publisher.shutdown();
    //   mqtt2ros.ros.publisher = mqtt2ros.ros.shape_shifter.advertise(
    //     node_handle_, mqtt2ros.ros.topic, mqtt2ros.ros.queue_size,
    //     mqtt2ros.ros.latched);

    //   NODELET_INFO("ROS publisher message type on topic '%s' set to '%s'",
    //                mqtt2ros.ros.topic.c_str(), ros_msg_type.name.c_str());

    //   // subscribe to MQTT topic with actual ROS messages
    //   client_->subscribe(mqtt_data_topic, mqtt2ros.mqtt.qos);
    //   NODELET_DEBUG("Subscribed MQTT topic '%s'", mqtt_data_topic.c_str());
    // }

  } else {

    // publish ROS message, if publisher initialized
    if (true) { // TODO: (!mqtt2ros_[mqtt_topic].ros.publisher.getTopic().empty()) {
      mqtt2ros(mqtt_msg); // TODO: , arrival_stamp);
    } else {
      RCLCPP_WARN(
        get_logger(),
        "ROS publisher for data from MQTT topic '%s' is not yet initialized: "
        "ROS message type not yet known",
        mqtt_topic.c_str());
    }
  }
}


void MqttClient::delivery_complete(mqtt::delivery_token_ptr token) {}


void MqttClient::on_success(const mqtt::token& token) {}


void MqttClient::on_failure(const mqtt::token& token) {

  RCLCPP_ERROR(
    get_logger(),
    "Connection to broker failed (return code %d), will automatically "
    "retry...",
    token.get_return_code());
}

}  // namespace mqtt_client


int main(int argc, char *argv[]) {

  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<mqtt_client::MqttClient>());
  rclcpp::shutdown();

  return 0;
}