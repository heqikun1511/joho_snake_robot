import math

import rclpy
from rclpy.node import Node
from std_msgs.msg import Float64MultiArray
from geometry_msgs.msg import Twist


#
class GaitController(Node):
    MODULE_COUNT = 4  #蛇身体四个模块

    def __init__(self):
        super().__init__("snake_gait_controller")

        self.declare_parameter("publish_rate", 50.0)
        self.declare_parameter("enabled", True)

        # 单位：弧度
        self.declare_parameter("yaw_amplitude", 0.25)   #俯仰幅度
        self.declare_parameter("pitch_amplitude", 0.0)
        self.declare_parameter("yaw_offset", 0.0)       #偏移量
        self.declare_parameter("pitch_offset", 0.0)
        
        # 单位：Hz、rad
        self.declare_parameter("frequency", 0.25)
        self.declare_parameter("phase_difference", 0.8)   # 模块间相位差 (rad)   第二节比第一节晚多少  大于0前进
        self.declare_parameter(
            "pitch_phase_offset",
            math.pi / 2.0,
        )

        self.declare_parameter("cmd_vel_timeout", 0.5)
        self.declare_parameter("frequency_gain", 1.0)
        self.declare_parameter("steering_gain", 0.25)
        self.declare_parameter("maximum_frequency", 0.5)
        self.declare_parameter("maximum_steering", 0.25)    

        # 与 URDF 中 ±60° 限位一致
        self.declare_parameter("joint_limit", 1.0472)

        rate = float(self.get_parameter("publish_rate").value)
        if rate <= 0.0:
            raise ValueError("publish_rate must be greater than zero")
        self.command_publisher = self.create_publisher(
            Float64MultiArray,
            "/snake_position_controller/commands",
            10,
        )

        self.start_time = self.get_clock().now()
        self.timer = self.create_timer(
            1.0 / rate,
            self.publish_joint_commands,
        )

        self.get_logger().info(
            f"Gait controller started at {rate:.1f} Hz"
        )


def cmd_vel_callback(self, message):
    self.linear_velocity = float(message.linear.x)
    self.angular_velocity = float(message.angular.z)
    self.last_cmd_vel_time = self.get_clock().now()



    def publish_joint_commands(self):
        enabled = bool(self.get_parameter("enabled").value)

        yaw_amplitude = float(
            self.get_parameter("yaw_amplitude").value
        )
        pitch_amplitude = float(
            self.get_parameter("pitch_amplitude").value
        )
        yaw_offset = float(
            self.get_parameter("yaw_offset").value
        )
        pitch_offset = float(
            self.get_parameter("pitch_offset").value
        )
        frequency = linear_velocity * frequency_gain
        frequency = max(-maximum_frequency,min(maximum_frequency, frequency)
                            
                ,
        )
        phase_difference = float(
            self.get_parameter("phase_difference").value
        )
        pitch_phase_offset = float(
            self.get_parameter("pitch_phase_offset").value
        )
        joint_limit = abs(
            float(self.get_parameter("joint_limit").value)
        )

        elapsed = (
            self.get_clock().now() - self.start_time
        ).nanoseconds * 1e-9   #运行时间

        omega = 2.0 * math.pi * frequency    #角频率
        commands = []

        for module_index in range(self.MODULE_COUNT):
            phase = (
                omega * elapsed
                + module_index * phase_difference
            )

            if enabled:
                yaw = (
                    yaw_amplitude * math.cos(phase)
                    + yaw_offset
                )
                pitch = (
                    pitch_amplitude
                    * math.cos(phase + pitch_phase_offset)
                    + pitch_offset
                )
            else:
                yaw = 0.0
                pitch = 0.0  #回到中位，安全开关


            yaw = max(-joint_limit, min(joint_limit, yaw))
            pitch = max(-joint_limit, min(joint_limit, pitch))


            # 必须匹配 controllers.yaml 中的关节顺序
            commands.extend([yaw, pitch])

        message = Float64MultiArray()
        message.data = commands
        self.command_publisher.publish(message)





def main(args=None):
    rclpy.init(args=args)
    node = GaitController()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
