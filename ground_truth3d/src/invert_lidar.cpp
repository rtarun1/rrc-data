#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/point_cloud2_iterator.hpp>
#include <cmath>

class InvertLiDAR : public rclcpp::Node {
public:
    InvertLiDAR() : Node("invert_lidar"){

    auto qos = rclcpp::QoS(rclcpp::KeepLast(10)).reliable();

    lidar_sub_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
            "/livox/lidar", qos, 
            std::bind(&InvertLiDAR::lidar_callback, this, std::placeholders::_1));

    imu_sub_ = this->create_subscription<sensor_msgs::msg::Imu>(
            "/livox/imu", qos, 
            std::bind(&InvertLiDAR::imu_callback, this, std::placeholders::_1));

    lidar_pub_ = this->create_publisher<sensor_msgs::msg::PointCloud2>(
        "/livox/lidar_inverted", qos);
        
    imu_pub_ = this->create_publisher<sensor_msgs::msg::Imu>(
        "/livox/imu_inverted", qos);

    RCLCPP_INFO(this->get_logger(), "Invert Lidar node started");
    }

private:
    // using unique pointer to do zero copy transfer
    void lidar_callback(sensor_msgs::msg::PointCloud2::UniquePtr msg) {
        sensor_msgs::PointCloud2Iterator<float> iter_y(*msg, "y");
        sensor_msgs::PointCloud2Iterator<float> iter_z(*msg, "z");

        for (; iter_y != iter_y.end() && iter_z != iter_z.end(); ++iter_y, ++iter_z) {
            *iter_y = -(*iter_y);
            *iter_z = -(*iter_z);
        }

        lidar_pub_->publish(std::move(msg));
    }

    void imu_callback(sensor_msgs::msg::Imu::UniquePtr msg) {

        msg->angular_velocity.y = -msg->angular_velocity.y;
        msg->angular_velocity.z = -msg->angular_velocity.z;

        msg->linear_acceleration.y = -msg->linear_acceleration.y;
        msg->linear_acceleration.z = -msg->linear_acceleration.z;

        imu_pub_->publish(std::move(msg));
    }

    rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr lidar_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub_;
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr lidar_pub_;
    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<InvertLiDAR>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}