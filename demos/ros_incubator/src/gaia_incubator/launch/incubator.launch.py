import launch
from launch_ros.actions import ComposableNodeContainer
from launch_ros.descriptions import ComposableNode

def generate_launch_description():
    container = ComposableNodeContainer(
            name='demo_container',
            namespace='',
            package='gaia_incubator',
            executable='incubator_container',
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
            output='screen',
    )

    return launch.LaunchDescription([container])
