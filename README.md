# mqtt_client

<p align="center">
  <img src="https://img.shields.io/github/v/release/ika-rwth-aachen/mqtt_client"/></a>
  <img src="https://img.shields.io/github/license/ika-rwth-aachen/mqtt_client"/></a>
  <a href="https://github.com/ika-rwth-aachen/mqtt_client/actions/workflows/industrial_ci.yml"><img src="https://github.com/ika-rwth-aachen/mqtt_client/actions/workflows/industrial_ci.yml/badge.svg"/></a>
  <a href="https://github.com/ika-rwth-aachen/mqtt_client/actions/workflows/docker-ros.yml"><img src="https://github.com/ika-rwth-aachen/mqtt_client/actions/workflows/docker-ros.yml/badge.svg"/></a>
  <img src="https://img.shields.io/badge/ROS 2-humble|jazzy|kilted|rolling-293754"/>
  <a href="https://github.com/ika-rwth-aachen/mqtt_client"><img src="https://img.shields.io/github/stars/ika-rwth-aachen/mqtt_client?style=social"/></a>
</p>

The *mqtt_client* package provides a ROS 2 component node that enables connected ROS-based devices or robots to exchange ROS messages via an MQTT broker using the [MQTT](http://mqtt.org) protocol. This works generically for arbitrary ROS message types. The *mqtt_client* can also exchange primitive messages with MQTT clients running on devices not based on ROS.

> [!IMPORTANT]  
> This repository is open-sourced and maintained by the [**Institute for Automotive Engineering (ika) at RWTH Aachen University**](https://www.ika.rwth-aachen.de/).  
> **V2X Communication** is one of many research topics within our [*Vehicle Intelligence & Automated Driving*](https://www.ika.rwth-aachen.de/en/competences/fields-of-research/vehicle-intelligence-automated-driving.html) domain.  
> If you would like to learn more about how we can support your automated driving or robotics efforts, feel free to reach out to us!  
> :email: ***opensource@ika.rwth-aachen.de***

- [Installation](#installation)
  - [docker-ros](#docker-ros)
- [Usage](#usage)
  - [Quick Start](#quick-start)
  - [Launch](#launch)
  - [Configuration](#configuration)
- [Primitive Messages](#primitive-messages)
- [Latency Computation](#latency-computation)
- [Package Summary](#package-summary)
- [Acknowledgements](#acknowledgements)

## Installation

The *mqtt_client* package is released as an official ROS 2 package and can easily be installed via a package manager.

```bash
sudo apt update
sudo apt install ros-$ROS_DISTRO-mqtt-client
```

If you would like to install *mqtt_client* from source, simply clone this repository into your ROS 2 workspace. All dependencies that are listed in the ROS [`package.xml`](package.xml) can be installed using [*rosdep*](http://wiki.ros.org/rosdep).

```bash
# mqtt_client$
rosdep install -r --ignore-src --from-paths .

# workspace$
colcon build --packages-up-to mqtt_client --cmake-args -DCMAKE_BUILD_TYPE=Release
```

### docker-ros

*mqtt_client* is also available as a Docker image, containerized through [*docker-ros*](https://github.com/ika-rwth-aachen/docker-ros).

```bash
docker run --rm ghcr.io/ika-rwth-aachen/mqtt_client:latest # or distro-specific tags, e.g., :rolling
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

For a more advanced setup of your own broker, check out our instructions for running an MQTT broker in Docker with enabled authentication and encryption [here](https://github.com/ika-rwth-aachen/mqtt-in-docker).

#### Demo Configuration

The *mqtt_client* is best configured with a ROS parameter *yaml* file. The configuration shown below (also see [`params.yaml`](mqtt_client/config/params.yaml) / [`params.yaml`](mqtt_client/config/params.yaml)) allows an exchange of messages as follows:

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

Launch the *mqtt_client* node with the pre-configured demo parameters:

```bash
ros2 launch mqtt_client standalone.launch.xml
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
# 1st terminal: publish ROS message to /ping
ros2 topic pub /ping/ros std_msgs/msg/String "{data: \"Hello MQTT\"}"
```

```bash
# 2nd terminal: listen for ROS messages on /pong
ros2 topic echo /pong/ros
```

In order to test the communication between *mqtt_client* and other MQTT clients, publish a primitive ROS message on ROS topic `/ping/primitive`, directly publish a primitive MQTT message on MQTT topic `pingpong/primitive` and wait for responses on ROS topic `/pong/primitive`. Note that you need to restart the ROS 2 *mqtt_client* with a different config file.

```bash
# mqtt_client$
ros2 launch mqtt_client standalone.launch.xml params_file:=$(ros2 pkg prefix mqtt_client)/share/mqtt_client/config/params.primitive.yaml
```

```bash
# 3rd terminal: publish primitive ROS message to /ping/primitive
ros2 topic pub /ping/primitive std_msgs/msg/Int32 "{data: 42}"
```

```bash
# 4th terminal: listen for primitive ROS messages on /pong/primitive
ros2 topic echo /pong/primitive
```

```bash
# 5th terminal: publish primitive MQTT message to pingpong/primitive directly using mosquitto_pub
docker run --rm --network host eclipse-mosquitto mosquitto_pub -h localhost -t "pingpong/primitive" --repeat 20 --repeat-delay 1 -m 69
```

If everything works as expected, the second terminal should print a message at 1Hz, while the fourth terminal should print two different messages at 1Hz.

### Launch

You can start the *mqtt_client* node with:

```bash
ros2 launch mqtt_client standalone.launch.xml
```

This will automatically load the provided demo [`params.yaml`](mqtt_client/config/params.yaml) / [`params.yaml`](mqtt_client/config/params.yaml). If you wish to load your custom configuration file, simply pass `params_file`.

```bash
ros2 launch mqtt_client standalone.launch.xml params_file:="</PATH/TO/PARAMS.YAML>"
```

In order to exploit the benefits of *mqtt_client* being a ROS 2 component, load the component into your own component container.

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
  id:                   # unique ID string used to identify the client (broker may allow empty ID and automatically generate one)
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
    version:              # TLS version (https://github.com/eclipse/paho.mqtt.cpp/blob/master/src/mqtt/ssl_options.h#L305)
    verify:               # verify the client should conduct post-connect checks.
    alpn_protos:          # list of ALPN protocols (https://www.openssl.org/docs/man1.1.1/man3/SSL_CTX_set_alpn_protos.html)
```

#### Bridge Parameters

```yaml
bridge:
  ros2mqtt:                # Object specifying which ROS topics to map to which MQTT topics
    ros_topics:            # Array specifying which ROS topics to bridge
      - {{ ros_topic_name }} # The ROS topic that should be bridged, corresponds to the sub-object in the YAML
    {{ ros_topic_name }}:
      mqtt_topic:          # MQTT topic on which the corresponding ROS messages are sent to the broker
      primitive:           # [false] whether to publish as primitive message
      ros_type:            # [*empty*] If set, the ROS msg type provided will be used. If empty, the type is automatically deduced via the publisher
      inject_timestamp:    # [false] whether to attach a timestamp to a ROS2MQTT payload (for latency computation on receiver side)
      advanced:
        ros:
          queue_size:      # [1] ROS subscriber queue size
          qos:
            reliability:   # [auto] One of "auto", "system_default", "reliable", "best_effort". If auto, the QoS is automatically determined via the publisher
            durability:    # [auto] One of "auto", "system_default", "volatile", "transient_local". If auto, the QoS is automatically determined via the publisher
        mqtt:
          qos:             # [0] MQTT QoS value
          retained:        # [false] whether to retain MQTT message
  mqtt2ros:                # Object specifying which MQTT topics to map to which ROS topics
    mqtt_topics:           # Array specifying which ROS topics to bridge
      - {{ mqtt_topic_name }} # The MQTT topic that should be bridged, corresponds to the sub-object in the YAML
    {{ mqtt_topic_name }}:
      ros_topic:           # ROS topic on which corresponding MQTT messages are published
      ros_type:            # [*empty*] If set, the ROS msg type provided will be used. If empty, the type is automatically deduced via the MQTT message
      primitive:           # [false] whether to publish as primitive message (if coming from non-ROS MQTT client)
      advanced:
        mqtt:
          qos:             # [0] MQTT QoS value
        ros:
          queue_size:      # [1] ROS publisher queue size
          latched:         # [false] whether to latch ROS message
          qos:
            reliability:   # [system_default] One of "system_default", "reliable", "best_effort". 
            durability:    # [system_default] One of "system_default", "volatile", "transient_local". 
```

## Primitive Messages

As seen in the [Quick Start](#quick-start), the *mqtt_client* can not only exchange arbitrary ROS messages with other *mqtt_clients*, but it can also exchange primitive message data with other non-*mqtt_client* MQTT clients. This allows ROS-based devices to exchange primitive messages with devices not based on ROS. The `primitive` parameter can be set for both ROS-to-MQTT (`bridge/ros2mqtt`) and for MQTT-to-ROS (`bridge/mqtt2ros`) transmissions.

If a ROS-to-MQTT transmission is configured as `primitive` and the ROS message type is one of the supported primitive ROS message types, the raw data is published as a string. The supported primitive ROS message types are [`std_msgs/String`](http://docs.ros.org/en/api/std_msgs/html/msg/String.html), [`std_msgs/Bool`](http://docs.ros.org/en/api/std_msgs/html/msg/Bool.html), [`std_msgs/Char`](http://docs.ros.org/en/api/std_msgs/html/msg/Char.html), [`std_msgs/UInt8`](http://docs.ros.org/en/api/std_msgs/html/msg/UInt8.html), [`std_msgs/UInt16`](http://docs.ros.org/en/api/std_msgs/html/msg/UInt16.html), [`std_msgs/UInt32`](http://docs.ros.org/en/api/std_msgs/html/msg/UInt32.html), [`std_msgs/UInt64`](http://docs.ros.org/en/api/std_msgs/html/msg/UInt64.html), [`std_msgs/Int8`](http://docs.ros.org/en/api/std_msgs/html/msg/Int8.html), [`std_msgs/Int16`](http://docs.ros.org/en/api/std_msgs/html/msg/Int16.html), [`std_msgs/Int32`](http://docs.ros.org/en/api/std_msgs/html/msg/Int32.html), [`std_msgs/Int64`](http://docs.ros.org/en/api/std_msgs/html/msg/Int64.html), [`std_msgs/Float32`](http://docs.ros.org/en/api/std_msgs/html/msg/Float32.html), [`std_msgs/Float64`](http://docs.ros.org/en/api/std_msgs/html/msg/Float64.html).

If an MQTT-to-ROS transmission is configured as `primitive`, the MQTT message is interpreted and published as a primitive data type, if possible. The message is probed in the following order: `bool` ([`std_msgs/Bool`](http://docs.ros.org/en/api/std_msgs/html/msg/Bool.html)), `int` ([`std_msgs/Int32`](http://docs.ros.org/en/api/std_msgs/html/msg/Int32.html)), `float` ([`std_msgs/Float32`](http://docs.ros.org/en/api/std_msgs/html/msg/Float32.html)), `string` ([`std_msgs/String`](http://docs.ros.org/en/api/std_msgs/html/msg/String.html)).


## Latency Computation

The *mqtt_client* provides built-in functionality to measure the latency of transferring a ROS message via an MQTT broker back to ROS. Note that this functionality is only available for non-primitive messages (see [Primitive Messages](#primitive-messages)). To this end, the sending client injects the current timestamp into the MQTT message. The receiving client can then compute the latency between message reception time and the injected timestamp. **Naturally, this is only accurate to the level of synchronization between clocks on sending and receiving machine.**

In order to inject the current timestamp into outgoing MQTT messages, the parameter `inject_timestamp` has to be set for the corresponding `bridge/ros2mqtt` entry. The receiving *mqtt_client* will then automatically publish the measured latency in seconds as a ROS `std_msgs/Float64` message on topic `/<mqtt_client_name>/latencies/<mqtt2ros/ros_topic>`.

These latencies can be printed easily with:

```bash
ros2 topic echo /<mqtt_client_name>/latencies/<mqtt2ros/ros_topic>/data
```

or plotted with [rqt_plot](http://wiki.ros.org/rqt_plot):

```bash
ros2 run rqt_plot rqt_plot /<mqtt_client_name>/latencies/<mqtt2ros/ros_topic>/data
```


## Package Summary

This short package summary documents the package in line with the [ROS Wiki Style Guide](http://wiki.ros.org/StyleGuide).

### Components

#### `mqtt_client/MqttClient`

Enables connected ROS-based devices or robots to exchange ROS messages via an MQTT broker using the [MQTT](http://mqtt.org) protocol.

##### Subscribed Topics

- `<bridge/ros2mqtt/ros_topic>` ([`rclcpp::SerializedMessage`](https://docs.ros.org/en/ros2_packages/rolling/api/rclcpp/generated/classrclcpp_1_1GenericSubscription.html))
  ROS topic whose messages are transformed to MQTT messages and sent to the MQTT broker. May have arbitrary ROS message type.

##### Published Topics

- `<bridge/mqtt2ros/ros_topic>` ([`rclcpp::SerializedMessage`](https://docs.ros.org/en/ros2_packages/rolling/api/rclcpp/generated/classrclcpp_1_1GenericPublisher.html))
  ROS topic on which MQTT messages received from the MQTT broker are published. May have arbitrary ROS message type.
- `~/latencies/<bridge/mqtt2ros/ros_topic>` ([`std_msgs/Float64`](https://docs.ros.org/en/api/std_msgs/html/msg/Float64.html))
  Latencies measured on the message transfer to `<bridge/mqtt2ros/ros_topic>` are published here, if the received messages have a timestamp injected (see [Latency Computation](#latency-computation)).

##### Services

- `~/is_connected` ([`mqtt_client/srv/IsConnected`](mqtt_client_interfaces/srv/IsConnected.srv))
  Returns whether the client is connected to the MQTT broker.

- `~/new_ros2mqtt_bridge` ([`mqtt_client/srv/NewRos2MqttBridge`](mqtt_client_interfaces/srv/NewRos2MqttBridge.srv))
  Returns whether a new ROS -> MQTT bridge was created.

- `~/new_mqtt2ros_bridge` ([`mqtt_client/srv/NewMqtt2RosBridge`](mqtt_client_interfaces/srv/NewMqtt2RosBridge.srv))
  Returns whether a new MQTT -> ROS bridge was created.

##### Parameters

See [Configuration](#configuration).


## Acknowledgements

This research is accomplished within the projects [6GEM](https://6gem.de/) (FKZ 16KISK036K) and [UNICAR*agil*](https://www.unicaragil.de/) (FKZ 16EMO0284K). We acknowledge the financial support for the projects by the Federal Ministry of Education and Research of Germany (BMBF).
