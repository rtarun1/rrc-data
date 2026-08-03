import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import TimerAction
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration

def generate_launch_description():

    ground_truth3d_share = get_package_share_directory('ground_truth3d')

    config = os.path.join(
        ground_truth3d_share,
        'config',
        'sync_saver.yaml'
    )

    rviz_arg = DeclareLaunchArgument(
        'rviz',
        default_value='false',
        description='Launch RViz'
    )

    rosbag_play_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                ground_truth3d_share,
                'launch',
                'rosbag_play.launch.py'
            )
        ),
        launch_arguments={
            'bag_dir': "/home/container_user/rrc_data/src/records/2026-08-03_04-15-14/rosbag",
            'rviz': LaunchConfiguration('rviz'),
        }.items(),
    )

    synced_saver_node = Node(
            package='ground_truth3d',
            executable='synced_saver',  
            name='synced_saver',
            parameters=[config],
            output='screen'
        )

    glim_node = Node(
        package='glim_ros',
        executable='glim_rosnode',
        name='glim_rosnode',
        parameters=[
        {
            'config_path': '/home/container_user/rrc_data/src/third_party/glim/config_cpu'
        }
    ],
        output='screen'
    )

    rosbag_with_delay = TimerAction(
            period=5.0,
            actions=[rosbag_play_launch],
        )

    return LaunchDescription([
        rviz_arg,
        glim_node,
        synced_saver_node,
        rosbag_with_delay
    ])