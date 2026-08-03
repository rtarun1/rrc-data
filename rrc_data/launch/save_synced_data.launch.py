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

    save_synced_data_node = Node(
        package='rrc_data',
        executable='save_synced_data',
        name='save_synced_data',
        parameters=[config],
        output='screen'
    )

    rosbag_with_delay = TimerAction(
                period=5.0,
                actions=[rosbag_play_launch],
                # condition=IfCondition(record),
            )

    return LaunchDescription([
        save_synced_data_node,
        rosbag_with_delay
    ])