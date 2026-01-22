#pragma once

#include <rclcpp/rclcpp.hpp>
#include <tf2_ros/static_transform_broadcaster.hpp>
#include <tf2_ros/transform_broadcaster.hpp>
#include <ViconDataStreamSDK/DataStreamClient.h>

namespace sfg_vicon_bridge
{
    class ViconBridge : public rclcpp::Node
    {
    public:
        ViconBridge(const rclcpp::NodeOptions &options = rclcpp::NodeOptions());

    private:
        // ROS parameters
        std::string m_hostname;
        std::uint32_t m_buffer_size;

        ViconDataStreamSDK::CPP::Client m_client;

        tf2_ros::TransformBroadcaster m_broadcaster;
        tf2_ros::StaticTransformBroadcaster m_static_broadcaster;
        std::unordered_map<std::string, std::unique_ptr<rclcpp::Publisher<geometry_msgs::msg::TransformStamped>>> m_publishers;
    };
}