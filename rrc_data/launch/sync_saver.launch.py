import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node

def generate_launch_description():
    
    rrc_data_share = get_package_share_directory('rrc_data')

    config = os.path.join(
        get_package_share_directory('rrc_data'),
        'config',
        'sync_saver.yaml'
    )

    rosbag_play_launch = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(rrc_data_share, 'launch', 'rosbag_play.launch.py')
            ),
            launch_arguments={
                'bag_dir': "/home/container_user/rrc_data/src/records/2026-08-03_04-15-14/rosbag"
            }.items(),
        )

    synced_saver_node = Node(
        package='rrc_data',
        executable='synced_saver',
        name='synced_saver',
        parameters=[config],
        output='screen'
    )

    rosbag_with_delay = TimerAction(
                period=5.0,
                actions=[rosbag_play_launch],
                # condition=IfCondition(record),
            )

    return LaunchDescription([
        synced_saver_node,
        rosbag_with_delay
    ])