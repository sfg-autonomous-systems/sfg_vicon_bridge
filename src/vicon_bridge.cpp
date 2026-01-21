#include "sfg_vicon_bridge/vicon_bridge.hpp"

namespace sfg_vicon_bridge
{
    ViconBridge::ViconBridge(const rclcpp::NodeOptions &options) : Node("vicon_bridge", options)
    {
        RCLCPP_INFO(this->get_logger(), "Started Vicon bridge.");
    }
}