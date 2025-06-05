#include <gtest/gtest.h>
#include <mqtt/message.h>
#include <std_msgs/msg/int64.hpp>
#include <rclcpp/serialized_message.hpp>
#include "mqtt_client/MqttClient.ros2.hpp"

namespace mqtt_client {
// forward declaration of function under test
bool fixedMqtt2PrimitiveRos(mqtt::const_message_ptr mqtt_msg,
                            const std::string& msg_type,
                            rclcpp::SerializedMessage &serialized_msg);
}

TEST(FixedMqtt2PrimitiveRos, Int64Conversion) {
  auto mqtt_msg = mqtt::make_message("test", "42");
  rclcpp::SerializedMessage serialized;
  ASSERT_TRUE(mqtt_client::fixedMqtt2PrimitiveRos(mqtt_msg,
                                                  "std_msgs/msg/Int64",
                                                  serialized));
  std_msgs::msg::Int64 out;
  mqtt_client::deserializeRosMessage(serialized, out);
  EXPECT_EQ(out.data, 42);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
