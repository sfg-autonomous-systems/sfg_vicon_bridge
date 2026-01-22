#include "sfg_vicon_bridge/vicon_bridge.hpp"

#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

#include "sfg_utils/ros_utils.hpp"

namespace sfg_vicon_bridge
{
    template <typename ResponseType>
    const ResponseType &throw_on_failure(const ResponseType &response, const std::string &error)
    {
        if (response.Result != ViconDataStreamSDK::CPP::Result::Success)
        {
            throw std::runtime_error(error);
        }
        return response;
    }

    ViconBridge::ViconBridge(const rclcpp::NodeOptions &options)
        : Node("vicon_bridge", rclcpp::NodeOptions(options).allow_undeclared_parameters(true).automatically_declare_parameters_from_overrides(true)),
          m_done(false),
          m_transform_buffer(std::make_unique<tf2_ros::Buffer>(get_clock())),
          m_transform_listener(std::make_unique<tf2_ros::TransformListener>(*m_transform_buffer)),
          m_tf_broadcaster(this)
    {
        // Declare and retrieve ROS parameters.
        m_hostname = sfg_utils::ros_utils::declare_parameter_if_not_declared(
            *this,
            "hostname",
            "127.0.0.1:801",
            rcl_interfaces::msg::ParameterDescriptor()
                .set__description("The hostname and port of the Vicon server."));

        m_buffer_size = sfg_utils::ros_utils::declare_parameter_if_not_declared(
            *this,
            "buffer_size",
            200,
            rcl_interfaces::msg::ParameterDescriptor()
                .set__description("Number of frames the Vicon client should buffer internally."));

        m_global_frame_id = sfg_utils::ros_utils::declare_parameter_if_not_declared(
            *this,
            "global_frame_id",
            "map",
            rcl_interfaces::msg::ParameterDescriptor()
                .set__description("The global frame ID to use for all published transforms."));

        m_object_to_frame_id_mapping = load_object_to_frame_id_mapping();
        m_client_polling_thread = std::thread(&ViconBridge::poll_client, this);

        RCLCPP_INFO(get_logger(), "Started Vicon bridge.");
    }

    ViconBridge::~ViconBridge()
    {
        m_done = true;

        if (m_client_polling_thread.joinable())
        {
            m_client_polling_thread.join();
        }
    }

    std::unordered_map<std::string, ViconBridge::FrameIdMapping> ViconBridge::load_object_to_frame_id_mapping()
    {
        const std::string &object_to_frame_id_mapping_prefix = "object_to_frame_id_mapping";

        RCLCPP_INFO(get_logger(), "Loading object to frame id mapping with mappings:");
        std::set<std::string> objects;

        for (const auto &parameter_name : list_parameters({object_to_frame_id_mapping_prefix}, 3).names)
        {
            // Parameter name is the full name including the prefix. We need to extract the object name by
            // removing the object to frame id mapping prefix (and the dot) and...
            auto object = parameter_name.substr(object_to_frame_id_mapping_prefix.size() + 1);
            // ...removing any sub-parameter suffix (e.g. ".tracked_frame_id" or ".base_frame_id").
            object = object.substr(0, object.find('.'));
            objects.insert(object);
        }

        std::unordered_map<std::string, FrameIdMapping> object_to_frame_id_mapping;

        for (const auto &object : objects)
        {
            auto parameter_prefix = object_to_frame_id_mapping_prefix + "." + object;
            auto tracked_frame_id = get_parameter_or(parameter_prefix + ".tracked_frame_id", object);
            auto base_frame_id = get_parameter_or(parameter_prefix + ".base_frame_id", tracked_frame_id);
            object_to_frame_id_mapping[object] = {tracked_frame_id, base_frame_id};

            RCLCPP_INFO(get_logger(), "  %s -> { tracked frame id: '%s', base frame id: '%s' }", object.c_str(), tracked_frame_id.c_str(), base_frame_id.c_str());
        }
        return object_to_frame_id_mapping;
    }

    void ViconBridge::initialize_client()
    {
        while (!m_client.IsConnected().Connected)
        {
            RCLCPP_INFO(get_logger(), "Connecting to Vicon server at %s...", m_hostname.c_str());
            m_client.Connect(m_hostname);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        RCLCPP_INFO(get_logger(), "Connected to Vicon server at %s.", m_hostname.c_str());

        m_client.SetBufferSize(m_buffer_size);
        throw_on_failure(m_client.EnableSegmentData(), "Failed to enable segment data from Vicon server.");
        throw_on_failure(m_client.SetStreamMode(ViconDataStreamSDK::CPP::StreamMode::ServerPush), "Failed to set Vicon stream mode.");
    }

    void ViconBridge::poll_client()
    {
        try
        {
            initialize_client();
        }
        catch (const std::exception &exception)
        {
            RCLCPP_ERROR(get_logger(), "Failed to initialize Vicon client: %s", exception.what());
            return;
        }

        while (!m_done && rclcpp::ok())
        {
            if (m_client.GetFrame().Result == ViconDataStreamSDK::CPP::Result::NoFrame)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            try
            {
                process_frame();
            }
            catch (const std::exception &exception)
            {
                RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 1000, "Frame processing error: %s", exception.what());
            }
        }

        try
        {
            throw_on_failure(m_client.Disconnect(), "Failed to disconnect from Vicon server.");
        }
        catch (const std::exception &exception)
        {
            RCLCPP_WARN(get_logger(), "Failed to disconnect from Vicon server: %s", exception.what());
        }
    }

    void ViconBridge::process_frame()
    {
        auto stamp = now() - rclcpp::Duration::from_seconds(m_client.GetLatencyTotal().Total);
        auto subject_count = throw_on_failure(m_client.GetSubjectCount(), "Failed to get subject count from Vicon server.").SubjectCount;

        for (size_t subject_index = 0; subject_index < subject_count; subject_index++)
        {
            // Vicon's DataStream SDK supports subjects having multiple segments. But since we are exclusively using Vicon Tracker as the server which
            // does not seem to support multi-segment subjects (i.e. tracked objects with multiple segments), we only retrieve the root segment for each subject.
            auto subject_name = throw_on_failure(m_client.GetSubjectName(subject_index), "Failed to get subject name from Vicon server.").SubjectName;
            auto segment_name = throw_on_failure(m_client.GetSubjectRootSegmentName(subject_name), "Failed to get subject root segment name from Vicon server.").SegmentName;
            auto segment_translation = throw_on_failure(m_client.GetSegmentGlobalTranslation(subject_name, segment_name), "Failed to get root segment global translation from Vicon server.");
            auto segment_rotation = throw_on_failure(m_client.GetSegmentGlobalRotationQuaternion(subject_name, segment_name), "Failed to get root segment global rotation from Vicon server.");

            if (segment_translation.Occluded || segment_rotation.Occluded)
            {
                continue;
            }

            tf2::Transform global_to_tracked_frame_transform = tf2::Transform(
                tf2::Quaternion(segment_rotation.Rotation[0], segment_rotation.Rotation[1], segment_rotation.Rotation[2], segment_rotation.Rotation[3]),
                tf2::Vector3(segment_translation.Translation[0] / 1000.0, segment_translation.Translation[1] / 1000.0, segment_translation.Translation[2] / 1000.0));

            auto frame_id_mapping = get_frame_id_mapping(subject_name);
            tf2::Stamped<tf2::Transform> tracked_to_base_frame_transform;

            try
            {
                tf2::fromMsg(m_transform_buffer->lookupTransform(frame_id_mapping.tracked_frame_id, frame_id_mapping.base_frame_id, stamp), tracked_to_base_frame_transform);
            }
            catch (const tf2::TransformException &exception)
            {
                RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 1000, "Failed to look up transform from tracked frame '%s' to base frame '%s': %s", frame_id_mapping.tracked_frame_id.c_str(), frame_id_mapping.base_frame_id.c_str(), exception.what());
                continue;
            }

            geometry_msgs::msg::TransformStamped msg;
            msg.header.stamp = stamp;
            msg.header.frame_id = m_global_frame_id;
            msg.child_frame_id = frame_id_mapping.base_frame_id;
            msg.transform = tf2::toMsg(global_to_tracked_frame_transform * tracked_to_base_frame_transform);
            m_tf_broadcaster.sendTransform(msg);
        }
    }

    ViconBridge::FrameIdMapping ViconBridge::get_frame_id_mapping(const std::string &object_name) const
    {
        if (auto iterator = m_object_to_frame_id_mapping.find(object_name); iterator != m_object_to_frame_id_mapping.end())
        {
            return iterator->second;
        }
        return {object_name, object_name};
    }
}