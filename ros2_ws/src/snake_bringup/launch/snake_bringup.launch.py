import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import Command, LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


#引入所有必须的路径，加载config
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


#核心：运行时执行的shell命令
    robot_description = ParameterValue(
        Command([
            'xacro ',
            xacro_file,
            ' use_mock_hardware:=',
            use_mock_hardware,
            ' serial_port:=',
            serial_port
        ]),
        value_type=str
    )
#robot_state_publisher 节点
    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        output='screen',
        parameters=[{
            'robot_description': robot_description
        }]
    )
# ros2_control_node 节点
    ros2_control_node = Node(
        package='controller_manager',
        executable='ros2_control_node',
        output='screen',
        parameters=[
            {
                'robot_description': robot_description
            },
            controllers_file
        ]
    )
#两个 spawner 节点（加载控制器）
    joint_state_broadcaster_spawner = Node(
        package='controller_manager',
        executable='spawner',
        arguments=[
            'joint_state_broadcaster',
            '--controller-manager',
            '/controller_manager'
        ],
        output='screen'
    )
    trajectory_controller_spawner = Node(
        package='controller_manager',
        executable='spawner',
        arguments=[
            'snake_joint_controller',
            '--controller-manager',
            '/controller_manager'
        ],
        output='screen'
    )
#两个启动时的正式参数与默认值

#四个节点按顺序启动
    return LaunchDescription([
        DeclareLaunchArgument(
            'use_mock_hardware',
            default_value='true'
        ),
        DeclareLaunchArgument(
            'serial_port',
            default_value='/dev/ttyUSB0'
        ),

        robot_state_publisher,
        ros2_control_node,
        joint_state_broadcaster_spawner,
        trajectory_controller_spawner
    ])