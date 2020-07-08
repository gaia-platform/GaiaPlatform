from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='gaia_incubator',
            executable='incubator_manager',
            output='screen',
            name='incubator_manager'
        ),
        Node(
            package='gaia_incubator',
            executable='gaia_logic',
            output='screen',
            name='gaia_logic'
        )
    ])
