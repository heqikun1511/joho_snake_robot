import math

from geometry_msgs.msg import Twist
import rclpy
from rclpy.node import Node
from std_msgs.msg import Float64MultiArray


class GaitController(Node):
    """Convert velocity commands into eight snake joint position targets."""

    MODULE_COUNT = 4

    def __init__(self):
        super().__init__('snake_gait_controller')

        self.declare_parameter('publish_rate', 50.0)
        self.declare_parameter('enabled', True)
        self.declare_parameter('yaw_amplitude', 0.25)
        self.declare_parameter('pitch_amplitude', 0.0)
        self.declare_parameter('yaw_offset', 0.0)
        self.declare_parameter('pitch_offset', 0.0)
        self.declare_parameter('phase_difference', 0.8)
        self.declare_parameter('pitch_phase_offset', math.pi / 2.0)
        self.declare_parameter('cmd_vel_timeout', 0.5)
        self.declare_parameter('frequency_gain', 1.0)
        self.declare_parameter('steering_gain', 0.25)
        self.declare_parameter('maximum_frequency', 0.5)
        self.declare_parameter('maximum_steering', 0.25)
        self.declare_parameter('joint_limit', 1.0472)

        publish_rate = float(self.get_parameter('publish_rate').value)
        if publish_rate <= 0.0:
            raise ValueError('publish_rate must be greater than zero')

        self.linear_velocity = 0.0
        self.angular_velocity = 0.0
        self.phase = 0.0
        self.last_cmd_vel_time = self.get_clock().now()
        self.last_update_time = self.last_cmd_vel_time

        self.command_publisher = self.create_publisher(
            Float64MultiArray,
            '/snake_position_controller/commands',
            10,
        )
        self.cmd_vel_subscription = self.create_subscription(
            Twist,
            '/cmd_vel',
            self.cmd_vel_callback,
            10,
        )
        self.timer = self.create_timer(
            1.0 / publish_rate,
            self.publish_joint_commands,
        )

        self.get_logger().info(
            f'Gait controller started at {publish_rate:.1f} Hz'
        )

    def cmd_vel_callback(self, message):
        self.linear_velocity = float(message.linear.x)
        self.angular_velocity = float(message.angular.z)
        self.last_cmd_vel_time = self.get_clock().now()

    @staticmethod
    def clamp(value, limit):
        return max(-limit, min(limit, value))

    def publish_joint_commands(self):
        now = self.get_clock().now()
        dt = max(0.0, (now - self.last_update_time).nanoseconds * 1e-9)
        self.last_update_time = now

        timeout = max(
            0.0,
            float(self.get_parameter('cmd_vel_timeout').value),
        )
        command_age = (now - self.last_cmd_vel_time).nanoseconds * 1e-9
        command_is_fresh = command_age <= timeout

        linear_velocity = self.linear_velocity if command_is_fresh else 0.0
        angular_velocity = self.angular_velocity if command_is_fresh else 0.0

        frequency_gain = float(self.get_parameter('frequency_gain').value)
        steering_gain = float(self.get_parameter('steering_gain').value)
        maximum_frequency = abs(
            float(self.get_parameter('maximum_frequency').value)
        )
        maximum_steering = abs(
            float(self.get_parameter('maximum_steering').value)
        )

        frequency = self.clamp(
            frequency_gain * linear_velocity,
            maximum_frequency,
        )
        steering_offset = self.clamp(
            steering_gain * angular_velocity,
            maximum_steering,
        )

        # Integrating phase prevents a discontinuity when frequency changes.
        self.phase = math.fmod(
            self.phase + 2.0 * math.pi * frequency * dt,
            2.0 * math.pi,
        )

        enabled = bool(self.get_parameter('enabled').value)
        moving = (
            abs(linear_velocity) > 1e-3
            or abs(angular_velocity) > 1e-3
        )
        yaw_amplitude = float(self.get_parameter('yaw_amplitude').value)
        pitch_amplitude = float(
            self.get_parameter('pitch_amplitude').value
        )
        yaw_offset = float(self.get_parameter('yaw_offset').value)
        pitch_offset = float(self.get_parameter('pitch_offset').value)
        phase_difference = float(
            self.get_parameter('phase_difference').value
        )
        pitch_phase_offset = float(
            self.get_parameter('pitch_phase_offset').value
        )
        joint_limit = abs(float(self.get_parameter('joint_limit').value))

        commands = []
        for module_index in range(self.MODULE_COUNT):
            module_phase = self.phase + module_index * phase_difference
            if enabled and moving:
                yaw = (
                    yaw_amplitude * math.cos(module_phase)
                    + yaw_offset
                    + steering_offset
                )
                pitch = (
                    pitch_amplitude
                    * math.cos(module_phase + pitch_phase_offset)
                    + pitch_offset
                )
            else:
                yaw = 0.0
                pitch = 0.0

            commands.extend(
                [
                    self.clamp(yaw, joint_limit),
                    self.clamp(pitch, joint_limit),
                ]
            )

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


if __name__ == '__main__':
    main()
