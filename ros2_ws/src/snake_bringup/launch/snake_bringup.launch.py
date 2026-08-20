import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import Command, LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue

def generate_launch_description():
    description_dir = get_package_share_directory(
        'snake_description'
    )

    bringup_dir = get_package_share_directory(
        'snake_bringup'
    )
    xacro_file = os.path.join(
        description_dir,
        'urdf',
        'snake.urdf.xacro'
    )
    controllers_file = os.path.join(
        bringup_dir,
        'config',
        'controllers.yaml'
    )

    use_mock_hardware = LaunchConfiguration(
        'use_mock_hardware'
    )

    serial_port = LaunchConfiguration(
        'serial_port'
    )