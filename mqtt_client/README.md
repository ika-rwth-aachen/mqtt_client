## Full Documentation from GitHub

## Nodelets

### mqtt_client/MqttClient

Enables connected ROS-based devices or robots to exchange ROS messages
via an MQTT broker using the MQTT protocol.

#### Subscribed Topics

- `<bridge/ros2mqtt[*]/ros_topic>`
  ([topic_tools/msg/ShapeShifter](https://docs.ros.org/en/rolling/p/topic_tools/msg/ShapeShifter.html)):
  ROS topic whose messages are transformed to MQTT messages and sent to
  the MQTT broker. May have arbitrary ROS message type.

#### Published Topics

- `<bridge/mqtt2ros[*]/ros_topic>`
  ([topic_tools/msg/ShapeShifter](https://docs.ros.org/en/rolling/p/topic_tools/msg/ShapeShifter.html)):
  ROS topic on which MQTT messages received from the MQTT broker are
  published. May have arbitrary ROS message type.
- `~/latencies/<bridge/mqtt2ros[*]/ros_topic>`
  ([std_msgs/msg/Float64](https://docs.ros.org/en/rolling/p/std_msgs/msg/Float64.html)):
  Latencies measured on the message transfer to
  <bridge/mqtt2ros\[\*\]/ros_topic> are published here, if the
  received messages have a timestamp injected.

#### Services

- `is_connected`
  ([mqtt_client/srv/IsConnected](https://docs.ros.org/en/rolling/p/mqtt_client/srv/IsConnected.html)):
  Returns whether the client is connected to the MQTT broker.

#### Parameters

- `~broker/host` (`str`, default: `localhost`): IP address or hostname
  of the machine running the MQTT broker
- `~broker/port` (`int`, default: `1883`): port the MQTT broker is
  listening on
- `~broker/user` (`str`): username used for authenticating to the broker
  (if empty, will try to connect anonymously)
- `~broker/pass` (`str`): password used for authenticating to the broker
- `~broker/tls/enabled` (`bool`, default: `false`): whether to connect
  via SSL/TLS
- `~broker/tls/ca_certificate` (`str`, default:
  `/etc/ssl/certs/ca-certificates.crt`): CA certificate file trusted by
  client (relative to ROS_HOME)
- `~client/id` (`str`): unique ID used to identify the client (broker
  may allow empty ID and automatically generate one)
- `~client/buffer/size` (`int`, default: `0`): maximum number of
  messages buffered by the bridge when not connected to broker (only
  available if client ID is not empty)
- `~client/buffer/directory` (`str`, default: `buffer`): directory used
  to buffer messages when not connected to broker (relative to ROS_HOME)
- `~client/last_will/topic` (`str`): topic used for this client's
  last-will message (no last will, if not specified)
- `~client/last_will/message` (`str`, default: `offline`): last-will
  message
- `~client/last_will/qos` (`int`, default: `0`): QoS value for last-will
  message
- `~client/last_will/retained` (`bool`, default: `false`): whether to
  retain last-will message
- `~client/clean_session` (`bool`, default: `true`): whether to use a
  clean session for this client
- `~client/keep_alive_interval` (`float`, default: `60.0`): keep-alive
  interval in seconds
- `~client/max_inflight` (`int`, default: `65535`): maximum number of
  inflight messages
- `~client/tls/certificate` (`str`): client certificate file (only
  needed if broker requires client certificates; relative to ROS_HOME)
- `~client/tls/key` (`str`): client private key file (relative to
  ROS_HOME)
- `~client/tls/password` (`str`): client private key password
- `~bridge/ros2mqtt/[ros_topic]` (`str`): ROS topic whose messages are
  transformed to MQTT messages
- `~bridge/ros2mqtt/[mqtt_topic]` (`str`): MQTT topic on which the
  corresponding ROS messages are sent to the broker
- `~bridge/ros2mqtt/[inject_timestamp]` (`bool`, default: `false`):
  whether to attach a timestamp to a ROS2MQTT payload (for latency
  computation on receiver side)
- `~bridge/ros2mqtt/[advanced/ros/queue_size]` (`int`, default: `1`):
  ROS subscriber queue size
- `~bridge/ros2mqtt/[advanced/mqtt/qos]` (`int`, default: `0`): MQTT QoS
  value
- `~bridge/ros2mqtt/[advanced/mqtt/retained]` (`bool`, default:
  `false`): whether to retain MQTT message
- `~bridge/mqtt2ros/[mqtt_topic]` (`str`): MQTT topic on which messages
  are received from the broker
- `~bridge/mqtt2ros/[ros_topic]` (`str`): ROS topic on which
  corresponding MQTT messages are published
- `~bridge/mqtt2ros/[advanced/mqtt/qos]` (`int`, default: `0`): MQTT QoS
  value
- `~bridge/mqtt2ros/[advanced/ros/queue_size]` (`int`, default: `1`):
  ROS publisher queue size
- `~bridge/mqtt2ros/[advanced/ros/latched]` (`bool`, default: `false`):
  whether to latch ROS message
