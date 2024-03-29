#include <ros/ros.h>
#include "../include/ekf.h"
#include <ros/console.h>
#include "tf/transform_datatypes.h"
#include <thread>
#include <math.h>

using namespace Eigen;

int main(int argc, char **argv)
{
    ros::init(argc, argv, "extended_kalman_filter");
    ros::NodeHandle nh_;

    nh_.param("divergence_deg",divergence,10.0);

    vel_sub_ = nh_.subscribe("camera/odom/sample", 100, velCallback);
    laser_sub_ = nh_.subscribe("laser_strength", 100, laserCallback);
    estimate_pub_ = nh_.advertise<nav_msgs::Odometry>("ekf_estimate", 1);

    // std::thread propagation(propagate);

    // Initialize x
    x << 0.6 + 0.2, 0, 0.338, 0;            // r, rdot, theta, thetadot

     // Initialize Q
    double r_process_noise = 0.005;
    double theta_process_noise = 0.005;
    double rdot_process_noise = 0.01;
    double theta_dot_process_noise = 0.01;
    Q <<    r_process_noise, 0, 0, 0,
            0, theta_process_noise, 0, 0,
            0, 0, rdot_process_noise, 0,
            0, 0, 0, theta_dot_process_noise; 
    
    // Initialize P
    P <<    0.2, 0, 0, 0,
            0, 0.1, 0, 0,
            0, 0, 0.1, 0,
            0, 0, 0, 0.1;
    
    // Initialize R
    double laser_cov = 0.08; // 0.08
    double vel_cov = 0.1;
    VectorXd meas_cov(3);
    meas_cov << laser_cov, vel_cov, vel_cov;
    R = meas_cov.asDiagonal();

    // Initialize Things
    z(1) = 0;
    z(2) = 0;
    altitude = 1.2;
    x_uav = 0;
    y_uav = 0;


    time_prev_ = ros::Time::now();
    ros::spin();

    // propagation.join();

    return 0;
}


void velCallback(const nav_msgs::OdometryConstPtr& msg)
// Receive the IMU data, sort, send to calculateEstimate()
{
    z(1) = -msg->twist.twist.linear.x;
    z(2) = -msg->twist.twist.linear.y;
    altitude = 1.2; //-msg->pose.pose.position.z;

    x_uav = msg->pose.pose.position.x;
    y_uav = msg->pose.pose.position.y;

    // propagate();
    // update();
    // publishEstimate();
}

void laserCallback(const std_msgs::Float64ConstPtr& msg)
// Receive the IMU data, sort, send to calculateEstimate()
{
    z(0) = msg->data;

    propagate();
    update();
    publishEstimate();
}

void propagate()
{
    ros::Time now = ros::Time::now();
    double dt = now.toSec() - time_prev_.toSec();
    time_prev_ = now;
    F = I;
    F(0,2) = dt;
    F(1,3) = dt;
    x = F*x;
    P = F*P*F.transpose() + Q;
}

// void propagate()
// {   
//     ros::Rate rate(100);
//     while (ros::ok())
//     {
//         ros::Time now = ros::Time::now();
//         double dt = now.toSec() - time_prev_.toSec();
//         time_prev_ = now;
//         Vector3d dt_vec(dt, dt, dt);
//         // Build F
//         F = I;
//         F.topRightCorner(3, 3) = dt_vec.asDiagonal();
//         x = F*x;
//         P = F*P*F.transpose() + Q;
//         rate.sleep();
//     }
// }

void update()
{
    seth();
    setH();
    y = z - h;
    S = H*P*H.transpose() + R;
    K = P*H.transpose()*S.inverse();
    x = x + K*y;
    P = (I - K*H)*P;
}

void publishEstimate()
{
    double x_rel = x(0)*cos(x(1));
    double y_rel = x(0)*sin(x(1));

    double x_rov = x_rel + x_uav;
    double y_rov = y_rel + y_uav;
    
    estimate_msg_.header.stamp = ros::Time::now();
    estimate_msg_.pose.pose.position.x = x_rov;
    estimate_msg_.pose.pose.position.y = y_rov;
    // estimate_msg_.twist.twist.linear.x = x(1);
    estimate_msg_.pose.covariance[0] = P(0,0);
    estimate_msg_.pose.covariance[3] = P(1,1);


    estimate_pub_.publish(estimate_msg_);
}

void setH()
{
    double z = altitude;
    double theta = divergence*3.1415926535/180;
    double n = 10;
    double dIdr = 1.5*exp(-2*pow((x(0)/(z*sin(theta/2))),n))*-2*n/pow((z*sin(theta/2)),n)*pow(x(0),(n-1));
    double dvxdr = -x(3)*sin(x(1));
    double dvydr = x(3)*cos(x(1));
    double dvxdtheta = -x(2)*sin(x(1)) - x(0)*x(3)*cos(x(1));
    double dvydtheta = x(2)*cos(x(1)) - x(0)*x(3)*sin(x(1));
    double dvxdrdot = cos(x(1));
    double dvydrdot = sin(x(1));
    double dvxdthetadot = -x(0)*sin(x(1));
    double dvydthetadot = x(0)*cos(x(1));
    H <<    dIdr, 0, 0, 0,
            dvxdr, dvxdtheta, dvxdrdot, dvxdthetadot,
            dvydr, dvydtheta, dvydrdot, dvydthetadot;
}

void seth()
{
    double z = altitude;
    double theta = divergence*3.1415926535/180;
    double n = 10;
    double I = 1.5*exp(-2*pow((x(0)/(z*sin(theta/2))),n));
    double velx = x(2)*cos(x(1)) - x(0)*x(3)*sin(x(1));
    double vely = x(2)*sin(x(1)) + x(0)*x(3)*cos(x(1));
    
    VectorXd h_temp(3);
    h_temp << I, velx, vely;
    h = h_temp;
}