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

#include <mqtt/async_client.h>
#include "rclcpp/rclcpp.hpp"

#include "rclcpp/logger.hpp"
#include "rcutils/logging_macros.h"
#include "rcpputils/filesystem_helper.hpp"


/**
 * Namespace for the mqtt_client package
 */
namespace mqtt_client {


/**
 * ROS Nodelet for sending and receiving ROS messages via MQTT
 *
 * The MqttClient enables connected ROS-based devices or robots to
 * exchange ROS messages via an MQTT broker using the MQTT protocol.
 * This works generically for any ROS message, i.e. there is no need
 * to specify the ROS message type for ROS messages you wish to
 * exchange via the MQTT broker.
 */
class MqttClient : public rclcpp::Node,
                   public virtual mqtt::callback,
                   public virtual mqtt::iaction_listener{

 public:
  MqttClient();



};
} //Namespace