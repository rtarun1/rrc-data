#include <filesystem>
#include <fstream>
#include <iomanip>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <sensor_msgs/image_encodings.hpp> 
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
        calib_dir_ = "src/data/calibration/";
        depth_dir_ = "src/data/depth/"; 

        std::filesystem::create_directories(pcd_dir_);
        std::filesystem::create_directories(img_dir_);
        std::filesystem::create_directories(pose_dir_);
        std::filesystem::create_directories(calib_dir_);
        std::filesystem::create_directories(depth_dir_);

        rmw_qos_profile_t qos_profile = rmw_qos_profile_sensor_data;

        lidar_sub_.subscribe(this, "/livox/lidar", qos_profile);
        camera_sub_.subscribe(this, "/camera1/camera1/color/image_raw", qos_profile);
        pose_sub_.subscribe(this, "/dlio/odom_node/pose", qos_profile);
        camera_info_sub_.subscribe(this, "/camera1/camera1/color/camera_info", qos_profile);
        depth_sub_.subscribe(this, "/camera1/camera1/aligned_depth_to_color/image_raw", qos_profile); // New subscriber

        sync_.reset(new Sync(SyncPolicy(100), lidar_sub_, camera_sub_, pose_sub_, camera_info_sub_, depth_sub_));
        sync_->registerCallback(std::bind(&SyncedSaver::sync_callback, this, 
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
    }

private:
    using SyncPolicy = message_filters::sync_policies::ApproximateTime<
        sensor_msgs::msg::PointCloud2,
        sensor_msgs::msg::Image,
        geometry_msgs::msg::PoseStamped,
        sensor_msgs::msg::CameraInfo,
        sensor_msgs::msg::Image>;
    using Sync = message_filters::Synchronizer<SyncPolicy>;

    void sync_callback(
        const sensor_msgs::msg::PointCloud2::ConstSharedPtr& cloud_msg,
        const sensor_msgs::msg::Image::ConstSharedPtr& image_msg,
        const geometry_msgs::msg::PoseStamped::ConstSharedPtr& pose_msg,
        const sensor_msgs::msg::CameraInfo::ConstSharedPtr& info_msg,
        const sensor_msgs::msg::Image::ConstSharedPtr& depth_msg) { 
            double timestamp = cloud_msg->header.stamp.sec + (cloud_msg->header.stamp.nanosec * 1e-9);

            std::ostringstream time_oss;
            time_oss << std::fixed << std::setprecision(9) << timestamp;
            std::string time_str = time_oss.str();

            save_pcd(cloud_msg, pcd_dir_ + time_str + ".pcd");
            save_image(image_msg, img_dir_ + time_str + ".png");
            save_pose(pose_msg, pose_dir_ + time_str + ".txt");
            save_intrinsics(info_msg, calib_dir_ + time_str + ".txt");
            save_depth(depth_msg, depth_dir_ + time_str + ".png"); 
        }

    void save_pcd(const sensor_msgs::msg::PointCloud2::ConstSharedPtr& msg, const std::string& filename) {
        pcl::PointCloud<pcl::PointXYZ> cloud;
        pcl::fromROSMsg(*msg, cloud);
        pcl::io::savePCDFileBinary(filename, cloud); 
    }

    void save_image(const sensor_msgs::msg::Image::ConstSharedPtr& msg, const std::string& filename) {
        try {
            cv_bridge::CvImageConstPtr cv_ptr = cv_bridge::toCvShare(msg, "bgr8");
            cv::imwrite(filename, cv_ptr->image);
        } catch (cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "cv_bridge exception (RGB): %s", e.what());
        }
    }

    void save_depth(const sensor_msgs::msg::Image::ConstSharedPtr& msg, const std::string& filename) {
        try {
            cv_bridge::CvImageConstPtr cv_ptr = cv_bridge::toCvShare(msg, sensor_msgs::image_encodings::TYPE_16UC1);
            cv::imwrite(filename, cv_ptr->image);
        } catch (cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "cv_bridge exception (Depth): %s", e.what());
        }
    }

    void save_pose(const geometry_msgs::msg::PoseStamped::ConstSharedPtr& msg, const std::string& filename) {
        std::ofstream file(filename);

        if (!file.is_open()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to open pose file: %s", filename.c_str());
            return;
        }

        Eigen::Quaterniond q_orig(
            msg->pose.orientation.w,
            msg->pose.orientation.x,
            msg->pose.orientation.y,
            msg->pose.orientation.z
        );
        Eigen::Matrix4d T_orig = Eigen::Matrix4d::Identity();
        T_orig.block<3,3>(0,0) = q_orig.toRotationMatrix();
        T_orig.block<3,1>(0,3) = Eigen::Vector3d(
            msg->pose.position.x,
            msg->pose.position.y,
            msg->pose.position.z
        );

        Eigen::Matrix4d T_x180 = Eigen::Matrix4d::Identity();
        T_x180(1, 1) = -1.0;
        T_x180(2, 2) = -1.0;

        Eigen::Quaterniond q_ext(
            0.5824216408768541,   // qw
            0.4051169630628588,   // qx
            0.4006810126971365,   // qy
            0.5797585743574961    // qz
        );
        Eigen::Matrix4d T_ext = Eigen::Matrix4d::Identity();
        T_ext.block<3,3>(0,0) = q_ext.toRotationMatrix();
        T_ext.block<3,1>(0,3) = Eigen::Vector3d(
            0.07093786809565478,  // tx
            0.012665553133702897, // ty
            -0.08437335095076476  // tz
        );

        Eigen::Matrix4d T_rotated = T_x180 * T_orig;
        
        Eigen::Matrix4d T_final = T_rotated * T_ext; 

        file << std::scientific << std::setprecision(18);

        for (int i = 0; i < 4; ++i) {
            file << T_final(i, 0) << " " << T_final(i, 1) << " " << T_final(i, 2) << " " << T_final(i, 3) << "\n";
        }
        
        file.close();
    }

    void save_intrinsics(const sensor_msgs::msg::CameraInfo::ConstSharedPtr& msg, const std::string& filename) {
        std::ofstream file(filename);
        file << std::scientific << std::setprecision(18);

        file << msg->k[0] << " " << msg->k[1] << " " << msg->k[2] << "\n";
        file << msg->k[3] << " " << msg->k[4] << " " << msg->k[5] << "\n";
        file << msg->k[6] << " " << msg->k[7] << " " << msg->k[8] << "\n";
        
        file.close();
    }

    std::string pcd_dir_, img_dir_, pose_dir_, calib_dir_, depth_dir_;

    message_filters::Subscriber<sensor_msgs::msg::PointCloud2> lidar_sub_;
    message_filters::Subscriber<sensor_msgs::msg::Image> camera_sub_;
    message_filters::Subscriber<geometry_msgs::msg::PoseStamped> pose_sub_;
    message_filters::Subscriber<sensor_msgs::msg::CameraInfo> camera_info_sub_;
    message_filters::Subscriber<sensor_msgs::msg::Image> depth_sub_; 
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