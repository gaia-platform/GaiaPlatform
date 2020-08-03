import launch
from launch_ros.actions import ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
import launch.actions
import launch.substitutions
import launch_ros.actions

def generate_launch_description():
    container = ComposableNodeContainer(
        name='gaia_container',
        package='rclcpp_components',
        namespace='',
        executable='component_container',
        composable_node_descriptions=[
            ComposableNode(
                package='gaia_pkg',
                plugin='gaia_node',
                name='gaia_node'
            ),
        ],
        output='screen'
    )

    return launch.LaunchDescription([container])
