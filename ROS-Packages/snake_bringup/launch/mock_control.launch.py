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

    xacro_file = PathJoinSubstitution(
        [
            FindPackageShare("snake_description"),
            "urdf",
            "snake_robot.urdf.xacro",
        ]
    )
    controllers_file = PathJoinSubstitution(
        [
            FindPackageShare("snake_bringup"),
            "config",
            "controllers.yaml",
        ]
    )

    robot_description = {
        "robot_description": Command(
            [FindExecutable(name="xacro"), " ", xacro_file]
        )
    }

    robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        name="robot_state_publisher",
        output="screen",
        parameters=[robot_description, {"use_sim_time": use_sim_time}],
    )

    controller_manager = Node(
        package="controller_manager",
        executable="ros2_control_node",
        name="controller_manager",
        output="screen",
        parameters=[controllers_file, {"use_sim_time": use_sim_time}],
        remappings=[("~/robot_description", "/robot_description")],
    )

    joint_state_broadcaster = Node(
        package="controller_manager",
        executable="spawner",
        name="joint_state_broadcaster_spawner",
        output="screen",
        arguments=[
            "joint_state_broadcaster",
            "--controller-manager",
            "/controller_manager",
            "--controller-manager-timeout",
            "30",
        ],
    )

    position_controller = Node(
        package="controller_manager",
        executable="spawner",
        name="snake_position_controller_spawner",
        output="screen",
        arguments=[
            "snake_position_controller",
            "--controller-manager",
            "/controller_manager",
            "--controller-manager-timeout",
            "30",
        ],
    )

    rviz = Node(
        package="rviz2",
        executable="rviz2",
        name="rviz2",
        output="screen",
        condition=IfCondition(use_rviz),
        parameters=[{"use_sim_time": use_sim_time}],
    )
    gait_controller = Node(
        package="snake_gait_controller",
        executable="gait_controller",
        name="snake_gait_controller",
        output="screen",
        parameters=[
        {
            "use_sim_time": True,
            "publish_rate": 50.0,
            "yaw_amplitude": 0.15,
            "pitch_amplitude": 0.0,
            "phase_difference": 0.8,
        }
    ],
)


    return LaunchDescription(
        [
            DeclareLaunchArgument("use_rviz", default_value="true"),
            DeclareLaunchArgument("use_sim_time", default_value="false"),
            robot_state_publisher,
            controller_manager,
            joint_state_broadcaster,
            position_controller,
            gait_controller,
            rviz,
        ]
    )
