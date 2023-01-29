#!/usr/bin/env python3
    
import os
from ament_index_python import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():

    config = os.path.join(
        get_package_share_directory("mqtt_client"),
        "config",
        "params.ros2.yaml"
    )

    return LaunchDescription([
        Node(
            package="mqtt_client",
            executable="mqtt_client",
            name="mqtt_client",
            output="screen",
            emulate_tty=True,
            parameters=[config]
        )
    ])
