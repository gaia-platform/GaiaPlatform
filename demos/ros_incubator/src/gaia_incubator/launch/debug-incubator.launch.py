import launch
from launch_ros.actions import ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
import launch.actions
import launch.substitutions
import launch_ros.actions

def generate_launch_description():
    container = ComposableNodeContainer(
        name='incubator_container',
        package='rclcpp_components',
        namespace='',
        executable='component_container',
        composable_node_descriptions=[
            ComposableNode(
                package='gaia_incubator',
                plugin='incubator_manager',
                name='incubator_manager'
            ),
            ComposableNode(
                package='gaia_incubator',
                plugin='gaia_logic',
                name='gaia_logic'
            )
        ],
        output='screen',
        prefix=['gdb -ex=r']
    )

    return launch.LaunchDescription([container])
