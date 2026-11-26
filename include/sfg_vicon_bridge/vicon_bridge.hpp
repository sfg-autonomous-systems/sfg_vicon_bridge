#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <string>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_broadcaster.hpp>
#include <tf2_ros/transform_listener.h>
#include <thread>
#include <unordered_map>

// Forward declare the client so we don't need ViconDataStreamSDK_CPP as a public dependency.
namespace ViconDataStreamSDK::CPP
{
    class Client;
}

namespace sfg_vicon_bridge
{
    class ViconBridge final : public rclcpp::Node
    {
    public:
        ViconBridge(const rclcpp::NodeOptions &options = rclcpp::NodeOptions());
        ViconBridge(const ViconBridge &) = delete;
        ViconBridge &operator=(const ViconBridge &) = delete;
        ViconBridge(ViconBridge &&) = delete;
        ViconBridge &operator=(ViconBridge &&) = delete;
        ~ViconBridge() override;

    private:
        struct FrameIdMapping
        {
        public:
            std::string m_marker_frame_id;
            std::string m_base_link_frame_id;
        };

        std::unordered_map<std::string, FrameIdMapping> load_object_to_frame_id_mapping();
        void initialize_client();
        void poll_client();
        void process_frame();
        FrameIdMapping get_frame_id_mapping(const std::string &object_name) const;

        // ROS parameters
        std::string m_server_address;
        std::uint32_t m_buffer_size;
        std::string m_map_frame_id;

        std::unique_ptr<ViconDataStreamSDK::CPP::Client> m_client;
        std::thread m_client_polling_thread;
        std::atomic<bool> m_done;
        std::unordered_map<std::string, FrameIdMapping> m_object_to_frame_id_mapping;

        std::unique_ptr<tf2_ros::Buffer> m_transform_buffer;
        std::unique_ptr<tf2_ros::TransformListener> m_transform_listener;
        std::unique_ptr<tf2_ros::TransformBroadcaster> m_tf_broadcaster;
    };
}
