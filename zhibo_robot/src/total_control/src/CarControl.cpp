#include "CarControl.h"
#include "Constant.h"

CarControl::CarControl(const std::string &name) : rclcpp::Node(name) {
    m_cmdVelPub = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 5);
}

void CarControl::move() {
    static auto msg = geometry_msgs::msg::Twist();
    msg.linear.x = m_forwardSpeed;
    msg.linear.y = 0;
    msg.angular.z = m_rotateSpeed;

    m_cmdVelPub->publish(msg);
}


void CarControl::setTrackResult(const std::vector<int16_t> &result) {
    int16_t distance = result[4];

    if (distance == 0.0) {
        std::cout << "error distance: 0.0" << std::endl;
        m_forwardSpeed = 0.0;
        return;
    }
    if (m_trackResult.size() < 5) {
        m_trackResult.push(distance);
        m_distance = static_cast<int16_t>((m_distance * (m_trackResult.size() - 1) + distance) / m_trackResult.size());
    } else {
        int16_t front = m_trackResult.front();
        m_trackResult.pop();
        m_distance = static_cast<int16_t>((m_distance * 5 - front + distance) / 5);
        m_trackResult.push(distance);
    }
    std::cout << "average distance: " << int(m_distance) << std::endl;
    if (m_distance > PHOTO_DISTANCE4CAR_FORWARD_LIMIT && m_distance < 4000) {
        m_forwardSpeed = 0.1;
        std::cout << "forward!!!!!!!!" << std::endl;
    } else if (m_distance < PHOTO_DISTANCE4CAR_BACKWARD_LIMIT) {
        m_forwardSpeed = -0.1;
        std::cout << "back!!!!!" << std::endl;
    } else {
        m_forwardSpeed = 0;
    }
}

void CarControl::laserScanCallback(const sensor_msgs::msg::LaserScan &scan) {
    double minDistance = INFINITY;
    double minAngle = 0.0;
    int count = 0;
    for (unsigned long i = 0; i < scan.ranges.size(); i++) {
        double curAngle = (scan.angle_min + static_cast<float>(i) * scan.angle_increment) / M_PI * 180.0;
        if (curAngle > LIDAR_TOTAL_DETECT_ANGLE && curAngle < 360 - LIDAR_TOTAL_DETECT_ANGLE) {
            continue;
        }
        if (scan.ranges[i] > 15) {
            continue;
        }
        if (scan.ranges[i] < minDistance) {
            minDistance = scan.ranges[i];
            minAngle = curAngle;
        }
        if (scan.ranges[i] < 0.8) {
            count++;
        }
    }
    std::cout << "count: " << count <<  " minAngle: " << minAngle << " minDistance: " << minDistance << std::endl;
    if (minDistance < 0.8 && count > 5) {
        m_blocked = true;
    } else {
        m_blocked = false;
    }

}

void CarControl::jointStateCallback(const sensor_msgs::msg::JointState &jointState) {
    m_armBottomAngle = jointState.position[0] * 180.0 / M_PI;
    m_armHeightAngle = jointState.position[2] * 180.0 / M_PI;

    if (m_armBottomAngle > ANGLE_MOVE_LIMIT + ARM_ANGLE4CAR_ROTATE_LIMIT) {
        // turn left
        m_rotateSpeed = 0.2;
        return;
    }
    if (m_armBottomAngle < -ANGLE_MOVE_LIMIT - ARM_ANGLE4CAR_ROTATE_LIMIT) {
        // turn right
        m_rotateSpeed = -0.2;
        return;
    }
    m_rotateSpeed = 0.0;
}

void CarControl::stop() {
    m_forwardSpeed = 0;
    move();
    while (!m_trackResult.empty()) {
        m_trackResult.pop();
    }
    m_distance = 0;
}



