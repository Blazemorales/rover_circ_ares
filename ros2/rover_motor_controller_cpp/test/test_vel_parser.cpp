// Over-limit commands (either sign) must produce the same motor command as
// the limit itself.
#include <gtest/gtest.h>

#include "geometry_msgs/msg/twist.hpp"
#include "motor_controller/vel_parser_node.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rover_msgs/msg/motors_command.hpp"

using motor_controller::VelParserNode;
using rover_msgs::msg::MotorsCommand;

static MotorsCommand parse(double linear, double angular) {
  auto parser = std::make_shared<VelParserNode>();
  auto probe = std::make_shared<rclcpp::Node>("probe");
  MotorsCommand out;
  bool got = false;
  auto sub = probe->create_subscription<MotorsCommand>(
      "motors_command", 10, [&](const MotorsCommand &m) {
        out = m;
        got = true;
      });
  auto twist = std::make_shared<geometry_msgs::msg::Twist>();
  twist->linear.x = linear;
  twist->angular.z = angular;

  rclcpp::executors::SingleThreadedExecutor exec;
  exec.add_node(parser);
  exec.add_node(probe);
  for (int i = 0; i < 50 && !got; i++) {
    parser->callback(twist);
    exec.spin_some(std::chrono::milliseconds(20));
  }
  EXPECT_TRUE(got);
  return out;
}

TEST(VelParser, ClampsBothSigns) {
  for (double s : {1.0, -1.0}) {
    auto at_limit = parse(s, s);
    auto over = parse(5 * s, 5 * s);
    EXPECT_EQ(over.drive_motor, at_limit.drive_motor);
    EXPECT_EQ(over.corner_motor, at_limit.corner_motor);
    for (int d : over.drive_motor) {
      EXPECT_LE(std::abs(d), 1000);
    }
  }
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  rclcpp::init(argc, argv);
  int r = RUN_ALL_TESTS();
  rclcpp::shutdown();
  return r;
}
