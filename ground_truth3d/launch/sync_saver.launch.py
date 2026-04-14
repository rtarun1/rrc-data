import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node

def generate_launch_description():
    
    ground_truth3d_share = get_package_share_directory('ground_truth3d')
    dlio_share = get_package_share_directory('direct_lidar_inertial_odometry')

    rosbag_play_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(ground_truth3d_share, 'launch', 'rosbag_play.launch.py')
        )
    )

    invert_lidar_node = Node(
        package='ground_truth3d',
        executable='invert_lidar',
        name='invert_lidar_node',
        output='screen'
    )

    dlio_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(dlio_share, 'launch', 'dlio.launch.py')
        ),
        launch_arguments={'rviz': 'true'}.items()
    )

    synced_saver_node = Node(
        package='ground_truth3d',
        executable='synced_saver',
        name='synced_saver_node',
        output='screen'
    )

    return LaunchDescription([
        rosbag_play_launch,
        dlio_launch,
        synced_saver_node
    ])