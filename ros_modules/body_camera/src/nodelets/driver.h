/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2011 Willow Garage, Inc.
 *    Radu Bogdan Rusu <rusu@willowgarage.com>
 *    Suat Gedikli <gedikli@willowgarage.com>
 *    Patrick Mihelich <mihelich@willowgarage.com>
 *
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
 */
#ifndef OPENNI_CAMERA_DRIVER_H
#define OPENNI_CAMERA_DRIVER_H

// ROS communication
#include <ros/ros.h>
#include <nodelet/nodelet.h>
#include <image_transport/image_transport.h>

// Configuration
#include <camera_info_manager/camera_info_manager.h>
#include <dynamic_reconfigure/server.h>
#include <body_camera/OpenNIUnstableConfig.h>

// OpenNI
#include "body_camera/openni_driver.h"

namespace body_camera
{
  ////////////////////////////////////////////////////////////////////////////////////////////
  class DriverNodelet : public nodelet::Nodelet
  {
    public:
      virtual ~DriverNodelet ();
    private:
      typedef OpenNIUnstableConfig Config;
      typedef dynamic_reconfigure::Server<Config> ReconfigureServer;

      /** \brief Nodelet initialization routine. */
      virtual void onInit ();
      void setupDevice ();
      void updateModeMaps ();
      void startSynchronization ();
      void stopSynchronization ();
      void setupDeviceModes (int image_mode, int depth_mode);

      /// @todo Consolidate all the mode stuff, maybe even in different class/header
      int mapXnMode2ConfigMode (const XnMapOutputMode& output_mode) const;
      XnMapOutputMode mapConfigMode2XnMode (int mode) const;

      // Callback methods
      void imageCb(boost::shared_ptr<openni_wrapper::Image> image, void* cookie);
      void depthCb(boost::shared_ptr<openni_wrapper::DepthImage> depth_image, void* cookie);
      void irCb(boost::shared_ptr<openni_wrapper::IRImage> ir_image, void* cookie);
      void connectCb();
      void configCb(Config &config, uint32_t level);

      // Methods to check whether to start/stop each stream
      inline bool isImageStreamRequired() const;
      inline bool isDepthStreamRequired() const;
      inline bool isIrStreamRequired() const;
    
      // Methods to get calibration parameters for the various cameras
      sensor_msgs::CameraInfoPtr getDefaultCameraInfo(int width, int height, double f) const;
      sensor_msgs::CameraInfoPtr getRgbCameraInfo(ros::Time time) const;
      sensor_msgs::CameraInfoPtr getIrCameraInfo(ros::Time time) const;
      sensor_msgs::CameraInfoPtr getDepthCameraInfo(ros::Time time) const;

      // published topics
      ros::Publisher pub_rgb_info_, pub_depth_info_, pub_depth_registered_info_, pub_ir_info_;
      image_transport::Publisher pub_rgb_raw_, pub_rgb_mono_, pub_rgb_color_;
      image_transport::Publisher pub_depth_, pub_depth_registered_;
      image_transport::Publisher pub_ir_;

      // publish methods
      void publishRgbImageRaw (const openni_wrapper::Image& image, ros::Time time) const;
      void publishGrayImage (const openni_wrapper::Image& image, ros::Time time) const;
      void publishRgbImage (const openni_wrapper::Image& image, ros::Time time) const;
      void publishDepthImageRaw(const openni_wrapper::DepthImage& depth,
                                ros::Time time, const std::string& frame_id,
                                const image_transport::Publisher& pub) const;
      void publishIrImage(const openni_wrapper::IRImage& ir, ros::Time time) const;

      /** \brief the actual openni device*/
      boost::shared_ptr<openni_wrapper::OpenNIDevice> device_;

      /** \brief reconfigure server*/
      boost::shared_ptr<ReconfigureServer> reconfigure_server_;
      Config config_;

      /** \brief Camera info manager objects. */
      boost::shared_ptr<camera_info_manager::CameraInfoManager> rgb_info_manager_, ir_info_manager_;
      std::string rgb_frame_id_;
      std::string depth_frame_id_;
      double depth_ir_offset_x_;
      double depth_ir_offset_y_;
      int z_offset_mm_;

      /// @todo Prefer binning to changing width/height
      unsigned image_width_;
      unsigned image_height_;
      unsigned depth_width_;
      unsigned depth_height_;

      void watchDog(const ros::TimerEvent& event);

      /** \brief timeout value in seconds to throw TIMEOUT exception */
      double time_out_;
      ros::Time time_stamp_;
      ros::Timer watch_dog_timer_;

      struct modeComp
      {
        bool operator () (const XnMapOutputMode& mode1, const XnMapOutputMode& mode2) const
        {
          if (mode1.nXRes < mode2.nXRes)
            return true;
          else if (mode1.nXRes > mode2.nXRes)
            return false;
          else if (mode1.nYRes < mode2.nYRes)
            return true;
          else if (mode1.nYRes > mode2.nYRes)
            return false;
          else if (mode1.nFPS < mode2.nFPS)
            return true;
          else
            return false;
        }
      };
      std::map<XnMapOutputMode, int, modeComp> xn2config_map_;
      std::map<int, XnMapOutputMode> config2xn_map_;
  };

  bool DriverNodelet::isImageStreamRequired() const
  {
    return (pub_rgb_raw_   .getNumSubscribers() > 0 ||
            pub_rgb_mono_  .getNumSubscribers() > 0 ||
            pub_rgb_color_ .getNumSubscribers() > 0 );
            //            pub_rgb_info_  .getNumSubscribers() > 0 );
  }

  bool DriverNodelet::isDepthStreamRequired() const
  {
    if (device_->isDepthRegistered())
      return (pub_depth_registered_     .getNumSubscribers() > 0 ||
              pub_depth_registered_info_.getNumSubscribers() > 0 );
    else
      return (pub_depth_     .getNumSubscribers() > 0 ||
              pub_depth_info_.getNumSubscribers() > 0 );
  }

  bool DriverNodelet::isIrStreamRequired() const
  {
    return (pub_ir_     .getNumSubscribers() > 0 );
            //            pub_ir_info_.getNumSubscribers() > 0 );
  }

}

#endif
