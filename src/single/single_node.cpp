#include "bluefox2/single_node.h"
#include "bluefox2/bluefox2_ros.h"

namespace bluefox2 {

SingleNode::SingleNode(const ros::NodeHandle& pnh)
    : CameraNodeBase(pnh),
      bluefox2_ros_(boost::make_shared<Bluefox2Ros>(pnh)) {}

void SingleNode::Acquire() {
  while (is_acquire() && ros::ok()) {
    bluefox2_ros_->RequestSingle();
    // This code is not used since we want the timestamp of the image to be at the end of the
    // exposure period. Mid-frame sync is done in a separate node based on trigger time.
    // const auto expose_us = bluefox2_ros_->camera().GetExposeUs();
    // const auto expose_duration = ros::Duration(expose_us * 1e-6 / 2);
    // const auto time = ros::Time::now() + expose_duration;
    bluefox2_ros_->PublishCamera(ros::Time::now());
    Sleep();
  }
}

void SingleNode::AcquireOnce() {
  if (is_acquire() && ros::ok()) {
    bluefox2_ros_->RequestSingle();
    // This code is not used since we want the timestamp of the image to be at the end of the
    // exposure period. Mid-frame sync is done in a separate node based on trigger time.
    // const auto expose_us = bluefox2_ros_->camera().GetExposeUs();
    // const auto expose_duration = ros::Duration(expose_us * 1e-6 / 2);
    // const auto time = ros::Time::now() + expose_duration;
    bluefox2_ros_->PublishCamera(ros::Time::now());
  }
}

void SingleNode::Setup(Bluefox2DynConfig& config) {
  bluefox2_ros_->set_fps(config.fps);
  bluefox2_ros_->camera().Configure(config);
}

}  // namepace bluefox2
