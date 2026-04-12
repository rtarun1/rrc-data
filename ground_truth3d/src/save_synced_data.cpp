#include <filesystem>
#include <fstream>
#include <iomanip>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>

#include <message_filters/subscriber.h>
#include <message_filters/sync_policies/approximate_time.h>
#include <message_filters/synchronizer.h>

#include <pcl_conversions/pcl_conversions.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_cloud.h>

#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>

#include <Eigen/Dense>


class SyncedSaver : public rclcpp::Node {
public:
    SyncedSaver() : Node("synced_saver"){

        pcd_dir_ = "src/data/pcd/";
        img_dir_ = "src/data/image/";
        pose_dir_ = "src/data/pose/";
        
        std::filesystem::create_directories(pcd_dir_);
        std::filesystem::create_directories(img_dir_);
        std::filesystem::create_directories(pose_dir_);

        rmw_qos_profile_t qos_profile = rmw_qos_profile_sensor_data;

        lidar_sub_.subscribe(this, "/livox/lidar_inverted", qos_profile);
        camera_sub_.subscribe(this, "/camera1/camera1/color/image_raw", qos_profile);
        pose_sub_.subscribe(this, "/dlio/odom_node/pose", qos_profile);

        sync_.reset(new Sync(SyncPolicy(10), lidar_sub_, camera_sub_, pose_sub_));
        sync_->registerCallback(std::bind(&SyncedSaver::sync_callback, this, 
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }

private:
    using SyncPolicy = message_filters::sync_policies::ApproximateTime<
        sensor_msgs::msg::PointCloud2,
        sensor_msgs::msg::Image,
        geometry_msgs::msg::PoseStamped>;
    using Sync = message_filters::Synchronizer<SyncPolicy>;

    void sync_callback(
        const sensor_msgs::msg::PointCloud2::ConstSharedPtr& cloud_msg,
        const sensor_msgs::msg::Image::ConstSharedPtr& image_msg,
        const geometry_msgs::msg::PoseStamped::ConstSharedPtr& pose_msg) {

            double timestamp = cloud_msg->header.stamp.sec + (cloud_msg->header.stamp.nanosec * 1e-9);

            std::ostringstream time_oss;
            time_oss << std::fixed << std::setprecision(9) << timestamp;
            std::string time_str = time_oss.str();

            save_pcd(cloud_msg, pcd_dir_ + time_str + ".pcd");

            save_image(image_msg, img_dir_ + time_str + ".png");

            save_pose(pose_msg, pose_dir_ + time_str + ".txt");
        }

    void save_pcd(const sensor_msgs::msg::PointCloud2::ConstSharedPtr& msg, const std::string& filename) {
        pcl::PointCloud<pcl::PointXYZ> cloud;
        pcl::fromROSMsg(*msg, cloud);
        pcl::io::savePCDFileBinary(filename, cloud); 
    }

    void save_image(const sensor_msgs::msg::Image::ConstSharedPtr& msg, const std::string& filename) {
        cv_bridge::CvImageConstPtr cv_ptr = cv_bridge::toCvShare(msg, "bgr8");
        cv::imwrite(filename, cv_ptr->image);
    }

    void save_pose(const geometry_msgs::msg::PoseStamped::ConstSharedPtr& msg, const std::string& filename) {
        std::ofstream file(filename);

        // eigen quaternion is (w, x, y, z)
        Eigen::Quaterniond q(
            msg->pose.orientation.w,
            msg->pose.orientation.x,
            msg->pose.orientation.y,
            msg->pose.orientation.z
        );
        Eigen::Matrix3d R = q.toRotationMatrix();
        Eigen::Vector3d t(
            msg->pose.position.x,
            msg->pose.position.y,
            msg->pose.position.z
        );

        file << std::scientific << std::setprecision(18);

        for (int i = 0; i < 3; ++i) {
            file << R(i, 0) << " " << R(i, 1) << " " << R(i, 2) << " " << t(i) << "\n";
        }
        file << "0.000000000000000000e+00 0.000000000000000000e+00 0.000000000000000000e+00 1.000000000000000000e+00\n";
        
        file.close();
    }

    std::string pcd_dir_, img_dir_, pose_dir_;

    message_filters::Subscriber<sensor_msgs::msg::PointCloud2> lidar_sub_;
    message_filters::Subscriber<sensor_msgs::msg::Image> camera_sub_;
    message_filters::Subscriber<geometry_msgs::msg::PoseStamped> pose_sub_;
    std::shared_ptr<Sync> sync_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<SyncedSaver>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}