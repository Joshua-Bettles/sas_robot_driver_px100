from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():

    return LaunchDescription([
        Node(
            output='screen',
            emulate_tty=True,
            package='sas_robot_driver_px100',
            executable='sas_robot_driver_PX100_node',
            name='PX100_1',
            parameters=[{
                "usb": "/dev/ttyUSB0",
                "joint_limits_min": [-360.0, -360.0, -360.0, -360.0, -360.0],
                "joint_limits_max": [360.0, 360.0, 360.0, 360.0, 360.0],
                "protocol":2.0,
                "baud_rate":1000000,
                "ids": [1,2,3,4,5],
                "thread_sampling_time_sec": 0.002 # Robot thread is at 500 Hz
            }]
        ),

    ])