from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from tracetools_launch.action import Trace


def generate_launch_description():
    namespace = LaunchConfiguration("namespace")
    node_name = LaunchConfiguration("node_name")
    ros_tracing = LaunchConfiguration("ros_tracing")
    params_file = LaunchConfiguration("params_file")

    return LaunchDescription([
        DeclareLaunchArgument("namespace", default_value=""),
        DeclareLaunchArgument("node_name", default_value="mqtt_client"),
        DeclareLaunchArgument(
            "params_file",
            default_value=PathJoinSubstitution([
                FindPackageShare("mqtt_client"), "config", "params.yaml"
            ]),
        ),
        DeclareLaunchArgument("ros_tracing", default_value="false", description="enable tracing"),
        Node(
            package="mqtt_client",
            executable="mqtt_client",
            name=node_name,
            namespace=namespace,
            output="screen",
            parameters=[params_file],
        ),
        Trace(
            session_name="trace",
            dual_session=True,
            condition=IfCondition(ros_tracing),
        ),
    ])
