import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import TimerAction
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node

def generate_launch_description():
    
    ground_truth3d_share = get_package_share_directory('ground_truth3d')
    dlio_share = get_package_share_directory('direct_lidar_inertial_odometry')

    config = os.path.join(
            get_package_share_directory('ground_truth3d'),
            'config',
            'sync_saver.yaml'
        )

    rosbag_play_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(ground_truth3d_share, 'launch', 'rosbag_play.launch.py')
        ),
        launch_arguments={
            'bag_dir': "/home/container_user/rrc-data/src/data/2026-05-09_15-41-01/rosbag"
        }.items(),
    )

    synced_saver_node = Node(
            package='ground_truth3d',
            executable='synced_saver',
            name='synced_saver',
            parameters=[config],
            output='screen'
        )

    dlio_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(dlio_share, 'launch', 'dlio.launch.py')
        ),
        launch_arguments={'rviz': 'true'}.items()
    )

    rosbag_with_delay = TimerAction(
            period=5.0,
            actions=[rosbag_play_launch],
            # condition=IfCondition(record),
        )

    return LaunchDescription([
        dlio_launch,
        synced_saver_node,
        rosbag_with_delay
    ])