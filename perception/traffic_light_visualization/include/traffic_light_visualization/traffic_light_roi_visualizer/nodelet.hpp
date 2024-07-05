// Copyright 2020 Tier IV, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef TRAFFIC_LIGHT_VISUALIZATION__TRAFFIC_LIGHT_ROI_VISUALIZER__NODELET_HPP_
#define TRAFFIC_LIGHT_VISUALIZATION__TRAFFIC_LIGHT_ROI_VISUALIZER__NODELET_HPP_

#include <image_transport/image_transport.hpp>
#include <image_transport/subscriber_filter.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <rclcpp/rclcpp.hpp>

#include <sensor_msgs/msg/image.hpp>
#include <tier4_perception_msgs/msg/traffic_light_array.hpp>
#include <tier4_perception_msgs/msg/traffic_light_roi_array.hpp>

#if __has_include(<cv_bridge/cv_bridge.hpp>)
#include <cv_bridge/cv_bridge.hpp>  // for ROS 2 Jazzy or newer
#else
#include <cv_bridge/cv_bridge.h>  // for ROS 2 Humble or older
#endif
#include <message_filters/subscriber.h>
#include <message_filters/sync_policies/approximate_time.h>
#include <message_filters/synchronizer.h>
#include <opencv2/imgproc/imgproc_c.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace traffic_light
{
struct ClassificationResult
{
  float prob = 0.0;
  std::string label;
};

class TrafficLightRoiVisualizerNodelet : public rclcpp::Node
{
public:
  explicit TrafficLightRoiVisualizerNodelet(const rclcpp::NodeOptions & options);
  void connectCb();

  void imageRoiCallback(
    const sensor_msgs::msg::Image::ConstSharedPtr & input_image_msg,
    const tier4_perception_msgs::msg::TrafficLightRoiArray::ConstSharedPtr & input_tl_roi_msg,
    const tier4_perception_msgs::msg::TrafficLightArray::ConstSharedPtr &
      input_traffic_signals_msg);

  void imageRoughRoiCallback(
    const sensor_msgs::msg::Image::ConstSharedPtr & input_image_msg,
    const tier4_perception_msgs::msg::TrafficLightRoiArray::ConstSharedPtr & input_tl_roi_msg,
    const tier4_perception_msgs::msg::TrafficLightRoiArray::ConstSharedPtr & input_tl_rough_roi_msg,
    const tier4_perception_msgs::msg::TrafficLightArray::ConstSharedPtr &
      input_traffic_signals_msg);

private:
  std::map<int, std::string> state2label_{
    // color
    {tier4_perception_msgs::msg::TrafficLightElement::RED, "red"},
    {tier4_perception_msgs::msg::TrafficLightElement::AMBER, "yellow"},
    {tier4_perception_msgs::msg::TrafficLightElement::GREEN, "green"},
    {tier4_perception_msgs::msg::TrafficLightElement::WHITE, "white"},
    // shape
    {tier4_perception_msgs::msg::TrafficLightElement::CIRCLE, "circle"},
    {tier4_perception_msgs::msg::TrafficLightElement::LEFT_ARROW, "left"},
    {tier4_perception_msgs::msg::TrafficLightElement::RIGHT_ARROW, "right"},
    {tier4_perception_msgs::msg::TrafficLightElement::UP_ARROW, "straight"},
    {tier4_perception_msgs::msg::TrafficLightElement::DOWN_ARROW, "down"},
    {tier4_perception_msgs::msg::TrafficLightElement::DOWN_LEFT_ARROW, "down_left"},
    {tier4_perception_msgs::msg::TrafficLightElement::DOWN_RIGHT_ARROW, "down_right"},
    {tier4_perception_msgs::msg::TrafficLightElement::CROSS, "cross"},
    // other
    {tier4_perception_msgs::msg::TrafficLightElement::UNKNOWN, "unknown"},
  };

  std::string extractShapeName(const std::string & label)
  {
    size_t start_pos = label.find('-');
    if (start_pos != std::string::npos) {
      start_pos++;                                  // Start after the hyphen
      size_t end_pos = label.find(',', start_pos);  // Find the next comma after the hyphen
      if (end_pos == std::string::npos) {  // If no comma is found, take the rest of the string
        end_pos = label.length();
      }
      return label.substr(start_pos, end_pos - start_pos);
    }
    return "unknown";  // Return "unknown" if no hyphen is found
  }

  bool createRect(
    cv::Mat & image, const tier4_perception_msgs::msg::TrafficLightRoi & tl_roi,
    const cv::Scalar & color);

  bool createRect(
    cv::Mat & image, const tier4_perception_msgs::msg::TrafficLightRoi & tl_roi,
    const ClassificationResult & result);

  bool getClassificationResult(
    int id, const tier4_perception_msgs::msg::TrafficLightArray & traffic_signals,
    ClassificationResult & result);

  bool getRoiFromId(
    int id, const tier4_perception_msgs::msg::TrafficLightRoiArray::ConstSharedPtr & rois,
    tier4_perception_msgs::msg::TrafficLightRoi & correspond_roi);

  rclcpp::TimerBase::SharedPtr timer_;
  image_transport::SubscriberFilter image_sub_;
  message_filters::Subscriber<tier4_perception_msgs::msg::TrafficLightRoiArray> roi_sub_;
  message_filters::Subscriber<tier4_perception_msgs::msg::TrafficLightRoiArray> rough_roi_sub_;
  message_filters::Subscriber<tier4_perception_msgs::msg::TrafficLightArray> traffic_signals_sub_;
  image_transport::Publisher image_pub_;
  typedef message_filters::sync_policies::ApproximateTime<
    sensor_msgs::msg::Image, tier4_perception_msgs::msg::TrafficLightRoiArray,
    tier4_perception_msgs::msg::TrafficLightArray>
    SyncPolicy;
  typedef message_filters::Synchronizer<SyncPolicy> Sync;
  std::shared_ptr<Sync> sync_;

  typedef message_filters::sync_policies::ApproximateTime<
    sensor_msgs::msg::Image, tier4_perception_msgs::msg::TrafficLightRoiArray,
    tier4_perception_msgs::msg::TrafficLightRoiArray, tier4_perception_msgs::msg::TrafficLightArray>
    SyncPolicyWithRoughRoi;
  typedef message_filters::Synchronizer<SyncPolicyWithRoughRoi> SyncWithRoughRoi;
  std::shared_ptr<SyncWithRoughRoi> sync_with_rough_roi_;

  bool enable_fine_detection_;
};

}  // namespace traffic_light

#endif  // TRAFFIC_LIGHT_VISUALIZATION__TRAFFIC_LIGHT_ROI_VISUALIZER__NODELET_HPP_
