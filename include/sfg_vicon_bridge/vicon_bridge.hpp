#pragma once

#include "rclcpp/rclcpp.hpp"

namespace sfg_vicon_bridge
{
    class ViconBridge : public rclcpp::Node
    {
    public:
        ViconBridge(const rclcpp::NodeOptions &options = rclcpp::NodeOptions());
    };
}