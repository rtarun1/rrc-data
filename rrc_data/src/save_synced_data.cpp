#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <vector>
#include <string>

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
#include <opencv2/calib3d.hpp>

#include <Eigen/Dense>

class SyncedData : public rclcpp::Node {
public:
    SyncedData() : Node("save_synced_data"){

        this->declare_parameter<std::string>("base_output_dir", "src/data/default_output");
        this->declare_parameter<bool>("save_pcd", true);

        std::string base_dir = this->get_parameter("base_output_dir").as_string();
        save_pcd_ = this->get_parameter("save_pcd").as_bool();

        RCLCPP_INFO(this->get_logger(), "Saving data to: %s", base_dir.c_str());

        img_dir_ = base_dir + "/image/";
        undistort_img_dir_ = base_dir + "/image_undistorted/";
        calib_dir_ = base_dir + "/calibration/";
        discoeff_dir_ = base_dir + "/discoeff/";
        depth_dir_ = base_dir + "/depth/"; 
        pose_dir_ = base_dir + "/pose/";

        std::filesystem::create_directories(img_dir_);
        std::filesystem::create_directories(undistort_img_dir_);
        std::filesystem::create_directories(calib_dir_);
        std::filesystem::create_directories(discoeff_dir_);
        std::filesystem::create_directories(depth_dir_);
        std::filesystem::create_directories(pose_dir_);

        rmw_qos_profile_t qos_profile = rmw_qos_profile_sensor_data;

        camera_sub_.subscribe(this, "/camera1/camera1/color/image_raw", qos_profile);
        camera_info_sub_.subscribe(this, "/camera1/camera1/color/camera_info", qos_profile);
        depth_sub_.subscribe(this, "/camera1/camera1/aligned_depth_to_color/image_raw", qos_profile);
        pose_sub_.subscribe(this, "/glim_rosnode/lidar_pose_corrected", qos_profile);

        if (save_pcd_) {
            pcd_dir_ = base_dir + "/pcd/";
            std::filesystem::create_directories(pcd_dir_);
            
            pc_sub_.subscribe(this, "/livox/lidar", qos_profile);
            
            sync5_.reset(new Sync5(SyncPolicy5(100), pc_sub_, camera_sub_, pose_sub_, camera_info_sub_, depth_sub_));
            sync5_->setMaxIntervalDuration(rclcpp::Duration::from_seconds(0.05));
            sync5_->registerCallback(std::bind(&SyncedData::sync_callback5, this, 
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
        } else {
            sync4_.reset(new Sync4(SyncPolicy4(100), camera_sub_, pose_sub_, camera_info_sub_, depth_sub_));
            sync4_->setMaxIntervalDuration(rclcpp::Duration::from_seconds(0.05));
            sync4_->registerCallback(std::bind(&SyncedData::sync_callback4, this, 
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        }
    }

private:
    using SyncPolicy5 = message_filters::sync_policies::ApproximateTime<
        sensor_msgs::msg::PointCloud2,
        sensor_msgs::msg::Image,
        geometry_msgs::msg::PoseStamped,
        sensor_msgs::msg::CameraInfo,
        sensor_msgs::msg::Image>;
    using Sync5 = message_filters::Synchronizer<SyncPolicy5>;

    using SyncPolicy4 = message_filters::sync_policies::ApproximateTime<
        sensor_msgs::msg::Image,
        geometry_msgs::msg::PoseStamped,
        sensor_msgs::msg::CameraInfo,
        sensor_msgs::msg::Image>;
    using Sync4 = message_filters::Synchronizer<SyncPolicy4>;

    void sync_callback5(
        const sensor_msgs::msg::PointCloud2::ConstSharedPtr& cloud_msg,
        const sensor_msgs::msg::Image::ConstSharedPtr& image_msg,
        const geometry_msgs::msg::PoseStamped::ConstSharedPtr& pose_msg,
        const sensor_msgs::msg::CameraInfo::ConstSharedPtr& info_msg,
        const sensor_msgs::msg::Image::ConstSharedPtr& depth_msg) { 

            RCLCPP_INFO_ONCE(this->get_logger(), "First synchronized message set (5 Topics) received! Saving...");

            std::string time_str = get_time_string(cloud_msg->header.stamp.sec, cloud_msg->header.stamp.nanosec);

            save_pcd(cloud_msg, pcd_dir_ + time_str + ".pcd");
            process_common_data(image_msg, pose_msg, info_msg, depth_msg, time_str);
        }

    void sync_callback4(
        const sensor_msgs::msg::Image::ConstSharedPtr& image_msg,
        const geometry_msgs::msg::PoseStamped::ConstSharedPtr& pose_msg,
        const sensor_msgs::msg::CameraInfo::ConstSharedPtr& info_msg,
        const sensor_msgs::msg::Image::ConstSharedPtr& depth_msg) { 

            RCLCPP_INFO_ONCE(this->get_logger(), "First synchronized message set (4 Topics) received! Saving...");

            std::string time_str = get_time_string(image_msg->header.stamp.sec, image_msg->header.stamp.nanosec);

            process_common_data(image_msg, pose_msg, info_msg, depth_msg, time_str);
        }

    void process_common_data(
        const sensor_msgs::msg::Image::ConstSharedPtr& image_msg,
        const geometry_msgs::msg::PoseStamped::ConstSharedPtr& pose_msg,
        const sensor_msgs::msg::CameraInfo::ConstSharedPtr& info_msg,
        const sensor_msgs::msg::Image::ConstSharedPtr& depth_msg,
        const std::string& time_str) {

            save_images(image_msg, info_msg, img_dir_ + time_str + ".png", undistort_img_dir_ + time_str + ".png");
            save_pose(pose_msg, pose_dir_ + time_str + ".txt");
            save_intrinsics(info_msg, calib_dir_ + time_str + ".txt");
            save_distortion_coeff(info_msg, discoeff_dir_ + time_str + ".txt");
            save_depth(depth_msg, depth_dir_ + time_str + ".png"); 
    }

    std::string get_time_string(int32_t sec, uint32_t nanosec) {
        std::ostringstream time_oss;
        time_oss << sec << "." << std::setw(9) << std::setfill('0') << nanosec;
        return time_oss.str();
    }

    void save_pcd(const sensor_msgs::msg::PointCloud2::ConstSharedPtr& msg, const std::string& filename) {
        pcl::PointCloud<pcl::PointXYZ> cloud;
        pcl::fromROSMsg(*msg, cloud);
        pcl::io::savePCDFileBinary(filename, cloud); 
    }

    void save_images(const sensor_msgs::msg::Image::ConstSharedPtr& img_msg, 
                     const sensor_msgs::msg::CameraInfo::ConstSharedPtr& info_msg, 
                     const std::string& dist_filename, 
                     const std::string& undist_filename) {
        try {
            cv_bridge::CvImageConstPtr cv_ptr = cv_bridge::toCvShare(img_msg, "bgr8");
            cv::Mat distorted_img = cv_ptr->image;
            
            cv::imwrite(dist_filename, distorted_img);

            cv::Mat K = (cv::Mat_<double>(3, 3) << 
                         info_msg->k[0], info_msg->k[1], info_msg->k[2],
                         info_msg->k[3], info_msg->k[4], info_msg->k[5],
                         info_msg->k[6], info_msg->k[7], info_msg->k[8]);
            
            std::vector<double> D_vec(info_msg->d.begin(), info_msg->d.end());
            cv::Mat D(D_vec, true);

            cv::Mat undistorted_img;
            cv::undistort(distorted_img, undistorted_img, K, D);

            cv::imwrite(undist_filename, undistorted_img);

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

        Eigen::Quaterniond q_ext(
            -0.497015705287846,    // qw
            0.49199556220557866,   // qx
            -0.5042116957045895,   // qy
            0.5066422025275273     // qz
        );
        Eigen::Matrix4d T_ext = Eigen::Matrix4d::Identity();
        T_ext.block<3,3>(0,0) = q_ext.toRotationMatrix();
        T_ext.block<3,1>(0,3) = Eigen::Vector3d(
            0.0594751875604552,    // tx
            -0.015623875200303818, // ty
            -0.04515392036142616   // tz
        );

        Eigen::Matrix4d T_final = T_orig * T_ext; 

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

    void save_distortion_coeff(const sensor_msgs::msg::CameraInfo::ConstSharedPtr& msg, const std::string& filename) {
        std::ofstream file(filename);
        file << std::scientific << std::setprecision(18);

        file << "Distortion Model:\n" << msg->distortion_model << "\n";
        file << "D:\n";
        for (const auto& d : msg->d) {
            file << d << " ";
        }
        file << "\n";
        
        file.close();
    }

    bool save_pcd_;
    std::string pcd_dir_, img_dir_, undistort_img_dir_, calib_dir_, depth_dir_, discoeff_dir_, pose_dir_;

    message_filters::Subscriber<sensor_msgs::msg::PointCloud2> pc_sub_;
    message_filters::Subscriber<sensor_msgs::msg::Image> camera_sub_;
    message_filters::Subscriber<sensor_msgs::msg::CameraInfo> camera_info_sub_;
    message_filters::Subscriber<sensor_msgs::msg::Image> depth_sub_; 
    message_filters::Subscriber<geometry_msgs::msg::PoseStamped> pose_sub_;

    std::shared_ptr<Sync5> sync5_;
    std::shared_ptr<Sync4> sync4_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<SyncedData>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}