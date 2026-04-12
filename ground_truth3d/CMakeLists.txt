cmake_minimum_required(VERSION 3.8)
project(ground_truth3d)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(message_filters REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(PCL REQUIRED COMPONENTS common io)
find_package(Eigen3 REQUIRED)

add_executable(invert_lidar src/invert_lidar.cpp)
ament_target_dependencies(invert_lidar rclcpp sensor_msgs)

install(TARGETS
  invert_lidar
  DESTINATION lib/${PROJECT_NAME}
)

add_executable(synced_saver src/save_synced_data.cpp)
target_include_directories(synced_saver PUBLIC
  ${PCL_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIR}
)

target_link_libraries(synced_saver 
  ${PCL_LIBRARIES}
)

ament_target_dependencies(synced_saver 
  rclcpp 
  sensor_msgs 
  geometry_msgs
  message_filters
  cv_bridge
  OpenCV
  pcl_conversions
)

install(TARGETS
  synced_saver
  DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY launch config rviz
  DESTINATION share/${PROJECT_NAME}
)
ament_package()
