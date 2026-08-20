import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.substitutions import Command
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue

#这个文件只会启动蛇形机器人的模型，不会启动ros2 controller
def generate_launch_description():
    package_dir = get_package_share_directory(
        'snake_description'
    )

    xacro_file = os.path.join(
        package_dir,
        'urdf',
        'snake.urdf.xacro'
    )
    rviz_config = os.path.join(
        package_dir,
        'rviz',
        'snake.rviz'
    )
    robot_description = ParameterValue(
        Command([
            'xacro ',
            xacro_file,
            ' use_mock_hardware:=true'
        ]),
        value_type=str
  )
    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        output='screen',
        parameters=[{
            'robot_description': robot_description
        }]
    )

    joint_state_publisher_gui = Node(
        package='joint_state_publisher_gui',
        executable='joint_state_publisher_gui',
        output='screen'
    )

    rviz = Node(
        package='rviz2',
        executable='rviz2',
        output='screen',
        arguments=['-d', rviz_config]
    )

    return LaunchDescription([
        robot_state_publisher,
        joint_state_publisher_gui,
        rviz
    ])