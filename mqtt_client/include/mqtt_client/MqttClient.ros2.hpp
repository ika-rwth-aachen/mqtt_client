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

#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <mqtt_client_interfaces/srv/is_connected.hpp>
#include <mqtt_client_interfaces/srv/new_mqtt2_ros_bridge.hpp>
#include <mqtt_client_interfaces/srv/new_ros2_mqtt_bridge.hpp>
#include <mqtt/async_client.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp/serialization.hpp>
#include <std_msgs/msg/float64.hpp>


/**
 * @brief Namespace for the mqtt_client package
 */
namespace mqtt_client {


/**
 * @brief ROS Nodelet for sending and receiving ROS messages via MQTT
 *
 * The MqttClient enables connected ROS-based devices or robots to
 * exchange ROS messages via an MQTT broker using the MQTT protocol.
 * This works generically for any ROS message, i.e. there is no need
 * to specify the ROS message type for ROS messages you wish to
 * exchange via the MQTT broker.
 */
class MqttClient : public rclcpp::Node,
                   public virtual mqtt::callback,
                   public virtual mqtt::iaction_listener {

 public:
  /**
   * @brief Initializes node.
   *
   * @param[in]   options   ROS node options
   */
  explicit MqttClient(const rclcpp::NodeOptions& options);

 protected:
  /**
   * @brief Loads ROS parameters from parameter server.
   */
  void loadParameters();

  /**
   * @brief Loads requested ROS parameter from parameter server.
   *
   * @param[in]   key      parameter name
   * @param[out]  value    variable where to store the retrieved parameter
   *
   * @return  true         if parameter was successfully retrieved
   * @return  false        if parameter was not found
   */
  bool loadParameter(const std::string& key, std::string& value);

  /**
   * @brief Loads requested ROS parameter from parameter server, allows default
   * value.
   *
   * @param[in]   key            parameter name
   * @param[out]  value          variable where to store the retrieved parameter
   * @param[in]   default_value  default value
   *
   * @return  true         if parameter was successfully retrieved
   * @return  false        if parameter was not found or default was used
   */
  bool loadParameter(const std::string& key, std::string& value,
                     const std::string& default_value);

  /**
   * @brief Loads requested ROS parameter from parameter server.
   *
   * @tparam  T            type (one of int, double, bool)
   *
   * @param[in]   key      parameter name
   * @param[out]  value    variable where to store the retrieved parameter
   *
   * @return  true         if parameter was successfully retrieved
   * @return  false        if parameter was not found
   */
  template <typename T>
  bool loadParameter(const std::string& key, T& value);

  /**
   * @brief Loads requested ROS parameter from parameter server, allows default
   * value.
   *
   * @tparam  T            type (one of int, double, bool)
   *
   * @param[in]   key            parameter name
   * @param[out]  value          variable where to store the retrieved parameter
   * @param[in]   default_value  default value
   *
   * @return  true         if parameter was successfully retrieved
   * @return  false        if parameter was not found or default was used
   */
  template <typename T>
  bool loadParameter(const std::string& key, T& value, const T& default_value);

  /**
   * @brief Loads requested ROS parameter from parameter server.
   *
   * @tparam  T            type (one of int, double, bool)
   *
   * @param[in]   key      parameter name
   * @param[out]  value    variable where to store the retrieved parameter
   *
   * @return  true         if parameter was successfully retrieved
   * @return  false        if parameter was not found
   */
  template <typename T>
  bool loadParameter(const std::string& key, std::vector<T>& value);

  /**
   * @brief Loads requested ROS parameter from parameter server, allows default
   * value.
   *
   * @tparam  T            type (one of int, double, bool)
   *
   * @param[in]   key            parameter name
   * @param[out]  value          variable where to store the retrieved parameter
   * @param[in]   default_value  default value
   *
   * @return  true         if parameter was successfully retrieved
   * @return  false        if parameter was not found or default was used
   */
  template <typename T>
  bool loadParameter(const std::string& key, std::vector<T>& value, const std::vector<T>& default_value);

  /**
   * @brief Converts a string to a path object resolving paths relative to
   * ROS_HOME.
   *
   * Resolves relative to CWD, if ROS_HOME is not set.
   * Returns empty path, if argument is empty.
   *
   * @param   path_string  (relative) path as string
   *
   * @return  std::filesystem::path  path variable
   */
  std::filesystem::path resolvePath(const std::string& path_string);

  /**
   * @brief Initializes broker connection and subscriptions.
   */
  void setup();

  /**
   * @brief Checks all active ROS topics in order to set up generic subscribers.
   */
  void setupSubscriptions();

  /**
   * @brief Sets up the client connection options and initializes the client
   * object.
   */
  void setupClient();

  /**
   * @brief Connects to the broker using the member client and options.
   */
  void connect();

  /**
   * @brief Publishes a generic serialized ROS message to the MQTT broker.
   *
   * Before publishing the ROS message to the MQTT broker, the ROS message type
   * is extracted. This type information is also sent to the MQTT broker on a
   * separate topic.
   *
   * The MQTT payload for the actual ROS message carries the following:
   * - 0 or 1 (indicating if timestamp is injected (=1))
   * - serialized timestamp (optional)
   * - serialized ROS message
   *
   * @param   serialized_msg  generic serialized ROS message
   * @param   ros_topic       ROS topic where the message was published
   */
  void ros2mqtt(
    const std::shared_ptr<rclcpp::SerializedMessage>& serialized_msg,
    const std::string& ros_topic);

  /**
   * @brief Publishes a ROS message received via MQTT to ROS.
   *
   * This utilizes the generic publisher stored for the MQTT topic on which the
   * message was received. The publisher has to be configured to the ROS message
   * type of the message. If the message carries an injected timestamp, the
   * latency is computed and published.
   *
   * The MQTT payload is expected to carry the following:
   * - 0 or 1 (indicating if timestamp is injected (=1))
   * - serialized timestamp (optional)
   * - serialized ROS message
   *
   * @param   mqtt_msg       MQTT message
   * @param   arrival_stamp  arrival timestamp used for latency computation
   */
  void mqtt2ros(mqtt::const_message_ptr mqtt_msg,
                const rclcpp::Time& arrival_stamp);

  /**
   * @brief Publishes a primitive message received via MQTT to ROS.
   *
   * Currently not implemented.
   *
   * @param   mqtt_msg     MQTT message
   */
  void mqtt2primitive(mqtt::const_message_ptr mqtt_msg);

  /**
   * @brief Callback for when the client has successfully connected to the
   * broker.
   *
   * Overrides mqtt::callback::connected(const std::string&).
   *
   * @param   cause
   */
  void connected(const std::string& cause) override;

  /**
   * @brief Callback for when the client has lost connection to the broker.
   *
   * Overrides mqtt::callback::connection_lost(const std::string&).
   *
   * @param   cause
   */
  void connection_lost(const std::string& cause) override;

  /**
   * @brief Returns whether the client is connected to the broker.
   *
   * @return true if client is connected to the broker
   * @return false if client is not connected to the broker
   */
  bool isConnected();

  /**
   * @brief ROS service returning whether the client is connected to the broker.
   *
   * @param request  service request
   * @param response service response
   */
  void isConnectedService(
    mqtt_client_interfaces::srv::IsConnected::Request::SharedPtr request,
    mqtt_client_interfaces::srv::IsConnected::Response::SharedPtr response);

  /**
   * @brief ROS service that dynamically creates a ROS -> MQTT mapping.
   *
   * @param request  service request
   * @param response service response
   */
  void newRos2MqttBridge(
    mqtt_client_interfaces::srv::NewRos2MqttBridge::Request::SharedPtr request,
    mqtt_client_interfaces::srv::NewRos2MqttBridge::Response::SharedPtr response);

  /**
   * @brief ROS service that dynamically creates an MQTT -> ROS mapping.
   *
   * @param request  service request
   * @param response service response
   */
  void newMqtt2RosBridge(
    mqtt_client_interfaces::srv::NewMqtt2RosBridge::Request::SharedPtr request,
    mqtt_client_interfaces::srv::NewMqtt2RosBridge::Response::SharedPtr response);

  /**
   * @brief Callback for when the client receives a MQTT message from the
   * broker.
   *
   * Overrides mqtt::callback::message_arrived(mqtt::const_message_ptr).
   * If the received MQTT message contains information about a ROS message type,
   * the corresponding ROS publisher is configured. If the received MQTT message
   * is a ROS message, the mqtt2ros conversion is called.
   *
   * @param   mqtt_msg     MQTT message
   */
  void message_arrived(mqtt::const_message_ptr mqtt_msg) override;

  /**
   * @brief Callback for when delivery for a MQTT message has been completed.
   *
   * Overrides mqtt::callback::delivery_complete(mqtt::delivery_token_ptr).
   *
   * @param   token        token tracking the message delivery
   */
  void delivery_complete(mqtt::delivery_token_ptr token) override;

  /**
   * @brief Callback for when a MQTT action succeeds.
   *
   * Overrides mqtt::iaction_listener::on_success(const mqtt::token&).
   * Does nothing.
   *
   * @param   token        token tracking the action
   */
  void on_success(const mqtt::token& token) override;

  /**
   * @brief Callback for when a MQTT action fails.
   *
   * Overrides mqtt::iaction_listener::on_failure(const mqtt::token&).
   * Logs error.
   *
   * @param   token        token tracking the action
   */
  void on_failure(const mqtt::token& token) override;

 protected:
  /**
   * @brief Struct containing broker parameters
   */
  struct BrokerConfig {
    std::string host;  ///< broker host
    int port;          ///< broker port
    std::string user;  ///< username
    std::string pass;  ///< password
    struct {
      bool enabled;  ///< whether to connect via SSL/TLS
      std::filesystem::path
        ca_certificate;  ///< public CA certificate trusted by client
    } tls;               ///< SSL/TLS-related variables
  };

  /**
   * @brief Struct containing client parameters
   */
  struct ClientConfig {
    std::string id;  ///< client unique ID
    struct {
      bool enabled;                     ///< whether client buffer is enabled
      int size;                         ///< client buffer size
      std::filesystem::path directory;  ///< client buffer directory
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
      std::filesystem::path certificate;    ///< client certificate
      std::filesystem::path key;            ///< client private keyfile
      std::string password;                 ///< decryption password for private key
      int version;                          ///< TLS version (https://github.com/eclipse/paho.mqtt.cpp/blob/master/src/mqtt/ssl_options.h#L305)
      bool verify;                          ///< Verify the client should conduct
                                            ///< post-connect checks
      std::vector<std::string> alpn_protos; ///< list of ALPN protocols
    } tls;                   ///< SSL/TLS-related variables
  };

  /**
   * @brief Struct containing variables related to a ROS2MQTT connection.
   */
  struct Ros2MqttInterface {
    struct {
      rclcpp::GenericSubscription::SharedPtr
        subscriber;          ///< generic ROS subscriber
      std::string msg_type;  ///< message type of subscriber
      int queue_size = 1;    ///< ROS subscriber queue size
      bool is_stale = false; ///< whether a new generic publisher/subscriber is required
    } ros;                   ///< ROS-related variables
    struct {
      std::string topic;      ///< MQTT topic
      int qos = 0;            ///< MQTT QoS value
      bool retained = false;  ///< whether to retain MQTT message
    } mqtt;                   ///< MQTT-related variables
    bool primitive = false;   ///< whether to publish as primitive message
    bool stamped = false;     ///< whether to inject timestamp in MQTT message
  };

  /**
   * @brief Struct containing variables related to a MQTT2ROS connection.
   */
  struct Mqtt2RosInterface {
    struct {
      int qos = 0;  ///< MQTT QoS value
    } mqtt;         ///< MQTT-related variables
    struct {
      std::string topic;     ///< ROS topic
      std::string msg_type;  ///< message type of publisher
      rclcpp::GenericPublisher::SharedPtr publisher;  ///< generic ROS publisher
      rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr
        latency_publisher;   ///< ROS publisher for latency
      int queue_size = 1;    ///< ROS publisher queue size
      bool latched = false;  ///< whether to latch ROS message
      bool is_stale = false; ///< whether a new generic publisher/subscriber is required
    } ros;                   ///< ROS-related variables
    bool primitive = false;  ///< whether to publish as primitive message (if
                             ///< coming from non-ROS MQTT client)
    bool stamped = false;    ///< whether timestamp is injected
  };

 protected:
  /**
   * @brief MQTT topic prefix under which ROS message type information is
   * published
   *
   * Must contain trailing '/'.
   */
  static const std::string kRosMsgTypeMqttTopicPrefix;

  /**
   * @brief ROS topic prefix under which ROS2MQTT2ROS latencies are published
   *
   * Must contain trailing '/'.
   */
  static const std::string kLatencyRosTopicPrefix;

  /**
   * @brief Timer to repeatedly check active ROS topics for topics to subscribe
   */
  rclcpp::TimerBase::SharedPtr check_subscriptions_timer_;

  /**
   * @brief ROS Service server for providing connection status
   */
  rclcpp::Service<mqtt_client_interfaces::srv::IsConnected>::SharedPtr
    is_connected_service_;

  /**
   * @brief ROS Service server for providing dynamic ROS to MQTT mappings.
   */
  rclcpp::Service<mqtt_client_interfaces::srv::NewRos2MqttBridge>::SharedPtr
    new_ros2mqtt_bridge_service_;

  /**
   * @brief ROS Service server for providing dynamic MQTT to ROS mappings.
   */
  rclcpp::Service<mqtt_client_interfaces::srv::NewMqtt2RosBridge>::SharedPtr
    new_mqtt2ros_bridge_service_;

  /**
   * @brief Status variable keeping track of connection status to broker
   */
  bool is_connected_ = false;

  /**
   * @brief Broker parameters
   */
  BrokerConfig broker_config_;

  /**
   * @brief Client parameters
   */
  ClientConfig client_config_;

  /**
   * @brief MQTT client variable
   */
  std::shared_ptr<mqtt::async_client> client_;

  /**
   * @brief MQTT client connection options
   */
  mqtt::connect_options connect_options_;

  /**
   * @brief ROS2MQTT connection variables sorted by ROS topic
   */
  std::map<std::string, Ros2MqttInterface> ros2mqtt_;

  /**
   * @brief MQTT2ROS connection variables sorted by MQTT topic
   */
  std::map<std::string, Mqtt2RosInterface> mqtt2ros_;

  /**
   * Message length of a serialized `builtin_interfaces::msg::Time` message
   */
  uint32_t stamp_length_;
};


template <typename T>
bool MqttClient::loadParameter(const std::string& key, T& value) {
  bool found = get_parameter(key, value);
  if (found)
    RCLCPP_DEBUG(get_logger(), "Retrieved parameter '%s' = '%s'", key.c_str(),
                 std::to_string(value).c_str());
  return found;
}


template <typename T>
bool MqttClient::loadParameter(const std::string& key, T& value,
                               const T& default_value) {
  bool found = get_parameter_or(key, value, default_value);
  if (!found)
    RCLCPP_WARN(get_logger(), "Parameter '%s' not set, defaulting to '%s'",
                key.c_str(), std::to_string(default_value).c_str());
  if (found)
    RCLCPP_DEBUG(get_logger(), "Retrieved parameter '%s' = '%s'", key.c_str(),
                 std::to_string(value).c_str());
  return found;
}


template <typename T>
bool MqttClient::loadParameter(const std::string& key, std::vector<T>& value)
{
  const bool found = get_parameter(key, value);
  if (found)
    RCLCPP_WARN(get_logger(), "Retrieved parameter '%s' = '[%s]'", key.c_str(),
                  fmt::format("{}", fmt::join(value, ", ")).c_str());
  return found;
}


template <typename T>
bool MqttClient::loadParameter(const std::string& key, std::vector<T>& value,
                               const std::vector<T>& default_value)
{
  const bool found = get_parameter_or(key, value, default_value);
  if (!found)
    RCLCPP_WARN(get_logger(), "Parameter '%s' not set, defaulting to '%s'",
                key.c_str(), fmt::format("{}", fmt::join(value, ", ")).c_str());
  if (found)
    RCLCPP_DEBUG(get_logger(), "Retrieved parameter '%s' = '%s'", key.c_str(),
                  fmt::format("{}", fmt::join(value, ", ")).c_str());
  return found;
}


/**
 * Serializes a ROS message.
 *
 * @tparam  T                    ROS message type
 *
 * @param[in]   msg              ROS message
 * @param[out]  serialized_msg   serialized message
 */
template <typename T>
void serializeRosMessage(const T& msg,
                         rclcpp::SerializedMessage& serialized_msg) {

  rclcpp::Serialization<T> serializer;
  serializer.serialize_message(&msg, &serialized_msg);
}


/**
 * Deserializes a ROS message.
 *
 * @tparam  T                   ROS message type
 *
 * @param[in]   serialized_msg  serialized message
 * @param[out]  msg             ROS message
 */
template <typename T>
void deserializeRosMessage(const rclcpp::SerializedMessage& serialized_msg,
                           T& msg) {

  rclcpp::Serialization<T> serializer;
  serializer.deserialize_message(&serialized_msg, &msg);
}

}  // namespace mqtt_client
