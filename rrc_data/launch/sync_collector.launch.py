import os
import datetime
from launch import LaunchDescription
from launch.actions import (
    IncludeLaunchDescription,
    RegisterEventHandler,
    Shutdown,
    DeclareLaunchArgument,
)
from launch.actions import TimerAction
from ament_index_python.packages import get_package_share_directory
from launch.event_handlers import OnProcessExit, OnProcessStart
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import (
    Command,
    FindExecutable,
    PathJoinSubstitution,
    LaunchConfiguration,
    NotSubstitution,
)
from launch.conditions import IfCondition, UnlessCondition
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare
from launch_xml.launch_description_sources import XMLLaunchDescriptionSource


def generate_launch_description():
    dlio_share = get_package_share_directory('direct_lidar_inertial_odometry')
    launch_arguments = []
    launch_arguments.append(
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='False',
            description='Launch in simulation mode',
        )
    )
    launch_arguments.append(
        DeclareLaunchArgument(
            'use_camera1',
            default_value='True',
            description='Launch Camera1',
        )
    )
    launch_arguments.append(
        DeclareLaunchArgument(
            'record',
            default_value='True',
            description='Record in rosbag'
        )
    )

    use_sim_time = LaunchConfiguration("use_sim_time")
    use_camera1 = LaunchConfiguration("use_camera1")
    record = LaunchConfiguration("record")


    get_current_timestamp = datetime.datetime.now().strftime('%Y-%m-%d_%H-%M-%S')
    log_full_path = os.path.join('/home/container_user/rrc_data/src/records/', get_current_timestamp)
    rosbag_full_path = os.path.join(log_full_path, 'rosbag')

    package_path = get_package_share_directory('rrc_data')


    camera1_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution(
                [FindPackageShare("realsense2_camera"), "launch", "rs_launch.py"]
            ),
        ),
        launch_arguments={
            # 'serial_no': "'213522251148'",
            'camera_name': "camera1",
            'camera_namespace': "camera1",
            'align_depth.enable': 'true',
            'device_type' : 'd455',
            'enable_color' : 'true',
            'enable_depth' : 'true',
            'pointcloud.enable' : 'true',
            'use_sim_time': use_sim_time,
        }.items(),
        condition=IfCondition(use_camera1),
    )

    livox_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution(
                [FindPackageShare("livox_ros_driver2"), "launch", "husky_mid360_launch.py"]
            ),
        ),
    )

    rosbag_recorder_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            [package_path, '/launch/rosbag_recorder.launch.py']
        ),
        launch_arguments={
            'use_sim_time': use_sim_time,
            'rosbag_storage_dir': rosbag_full_path,
        }.items(),
        condition=IfCondition(record),
    )

    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='log',
        arguments=['-d', get_package_share_directory('rrc_data') + '/rviz/data_collection.rviz'],
    )

    dlio_launch = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(dlio_share, 'launch', 'dlio.launch.py')
            ),
            launch_arguments={'rviz': 'true'}.items()
        )

    rosbag_with_delay = TimerAction(
        period=5.0,
        actions=[rosbag_recorder_launch],
        condition=IfCondition(record),
    )

    nodes = [
        camera1_launch,
        livox_launch,
        rviz_node,
        # dlio_launch,
        rosbag_with_delay,
    ]

    return LaunchDescription(launch_arguments + nodes)