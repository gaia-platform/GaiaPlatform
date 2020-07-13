import launch
from launch_ros.actions import ComposableNodeContainer
from launch_ros.descriptions import ComposableNode

def generate_launch_description():
    container = ComposableNodeContainer(
        node_name='incubator_container',
        package='rclcpp_components',
        node_executable='incubator_container')
    loader = LoadComposableNodes(
        composable_node_descriptions=[
            ComposableNode(
                package='gaia_incubator',
                plugin='incubator_manager',
                name='incubator_manager'),
            ComposableNode(
                package='gaia_incubator',
                plugin='gaia_logic',
                name='gaia_logic')
        ],
        target_container=container, output='screen')

    return LaunchDescription([container, loader])
