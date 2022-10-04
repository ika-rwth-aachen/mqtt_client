#!/usr/bin/env python3
    
import os
from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
    

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='mqtt_client',
            executable='mqtt_client_exe',
            name='mqtt_client',
            parameters=[os.path.join(
                get_package_share_directory('mqtt_client'),
                'launch', 'params.yaml')],
            output='screen'),
    ])