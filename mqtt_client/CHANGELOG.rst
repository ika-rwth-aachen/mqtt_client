^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package mqtt_client
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

2.2.1 (2024-03-19)
------------------
* Merge pull request #50 from babakc/main
  Amend AWS IoT CLI command in collect the correct endpoint
* Contributors: Lennart Reiher

2.2.0 (2023-11-29)
------------------
* Merge pull request `#35 <https://github.com/ika-rwth-aachen/mqtt_client/issues/35>`_ from mvccogo/main
  Dynamic registration of topics
* Merge pull request `#36 <https://github.com/ika-rwth-aachen/mqtt_client/issues/36>`_ from ika-rwth-aachen/fix/ros1-latencies
  Fix bug in ros1 latency deserialization
* Contributors: Lennart Reiher, Matheus V. C. Cogo, mvccogo

2.1.0 (2023-09-18)
------------------
* Merge pull request #31 from ika-rwth-aachen/features/ros2-component
  ROS2 Component
* Merge pull request #30 from oxin-ros/ros2-add-multiple-topics
  ROS 2: add multiple topics
* Merge pull request #28 from oxin-ros/add-ALPN-protocol-support-for-aws
  Add ALPN protocol support for AWS
* Contributors: David B, David Buckman, Lennart Reiher

2.0.1 (2023-06-10)
------------------
* fix unrecognized build type with catkin_make_isolated
  order of statements is somehow revelant; catkin_make_isolated would not detect the build type; build farm jobs were failing; https://build.ros.org/job/Ndev__mqtt_client__ubuntu_focal_amd64/10/console
* Contributors: Lennart Reiher

2.0.0 (2023-06-10)
------------------
* Merge pull request #23 from ika-rwth-aachen/docker-ros
  Integrate docker-ros
* Merge pull request #16 from ika-rwth-aachen/dev/ros2
  Add support for ROS2
* Contributors: Lennart Reiher

1.1.0 (2022-10-13)
------------------
* Merge pull request #6 from ika-rwth-aachen/feature/primitive-msgs
  Support exchange of primitive messages with other MQTT clients
* Contributors: Lennart Reiher

1.0.2 (2022-10-07)
------------------
* Merge pull request #4 from ika-rwth-aachen/improvement/runtime-optimization
  Optimize runtime
* Merge pull request #5 from ika-rwth-aachen/ci
  Set up CI
* Contributors: Lennart Reiher

1.0.1 (2022-08-11)
------------------
* Merge pull request #3 from ika-rwth-aachen/doc/code-api
  Improve Code API Documentation
* Merge pull request #1 from ika-rwth-aachen/improvement/documentation
  Improve documentation
* Contributors: Lennart Reiher
