# mqtt_client

<p align="center">
  <img src="https://img.shields.io/badge/ROS1-noetic-green"/></a>
  <img src="https://img.shields.io/github/v/release/ika-rwth-aachen/mqtt_client"/></a>
  <img src="https://img.shields.io/github/license/ika-rwth-aachen/mqtt_client"/></a>
  <a href="https://github.com/ika-rwth-aachen/mqtt_client/actions/workflows/build.yml"><img src="https://github.com/ika-rwth-aachen/mqtt_client/actions/workflows/build.yml/badge.svg"/></a>
  <a href="https://github.com/ika-rwth-aachen/mqtt_client"><img src="https://img.shields.io/github/stars/ika-rwth-aachen/mqtt_client?style=social"/></a>
</p>

The *mqtt_client* package provides a ROS nodelet that enables connected ROS-based devices or robots to exchange ROS messages via an MQTT broker using the [MQTT](http://mqtt.org) protocol. This works generically for arbitrary ROS message types. The *mqtt_client* can also exchange primitive messages with MQTT clients running on devices not based on ROS.

- [Installation](#installation)
- [Usage](#usage)
  - [Quick Start](#quick-start)
  - [Launch](#launch)
  - [Configuration](#configuration)
- [Primitive Messages](#primitive-messages)
- [Latency Computation](#latency-computation)
- [Package Summary](#package-summary)
- [How It Works](#how-it-works)
- [Acknowledgements](#acknowledgements)

## Installation

The *mqtt_client* package is released as an official ROS Noetic package and can easily be installed via a package manager.

```bash
sudo apt install ros-noetic-mqtt-client
```

If you would like to install *mqtt_client* from source, simply clone this repository into your ROS workspace. All dependencies that are listed in the ROS [`package.xml`](package.xml) can be installed using [*rosdep*](http://wiki.ros.org/rosdep).

```bash
# mqtt_client$
rosdep install -r --ignore-src --from-paths .
```

## Usage

The *mqtt_client* can be easily integrated into an existing ROS-based system. Below, you first find a quick start guide to test the *mqtt_client* on a single machine. Then, more details are presented on how to launch and configure it in more complex applications.

### Quick Start

Follow these steps to quickly launch a working *mqtt_client* that is sending ROS messages via an MQTT broker to itself.

#### Demo Broker

It is assumed that an MQTT broker (such as [*Mosquitto*](https://mosquitto.org/)) is running on `localhost:1883`.

For this demo, you may easily launch *Mosquitto* with its default configuration using *Docker*.

```bash
docker run --rm --network host --name mosquitto eclipse-mosquitto
```

#### Demo Configuration

The *mqtt_client* is best configured with a ROS parameter *yaml* file. The configuration shown below (also see [`params.yaml`](launch/params.yaml)) allows an exchange of messages as follows:

- ROS messages received locally on ROS topic `/ping/ros` are sent to the broker on MQTT topic `pingpong/ros`;
- MQTT messages received from the broker on MQTT topic `pingpong/ros` are published locally on ROS topic `/pong/ros`;
- primitive ROS messages received locally on ROS topic `/ping/primitive` are sent as primitive (string) messages to the broker on MQTT topic `pingpong/primitive`;
- MQTT messages received from the broker on MQTT topic `pingpong/primitive` are published locally as primitive ROS messages on ROS topic `/pong/primitive`.

```yaml
broker:
  host: localhost
  port: 1883
bridge:
  ros2mqtt:
    - ros_topic: /ping/ros
      mqtt_topic: pingpong/ros
    - ros_topic: /ping/primitive
      mqtt_topic: pingpong/primitive
      primitive: true
  mqtt2ros:
    - mqtt_topic: pingpong/ros
      ros_topic: /pong/ros
    - mqtt_topic: pingpong/primitive
      ros_topic: /pong/primitive
      primitive: true
```

#### Demo Client Launch

After building your ROS workspace, launch the *mqtt_client* nodelet with the pre-configured demo parameters using *roslaunch*, which should yield the following output.

```bash
roslaunch mqtt_client standalone.launch
```

```txt
[ WARN] [1665575657.358869079]: Parameter 'broker/tls/enabled' not set, defaulting to '0'
[ WARN] [1665575657.359798329]: Parameter 'client/id' not set, defaulting to ''
[ WARN] [1665575657.359810889]: Client buffer can not be enabled when client ID is empty
[ WARN] [1665575657.360300703]: Parameter 'client/clean_session' not set, defaulting to '1'
[ WARN] [1665575657.360576344]: Parameter 'client/keep_alive_interval' not set, defaulting to '60.000000'
[ WARN] [1665575657.360847295]: Parameter 'client/max_inflight' not set, defaulting to '65535'
[ INFO] [1665575657.361281461]: Bridging ROS topic '/ping/ros' to MQTT topic 'pingpong/ros' 
[ INFO] [1665575657.361303380]: Bridging primitive ROS topic '/ping/primitive' to MQTT topic 'pingpong/primitive' 
[ INFO] [1665575657.361352809]: Bridging MQTT topic 'pingpong/ros' to ROS topic '/pong/ros'
[ INFO] [1665575657.361370558]: Bridging MQTT topic 'pingpong/primitive' to primitive ROS topic '/pong/primitive'
[ INFO] [1665575657.362153083]: Connecting to broker at 'tcp://localhost:1883' ...
[ INFO] [1665575657.462622065]: Connected to broker at 'tcp://localhost:1883'
```

Note that the *mqtt_client* successfully connected to the broker and also echoed which ROS/MQTT topics are being bridged. For testing the communication between *mqtt_client*, itself, and other MQTT clients, open five new terminals.

In order to test the communication among *mqtt_clients*, publish any ROS message on ROS topic `/ping/ros` and wait for a response on ROS topic `/pong/ros`.

```bash
# 1st terminal: listen for ROS messages on /pong/ros
rostopic echo /pong/ros
```

```bash
# 2nd terminal: publish ROS message to /ping/ros
rostopic pub -r 1 /ping/ros std_msgs/String "Hello MQTT!"
```

In order to test the communication between *mqtt_client* and other MQTT clients, publish a primitive ROS message on ROS topic `/ping/primitive`, directly publish a primitive MQTT message on MQTT topic `pingpong/primitive` and wait for responses on ROS topic `/pong/primitive`.

```bash
# 3rd terminal: listen for primitive ROS messages on /pong/primitive
rostopic echo /pong/primitive
```

```bash
# 4th terminal: publish primitive ROS message to /ping/primitive
rostopic pub -r 1 /ping/primitive std_msgs/Int32 42
```

```bash
# 5th terminal: publish primitive MQTT message to pingpong/primitive
docker run --rm --network host eclipse-mosquitto mosquitto_pub -h localhost -t "pingpong/primitive" --repeat 20 --repeat-delay 1 -m 69
```

If everything works as expected, the second terminal should print a message at 1Hz, while the third terminal should print two different messages at 1Hz.

### Launch

You can start the *mqtt_client* nodelet in a standalone nodelet manager with:

```bash
roslaunch mqtt_client standalone.launch
```

This will automatically load the provided demo [params.yaml](launch/params.yaml) to the ROS parameter server. If you wish to load your custom configuration file, simply pass `params_file`.

```bash
roslaunch mqtt_client standalone.launch params_file:="</PATH/TO/PARAMS.YAML>"
```

You can also disable parameter loading altogether by passing `load_params:=false`. In this case, make sure to populate the ROS parameter server accordingly with other means.

```bash
roslaunch mqtt_client standalone.launch load_params:=false
```

In order to exploit the benefits of *mqtt_client* being a nodelet, load the nodelet to your own nodelet manager shared with other nodelets.

### Configuration

All available ROS parameters supported by the *mqtt_client* and their default values (in `[]`) are listed in the following.

#### Broker Parameters

```yaml
broker:
  host:              # [localhost] IP address or hostname of the machine running the MQTT broker
  port:              # [1883] port the MQTT broker is listening on
  user:              # username used for authenticating to the broker (if empty, will try to connect anonymously)
  pass:              # password used for authenticating to the broker
  tls:
    enabled:           # [false] whether to connect via SSL/TLS
    ca_certificate:    # [/etc/ssl/certs/ca-certificates.crt] CA certificate file trusted by client (relative to ROS_HOME)
```

#### Client Parameters

```yaml
client:
  id:                   # unique ID used to identify the client (broker may allow empty ID and automatically generate one)
  buffer:
    size:                 # [0] maximum number of messages buffered by the bridge when not connected to broker (only available if client ID is not empty)
    directory:            # [buffer] directory used to buffer messages when not connected to broker (relative to ROS_HOME)
  last_will:
    topic:                # topic used for this client's last-will message (no last will, if not specified)
    message:              # [offline] last-will message
    qos:                  # [0] QoS value for last-will message
    retained:             # [false] whether to retain last-will message
  clean_session:        # [true] whether to use a clean session for this client
  keep_alive_interval:  # [60.0] keep-alive interval in seconds
  max_inflight:         # [65535] maximum number of inflight messages
  tls:
    certificate:          # client certificate file (only needed if broker requires client certificates; relative to ROS_HOME)
    key:                  # client private key file (relative to ROS_HOME)
    password:             # client private key password
```

#### Bridge Parameters

```yaml
bridge:
  ros2mqtt:            # array specifying which ROS topics to map to which MQTT topics
    - ros_topic:         # ROS topic whose messages are transformed to MQTT messages
      mqtt_topic:        # MQTT topic on which the corresponding ROS messages are sent to the broker
      primitive:         # [false] whether to publish as primitive message
      inject_timestamp:  # [false] whether to attach a timestamp to a ROS2MQTT payload (for latency computation on receiver side)
      advanced:
        ros:
          queue_size:      # [1] ROS subscriber queue size
        mqtt:
          qos:             # [0] MQTT QoS value
          retained:        # [false] whether to retain MQTT message
  mqtt2ros:            # array specifying which MQTT topics to map to which ROS topics
    - mqtt_topic:        # MQTT topic on which messages are received from the broker
      ros_topic:         # ROS topic on which corresponding MQTT messages are published
      primitive:         # [false] whether to publish as primitive message (if coming from non-ROS MQTT client)
      advanced:
        mqtt:
          qos:             # [0] MQTT QoS value
        ros:
          queue_size:        # [1] ROS publisher queue size
          latched:           # [false] whether to latch ROS message
```

## Primitive Messages

As seen in the [Quick Start](#quick-start), the *mqtt_client* can not only exchange arbitrary ROS messages with other *mqtt_clients*, but it can also exchange primitive message data with other non-*mqtt_client* MQTT clients. This allows ROS-based devices to exchange primitive messages with devices not based on ROS. The `primitive` parameter can be set for both ROS-to-MQTT (`bridge/ros2mqtt`) and for MQTT-to-ROS (`bridge/mqtt2ros`) transmissions.

If a ROS-to-MQTT transmission is configured as `primitive`, the ROS message is simply serialized to a string representation, without providing any information on the underlying ROS message type via MQTT. If the ROS message type is one of the supported primitive ROS message types, the encapsulating ROS message components are also removed, s.t. only the raw data is published as a string. The supported primitive ROS message types are [`std_msgs/String`](http://docs.ros.org/en/api/std_msgs/html/msg/String.html), [`std_msgs/Bool`](http://docs.ros.org/en/api/std_msgs/html/msg/Bool.html), [`std_msgs/Char`](http://docs.ros.org/en/api/std_msgs/html/msg/Char.html), [`std_msgs/UInt8`](http://docs.ros.org/en/api/std_msgs/html/msg/UInt8.html), [`std_msgs/UInt16`](http://docs.ros.org/en/api/std_msgs/html/msg/UInt16.html), [`std_msgs/UInt32`](http://docs.ros.org/en/api/std_msgs/html/msg/UInt32.html), [`std_msgs/UInt64`](http://docs.ros.org/en/api/std_msgs/html/msg/UInt16.html), [`std_msgs/Int8`](http://docs.ros.org/en/api/std_msgs/html/msg/Int8.html), [`std_msgs/Int16`](http://docs.ros.org/en/api/std_msgs/html/msg/Int16.html), [`std_msgs/Int32`](http://docs.ros.org/en/api/std_msgs/html/msg/Int32.html), [`std_msgs/Int64`](http://docs.ros.org/en/api/std_msgs/html/msg/Int64.html), [`std_msgs/Float32`](http://docs.ros.org/en/api/std_msgs/html/msg/Float32.html), [`std_msgs/Float32`](http://docs.ros.org/en/api/std_msgs/html/msg/Float64.html).

If an MQTT-to-ROS transmission is configured as `primitive`, the MQTT message is interpreted and published as a primitive data type, if possible. The message is probed in the following order: `bool` ([`std_msgs/Bool`](http://docs.ros.org/en/api/std_msgs/html/msg/Bool.html)), `int` ([`std_msgs/Int32`](http://docs.ros.org/en/api/std_msgs/html/msg/Int32.html)), `float` ([`std_msgs/Float32`](http://docs.ros.org/en/api/std_msgs/html/msg/Float32.html)), `string` ([`std_msgs/String`](http://docs.ros.org/en/api/std_msgs/html/msg/String.html)).

## Latency Computation

The *mqtt_client* provides built-in functionality to measure the latency of transferring a ROS message via an MQTT broker back to ROS. Note that this functionality is only available for non-primitive messages (see [Primitive Messages](#primitive-messages)). To this end, the sending client injects the current timestamp into the MQTT message. The receiving client can then compute the latency between message reception time and the injected timestamp. **Naturally, this is only accurate to the level of synchronization between clocks on sending and receiving machine.**

In order to inject the current timestamp into outgoing MQTT messages, the parameter `inject_timestamp` has to be set for the corresponding `bridge/ros2mqtt` entry. The receiving *mqtt_client* will then automatically publish the measured latency in seconds as a ROS `std_msgs/Float64` message on topic `/<mqtt_client_name>/latencies/<mqtt2ros/ros_topic>`.

These latencies can be printed easily with *rostopic echo*

```bash
rostopic echo --clear /<mqtt_client_name>/latencies/<mqtt2ros/ros_topic>/data
```

or plotted with [rqt_plot](http://wiki.ros.org/rqt_plot):

```bash
rqt_plot /<mqtt_client_name>/latencies/<mqtt2ros/ros_topic>/data
```

## Package Summary

This short package summary documents the package in line with the [ROS Wiki Style Guide](http://wiki.ros.org/StyleGuide).

### Nodelets

#### `mqtt_client/MqttClient`

Enables connected ROS-based devices or robots to exchange ROS messages via an MQTT broker using the [MQTT](http://mqtt.org) protocol.

##### Subscribed Topics

- `<bridge/ros2mqtt[*]/ros_topic>` ([`topic_tools/ShapeShifter`](http://wiki.ros.org/topic_tools))  
  ROS topic whose messages are transformed to MQTT messages and sent to the MQTT broker. May have arbitrary ROS message type.

##### Published Topics

- `<bridge/mqtt2ros[*]/ros_topic>` ([`topic_tools/ShapeShifter`](http://wiki.ros.org/topic_tools))  
  ROS topic on which MQTT messages received from the MQTT broker are published. May have arbitrary ROS message type.
- `~/latencies/<bridge/mqtt2ros[*]/ros_topic>` ([`std_msgs/Float64`](https://docs.ros.org/en/api/std_msgs/html/msg/Float64.html))  
  Latencies measured on the message transfer to `<bridge/mqtt2ros[*]/ros_topic>` are published here, if the received messages have a timestamp injected (see [Latency Computation](#latency-computation)).

##### Services

- `is_connected` ([`mqtt_client/IsConnected`](srv/IsConnected.srv))  
  Returns whether the client is connected to the MQTT broker.

##### Parameters

See [Configuration](#configuration).

## How It Works

The *mqtt_client* is able to bridge ROS messages of arbitrary message type to an MQTT broker. To this end, it needs to employ generic ROS subscribers and publishers, which only take shape at runtime.

These generic ROS subscribers and publishers are realized through [topic_tools::ShapeShifter](http://docs.ros.org/diamondback/api/topic_tools/html/classtopic__tools_1_1ShapeShifter.html). For each pair of `ros_topic` and `mqtt_topic` specified under `bridge/ros2mqtt/`, a ROS subscriber is setup with the following callback signature:

```cpp
void ros2mqtt(topic_tools::ShapeShifter::ConstPtr&, std::string&)
```

Inside the callback, the generic messages received on the `ros_topic` are serialized using [ros::serialization](http://wiki.ros.org/roscpp/Overview/MessagesSerializationAndAdaptingTypes). The serialized form is then ready to be sent to the MQTT broker on the specified `mqtt_topic`.

Upon retrieval of an MQTT message, it is republished as a ROS message on the ROS network. To this end, [topic_tools::ShapeShifter::morph](http://docs.ros.org/indigo/api/topic_tools/html/classtopic__tools_1_1ShapeShifter.html#a2b74b522fb49dac05d48f466b6b87d2d) is used to have the ShapeShifter publisher take the shape of the specific ROS message type.

The required metainformation on the ROS message type can however only be extracted in the ROS subscriber callback of the publishing *mqtt_client* with calls to [topic_tools::ShapeShifter::getMD5Sum](http://docs.ros.org/indigo/api/topic_tools/html/classtopic__tools_1_1ShapeShifter.html#af05fbf42757658e4c6a0f99ff72f7daa), [topic_tools::ShapeShifter::getDataType](http://docs.ros.org/indigo/api/topic_tools/html/classtopic__tools_1_1ShapeShifter.html#a9d57b2285213fda5e878ce7ebc42f0fb), and [topic_tools::ShapeShifter::getMessageDefinition](http://docs.ros.org/indigo/api/topic_tools/html/classtopic__tools_1_1ShapeShifter.html#a503af7234eeba0ccefca64c4d0cc1df0). These attributes are wrapped in a ROS message of custom type [mqtt_client::RosMsgType](msg/RosMsgType.msg), serialized using [ros::serialization](http://wiki.ros.org/roscpp/Overview/MessagesSerializationAndAdaptingTypes) and also shared via the MQTT broker on a special topic.

When an *mqtt_client* receives such ROS message type metainformation, it configures the corresponding ROS ShapeShifter publisher using [topic_tools::ShapeShifter::morph](http://docs.ros.org/indigo/api/topic_tools/html/classtopic__tools_1_1ShapeShifter.html#a2b74b522fb49dac05d48f466b6b87d2d).

The *mqtt_client* also provides functionality to measure the latency of transferring a ROS message via an MQTT broker back to ROS. To this end, the sending client injects the current timestamp into the MQTT message. The receiving client can then compute the latency between message reception time and the injected timestamp. Since injection of the timestamp is optional, an extra bit of information is needed for the receiver to correctly decode the MQTT message. Therefore, the first entry in the `std::vector<uint8>` message buffer is used to indicate whether the message includes an injected timestamp. The resulting `std::vector<uint8>` payload takes on one of the following forms:

```txt
[ 1 | ... serialized timestamp ... | ... serialized ROS messsage ...]
[ 0 | ... serialized ROS messsage ...]
```

To summarize, the dataflow is as follows:

- a ROS message of arbitrary type is received on ROS topic `<ros2mqtt_ros_topic>` and passed to the generic callback
  - ROS message type information is extracted and wrapped as a `RosMsgType`
  - ROS message type information is serialized and sent via the MQTT broker on MQTT topic `mqtt_client/ros_msg_type/<ros2mqtt_mqtt_topic>`
  - the actual ROS message is serialized
  - if `inject_timestamp`, the current timestamp is serialized and concatenated with the message
  - an integer is added to the message's head indicating whether a timestamp was injected
  - the actual MQTT message is sent via the MQTT broker on MQTT topic `<ros2mqtt_mqtt_topic>`
- an MQTT message containing the ROS message type information is received on MQTT topic `mqtt_client/ros_msg_type/<ros2mqtt_mqtt_topic>`
  - message type information is extracted and the ShapeShifter ROS publisher is configured
- an MQTT message containing the actual ROS message is received
  - depending on the first element of the message, it is decoded into the serialized ROS message and the serialized timestamp
  - if the message contained a timestamp, the latency is computed and published on ROS topic `~/latencies/<mqtt2ros_ros_topic>`
  - the serialized ROS message is published using the *ShapeShifter* on ROS topic `<mqtt2ros_ros_topic>`

## Acknowledgements

This research is accomplished within the projects [6GEM](https://6gem.de/) (FKZ 16KISK036K) and [UNICAR*agil*](https://www.unicaragil.de/) (FKZ 16EMO0284K). We acknowledge the financial support for the projects by the Federal Ministry of Education and Research of Germany (BMBF).
