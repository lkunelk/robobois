#include "ros/ros.h"
#include <geometry_msgs/Twist.h>
#include <kobuki_msgs/BumperEvent.h>
#include <math.h>

#define RAD2DEG(rad) ((rad)*180. / M_PI)
#define SIGN(num) (std::abs(num) / num)

uint8_t bumper[3] = {kobuki_msgs::BumperEvent::RELEASED, kobuki_msgs::BumperEvent::RELEASED, kobuki_msgs::BumperEvent::RELEASED};

void bumperCallback(const kobuki_msgs::BumperEvent::ConstPtr &msg)
{
    // Access using bumper[kobuki_msgs::BumperEvent::{}] LEFT, CENTER, or RIGHT
    bumper[msg->bumper] = msg->state;
    
}

class Move{
    private:
        ros::Publisher vel_pub;
        ros::Subscriber bumper_sub;
        geometry_msgs::Twist vel;
        double next_update;
        bool bumped;

    public:
    Move(ros::NodeHandle nh){
        vel_pub = nh.advertise<geometry_msgs::Twist>("cmd_vel_mux/input/teleop", 1);
        bumper_sub = nh.subscribe("mobile_base/events/bumper", 10, &bumperCallback);
        reset_bumped();
        ROS_INFO("Mover initiated.");
    }

    void forward(double distance, double speed, bool verbose=false){
        bool going_backwards = SIGN(speed) == -1 || SIGN(distance) == -1;
        next_update = ros::Time::now().toSec() + std::abs(distance) / std::abs(speed);
        vel.linear.x = speed * SIGN(distance);
        vel.angular.z = 0;
        if (verbose){
            ROS_INFO("Moving %f m, will take %f s, will finish at %f s", distance, std::abs(distance) / std::abs(speed), next_update);
        }
        while(ros::ok() && ros::Time::now().toSec() < next_update){
            if (is_collision() && !going_backwards){
                // Regarding 2nd term: Can still go backwards when bumper pressed
                ROS_WARN("Collision!");
                stop(true);
                bumped = true;
                break;
            }
            vel_pub.publish(vel);
        }
    }

    void rotate(double angle, double speed, bool verbose=false){
        if (std::abs(angle) > 2 * M_PI){
            ROS_WARN("Angle given might be in degrees");
        }
        next_update = ros::Time::now().toSec() + std::abs(angle) / std::abs(speed);
        vel.linear.x = 0;
        vel.angular.z = speed * SIGN(angle);

        if (verbose){
            ROS_INFO("Rotating %f deg, will take %f s, will finish at %f s", RAD2DEG(angle), std::abs(angle) / std::abs(speed), next_update);    
        }
        while(ros::ok() && ros::Time::now().toSec() < next_update){
            if (is_collision()){
                bumped = true;
                stop(true);
                break;
            }
            vel_pub.publish(vel);
        }
    }

    void stop(bool verbose=false){
        if (verbose){
            ROS_INFO("STOPPED");
        }
        vel.linear.x = 0;
        vel.angular.z = 0;

        vel_pub.publish(vel);
    }

    double get_next_update(){
        return next_update;
    }

    double is_bumped(){
        return bumped;
    }
    void reset_bumped(){
        bumped = false;
    }

    bool is_collision(){
        ros::spinOnce();
        return bumper[kobuki_msgs::BumperEvent::LEFT] == kobuki_msgs::BumperEvent::PRESSED ||
            bumper[kobuki_msgs::BumperEvent::CENTER] == kobuki_msgs::BumperEvent::PRESSED ||
            bumper[kobuki_msgs::BumperEvent::RIGHT] == kobuki_msgs::BumperEvent::PRESSED;
    }

};
