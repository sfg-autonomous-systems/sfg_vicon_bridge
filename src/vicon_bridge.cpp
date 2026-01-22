#include "sfg_vicon_bridge/vicon_bridge.hpp"

namespace sfg_vicon_bridge
{
    ViconBridge::ViconBridge(const rclcpp::NodeOptions &options) : Node("vicon_bridge", options), m_broadcaster(this), m_static_broadcaster(this)
    {
        // Declare and retrieve ROS parameters.
        m_hostname = declare_parameter(
            "hostname",
            "localhost:801",
            rcl_interfaces::msg::ParameterDescriptor()
                .set__description("The hostname and port of the Vicon server."));

        m_buffer_size = declare_parameter(
            "buffer_size",
            200u,
            rcl_interfaces::msg::ParameterDescriptor()
                .set__description("Number of frames the Vicon client should buffer internally."));

        m_client.Connect(m_hostname);
        m_client.EnableSegmentData();
        m_client.SetStreamMode(ViconDataStreamSDK::CPP::StreamMode::ClientPull);
        m_client.SetBufferSize(m_buffer_size);

        RCLCPP_INFO(this->get_logger(), "Started Vicon bridge.");
    }
}