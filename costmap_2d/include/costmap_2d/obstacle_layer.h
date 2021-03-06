/*********************************************************************
 *
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2008, 2013, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Eitan Marder-Eppstein
 *         David V. Lu!!
 *********************************************************************/
#ifndef COSTMAP_2D_OBSTACLE_LAYER_H_
#define COSTMAP_2D_OBSTACLE_LAYER_H_

#include <ros/ros.h>
#include <costmap_2d/costmap_layer.h>
#include <costmap_2d/layered_costmap.h>
#include <costmap_2d/observation_buffer.h>

#include <nav_msgs/OccupancyGrid.h>

#include <sensor_msgs/LaserScan.h>
#include <laser_geometry/laser_geometry.h>
#include <sensor_msgs/PointCloud.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/point_cloud_conversion.h>
#include <tf/message_filter.h>
#include <message_filters/subscriber.h>
#include <dynamic_reconfigure/server.h>
#include <costmap_2d/ObstaclePluginConfig.h>

#include <string>  // for string
#include <vector>  // for vector<>
#include <list>
#include <boost/tuple/tuple.hpp>
#include <costmap_2d/footprint.h>
#include <std_msgs/Float64.h>

namespace costmap_2d
{

/// @brief Wall clock time, x and y coordinates in global reference frame
typedef boost::tuple<double, double, double> TimeWorldPoint;

class ObstacleLayer : public CostmapLayer
{
public:
  ObstacleLayer()
  {
    costmap_ = NULL;  // this is the unsigned char* member of parent class Costmap2D.
  }

  virtual ~ObstacleLayer();
  virtual void onInitialize();
  virtual void updateBounds(double robot_x, double robot_y, double robot_yaw, double* min_x, double* min_y,
                            double* max_x, double* max_y);
  virtual void updateCosts(costmap_2d::Costmap2D& master_grid, int min_i, int min_j, int max_i, int max_j);
  virtual void updateCosts(LayerActions *layer_actions, Costmap2D &master_grid,
                           int min_i, int min_j, int max_i, int max_j);

  virtual void activate();
  virtual void deactivate();
  virtual void reset();

  /**
   * @brief Flags that the obstacle memory should be cleared the next time we update the map
   */
  void clearObstacleMemory();

  /**
   * Indicates whether obstacle memory is currently on
   * @return true if memory is enabled, false otherwise
   */
  bool isMemoryEnabled();

  /**
   * @brief Used to enable or disable the obstacle memory
   * @param[in] enabled Whether to enable or disable the memory
   */
  void setMemoryEnabled(const bool enabled);

  /**
   * @brief  A callback to handle buffering LaserScan messages
   * @param message The message returned from a message notifier
   * @param buffer A pointer to the observation buffer to update
   * @param add_max_range Whether to add the max range meeasurements to the scan
   */
  void laserScanCallback(const sensor_msgs::LaserScanConstPtr& message,
                         const boost::shared_ptr<costmap_2d::ObservationBuffer>& buffer,
                         const bool add_max_range);

   /**
    * @brief A callback to handle buffering LaserScan messages which need filtering to turn Inf values into range_max.
    * @param message The message returned from a message notifier
    * @param buffer A pointer to the observation buffer to update
    */
  void laserScanValidInfCallback(const sensor_msgs::LaserScanConstPtr& message,
                                 const boost::shared_ptr<ObservationBuffer>& buffer);

  /**
   * @brief  A callback to handle buffering PointCloud messages
   * @param message The message returned from a message notifier
   * @param buffer A pointer to the observation buffer to update
   */
  void pointCloudCallback(const sensor_msgs::PointCloudConstPtr& message,
                          const boost::shared_ptr<costmap_2d::ObservationBuffer>& buffer);

  /**
   * @brief  A callback to handle buffering PointCloud2 messages
   * @param message The message returned from a message notifier
   * @param buffer A pointer to the observation buffer to update
   */
  void pointCloud2Callback(const sensor_msgs::PointCloud2ConstPtr& message,
                           const boost::shared_ptr<costmap_2d::ObservationBuffer>& buffer);

  /**
   * @brief A callback to handle localization confidence
   * @param message The confidence
   */
  void poseConfidenceCallback(const std_msgs::Float64& message);

  // for testing purposes
  void addStaticObservation(costmap_2d::Observation& obs, bool marking, bool clearing);
  void clearStaticObservations(bool marking, bool clearing);

protected:
  /// @brief customized version of updateBounds that explicitely remembers and forgets observations
  virtual void forgetfulUpdateBounds(double robot_x, double robot_y, double robot_yaw,
                            double* min_x, double* min_y, double* max_x, double* max_y);

  /**
   * @brief  Write a pixel on the costmap at a given TimeWorldPoint
   * @param p The TimeWorldPoint
   * @param value The new pixel value
   * @param min_x bounding box coordinates to update to include this operation
   * @param min_y
   * @param max_x
   * @param max_y
   */
  void writeTimeWorldPoint(const TimeWorldPoint& p, unsigned char value,
                           double* min_x, double* min_y, double* max_x, double* max_y);

  virtual void setupDynamicReconfigure(ros::NodeHandle& nh);

  /**
   * @brief  Get the observations used to mark space
   * @param marking_observations A reference to a vector that will be populated with the observations
   * @return True if all the observation buffers are current, false otherwise
   */
  bool getMarkingObservations(std::vector<costmap_2d::Observation>& marking_observations) const;

  /**
   * @brief  Get the observations used to clear space
   * @param clearing_observations A reference to a vector that will be populated with the observations
   * @return True if all the observation buffers are current, false otherwise
   */
  bool getClearingObservations(std::vector<costmap_2d::Observation>& clearing_observations) const;

  /**
   * @brief  Clear freespace based on one observation
   * @param clearing_observation The observation used to raytrace
   * @param min_x
   * @param min_y
   * @param max_x
   * @param max_y
   */
  virtual void raytraceFreespace(const costmap_2d::Observation& clearing_observation, double* min_x, double* min_y,
                                 double* max_x, double* max_y);

  void updateRaytraceBounds(double ox, double oy, double wx, double wy, double range, double* min_x, double* min_y,
                            double* max_x, double* max_y);

  std::vector<geometry_msgs::Point> transformed_footprint_;
  bool footprint_clearing_enabled_;
  void updateFootprint(double robot_x, double robot_y, double robot_yaw, double* min_x, double* min_y, 
                       double* max_x, double* max_y);

  std::string global_frame_;  ///< @brief The global frame for the costmap
  double max_obstacle_height_;  ///< @brief Max Obstacle Height

  laser_geometry::LaserProjection projector_;  ///< @brief Used to project laser scans into point clouds

  std::vector<boost::shared_ptr<message_filters::SubscriberBase> > observation_subscribers_;  ///< @brief Used for the observation message filters
  std::vector<boost::shared_ptr<tf::MessageFilterBase> > observation_notifiers_;  ///< @brief Used to make sure that transforms are available for each sensor
  std::vector<boost::shared_ptr<costmap_2d::ObservationBuffer> > observation_buffers_;  ///< @brief Used to store observations from various sensors
  std::vector<boost::shared_ptr<costmap_2d::ObservationBuffer> > marking_buffers_;  ///< @brief Used to store observation buffers used for marking obstacles
  std::vector<boost::shared_ptr<costmap_2d::ObservationBuffer> > clearing_buffers_;  ///< @brief Used to store observation buffers used for clearing obstacles

  // Used only for testing purposes
  std::vector<costmap_2d::Observation> static_clearing_observations_, static_marking_observations_;

  bool rolling_window_;
  dynamic_reconfigure::Server<costmap_2d::ObstaclePluginConfig> *dsrv_;

  int combination_method_;

private:
  void reconfigureCB(costmap_2d::ObstaclePluginConfig &config, uint32_t level);

  int min_x_;  ///< @brief bounding box in cell coordinates
  int min_y_;  ///< @brief bounding box in cell coordinates
  int max_x_;  ///< @brief bounding box in cell coordinates
  int max_y_;  ///< @brief bounding box in cell coordinates

  double obstacle_lifespan_; // seconds
  double obstacle_keep_radius_; // meters
  int obstacle_queue_size_; // observations
  double obstacle_compare_tolerance_; // meters
  bool use_forgetful_version_;

  typedef std::map< std::pair<unsigned int,unsigned int>, TimeWorldPoint> obst_map_t;
  obst_map_t time_world_points_;

  ros::Subscriber pose_confidence_sub_;
  float pose_confidence_;           /// <@brief current confidence in pose.
  float pose_confidence_threshold_; /// <@brief below this threshold we will not remember obstacles.

  bool clear_obstacle_memory_;  /// <@brief Used to reset the obstacle memory before the next update
};

}  // namespace costmap_2d

#endif  // COSTMAP_2D_OBSTACLE_LAYER_H_
