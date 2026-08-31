from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import (
    Command,
    FindExecutable,
    LaunchConfiguration,
    PathJoinSubstitution,
)
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    use_rviz = LaunchConfiguration("use_rviz")
    use_sim_time = LaunchConfiguration("use_sim_time")

    # snake_description安装后的共享目录
    description_share = FindPackageShare("snake_description")

    # snake_bringup安装后的共享目录
    bringup_share = FindPackageShare("snake_bringup")

    # 主机器人Xacro文件
    xacro_file = PathJoinSubstitution(
        [
            description_share,
            "urdf",
            "snake_robot.urdf.xacro",
        ]
    )

    # ros2_control控制器参数
    controllers_file = PathJoinSubstitution(
        [
            bringup_share,
            "config",
            "controllers.yaml",
        ]
    )

    # 执行：
    # xacro snake_robot.urdf.xacro
    #
    # 输出的完整URDF XML会传给robot_state_publisher。
    robot_description = {
        "robot_description": Command(
            [
                FindExecutable(name="xacro"),
                " ",
                xacro_file,
            ]
        )
    }

    # 发布机器人模型、TF以及/robot_description
    robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        name="robot_state_publisher",
        output="screen",
        parameters=[
            robot_description,
            {
                "use_sim_time": use_sim_time,
            },
        ],
    )

    # ros2_control核心进程
    #
    # Jazzy中Controller Manager从~/robot_description订阅模型，
    # 这里将它重映射到robot_state_publisher发布的/robot_description。
    controller_manager = Node(
        package="controller_manager",
        executable="ros2_control_node",
        name="controller_manager",
        output="screen",
        parameters=[
            controllers_file,
            {
                "use_sim_time": use_sim_time,
            },
        ],
        remappings=[
            ("~/robot_description", "/robot_description"),
        ],
    )

    # 发布/joint_states
    joint_state_broadcaster_spawner = Node(
        package="controller_manager",
        executable="spawner",
        name="joint_state_broadcaster_spawner",
        output="screen",
        arguments=[
            "joint_state_broadcaster",
            "--controller-manager",
            "/controller_manager",
        ],
    )

    # 接收8个关节的位置命令
    position_controller_spawner = Node(
        package="controller_manager",
        executable="spawner",
        name="snake_position_controller_spawner",
        output="screen",
        arguments=[
            "snake_position_controller",
            "--controller-manager",
            "/controller_manager",
        ],
    )

    # 可选启动RViz
    rviz = Node(
        package="rviz2",
        executable="rviz2",
        name="rviz2",
        output="screen",
        condition=IfCondition(use_rviz),
        parameters=[
            {
                "use_sim_time": use_sim_time,
            }
        ],
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "use_rviz",
                default_value="true",
                description="Whether to start RViz",
            ),
            DeclareLaunchArgument(
                "use_sim_time",
                default_value="false",
                description="Use simulation clock",
            ),
            robot_state_publisher,
            controller_manager,
            joint_state_broadcaster_spawner,
            position_controller_spawner,
            rviz,
        ]
    )