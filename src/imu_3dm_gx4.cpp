#include <ros/ros.h>
#include <ros/node_handle.h>
#include <diagnostic_updater/diagnostic_updater.h>
#include <diagnostic_updater/publisher.h>

#include <sensor_msgs/FluidPressure.h>
#include <geometry_msgs/Vector3Stamped.h>
#include <geometry_msgs/QuaternionStamped.h>

#include <dscl_msgs/FilterOutput.h>
#include <dscl_msgs/Imu9DOF.h> 
#include "imu.hpp"
#include "helper_funcs/log.h"      	        /* log utils */


using namespace imu_3dm_gx4;

#define kEarthGravity (9.80665)

ros::Publisher pubIMU;
ros::Publisher pubPressure;
ros::Publisher pubFilter;
std::string frameId;

Imu::Info info;
Imu::DiagnosticFields fields;
bool enableMagnetometer;

//  diagnostic_updater resources
std::shared_ptr<diagnostic_updater::Updater> updater;
std::shared_ptr<diagnostic_updater::TopicDiagnostic> imuDiag;
std::shared_ptr<diagnostic_updater::TopicDiagnostic> filterDiag;

void publishData(const Imu::IMUData &data) {
  dscl_msgs::Imu9DOF imu;
  sensor_msgs::FluidPressure pressure;

  //  assume we have all of these since they were requested
  /// @todo: Replace this with a mode graceful failure...
  assert(data.fields & Imu::IMUData::Accelerometer);
  assert(data.fields & Imu::IMUData::Barometer);
  assert(data.fields & Imu::IMUData::Gyroscope);
  //  only check the mag if it's enabled
  if (enableMagnetometer) {
    assert(data.fields & Imu::IMUData::Magnetometer);
  }

  //  timestamp identically
  imu.header.stamp = ros::Time::now();
  imu.header.frame_id = frameId;
  pressure.header.stamp = imu.header.stamp;
  pressure.header.frame_id = frameId;


  imu.acc.x = data.accel[0] * kEarthGravity;
  imu.acc.y = data.accel[1] * kEarthGravity;
  imu.acc.z = data.accel[2] * kEarthGravity;
  imu.ang.x = data.gyro[0];
  imu.ang.y = data.gyro[1];
  imu.ang.z = data.gyro[2];

  pressure.fluid_pressure = data.pressure;

  if (enableMagnetometer) {
    imu.mag.x = data.mag[0];
    imu.mag.y = data.mag[1];
    imu.mag.z = data.mag[2];
  }
  else {

    imu.mag.x = -1;
    imu.mag.y = -1;
    imu.mag.z = -1;

  }

  //  publish
  pubIMU.publish(imu);
  pubPressure.publish(pressure);

  char buffer[512];
  sprintf(buffer,"%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf",imu.header.stamp.toSec(),imu.ang.x,imu.ang.y,imu.ang.z,imu.acc.x,imu.acc.y,imu.acc.z,imu.mag.x,imu.mag.y,imu.mag.z,pressure.fluid_pressure);

  log_this_now(LOG_FID_MST_FORMAT, buffer);

  
  if (imuDiag) {
    imuDiag->tick(imu.header.stamp);
  }
}

void publishFilter(const Imu::FilterData &data) {
  assert(data.fields & Imu::FilterData::Quaternion);
  assert(data.fields & Imu::FilterData::Bias);
  assert(data.fields & Imu::FilterData::AngleUnertainty);
  assert(data.fields & Imu::FilterData::BiasUncertainty);
  
  dscl_msgs::FilterOutput output;
  output.header.stamp = ros::Time::now();
  output.header.frame_id = frameId;
  output.orientation.w = data.quaternion[0];
  output.orientation.x = data.quaternion[1];
  output.orientation.y = data.quaternion[2];
  output.orientation.z = data.quaternion[3];
  output.bias.x = data.bias[0];
  output.bias.y = data.bias[1];
  output.bias.z = data.bias[2];

  output.bias_covariance[0] = data.biasUncertainty[0]*data.biasUncertainty[0];
  output.bias_covariance[4] = data.biasUncertainty[1]*data.biasUncertainty[1];
  output.bias_covariance[8] = data.biasUncertainty[2]*data.biasUncertainty[2];
  
  output.orientation_covariance[0] = data.angleUncertainty[0]*
      data.angleUncertainty[0];
  output.orientation_covariance[4] = data.angleUncertainty[1]*
      data.angleUncertainty[1];
  output.orientation_covariance[8] = data.angleUncertainty[2]*
      data.angleUncertainty[2];
  
  output.quat_status = data.quaternionStatus;
  output.bias_status = data.biasStatus;
  output.orientation_covariance_status = data.angleUncertaintyStatus;
  output.bias_covariance_status = data.biasUncertaintyStatus;
  
  pubFilter.publish(output);
  if (filterDiag) {
    filterDiag->tick(output.header.stamp);
  }
}

std::shared_ptr<diagnostic_updater::TopicDiagnostic> configTopicDiagnostic(
    const std::string& name, double * target) {
  std::shared_ptr<diagnostic_updater::TopicDiagnostic> diag;
  const double period = 1.0 / *target;  //  for 1000Hz, period is 1e-3
  
  diagnostic_updater::FrequencyStatusParam freqParam(target, target, 0.01, 10);
  diagnostic_updater::TimeStampStatusParam timeParam(0, period * 0.5);
  diag.reset(new diagnostic_updater::TopicDiagnostic(name, 
                                                     *updater, 
                                                     freqParam,
                                                     timeParam));
  return diag;
}

void updateDiagnosticInfo(diagnostic_updater::DiagnosticStatusWrapper& stat,
                          imu_3dm_gx4::Imu* imu) {
  //  add base device info
  std::map<std::string,std::string> map = info.toMap();
  for (const std::pair<std::string,std::string>& p : map) {
    stat.add(p.first, p.second);
  }
  
  try {
    //  try to read diagnostic info
    imu->getDiagnosticInfo(fields, info);
    
    auto map = fields.toMap();
    for (const std::pair<std::string, unsigned int>& p : map) {
      stat.add(p.first, p.second);
    }
    stat.summary(diagnostic_msgs::DiagnosticStatus::OK, "Read diagnostic info.");
  }
  catch (std::exception& e) {
    const std::string message = std::string("Failed: ") + e.what();
    stat.summary(diagnostic_msgs::DiagnosticStatus::ERROR, message);
  }
}

int main(int argc, char **argv) {
  ros::init(argc, argv, "imu_3dm_gx4");
  ros::NodeHandle nh("~");

  std::string device;
  int baudrate;
  bool enableFilter;
  bool enableMagUpdate, enableAccelUpdate;
  int requestedImuRate, requestedFilterRate;
  bool verbose;
  
  //  load parameters from launch file
  nh.param<std::string>("device", device, "/dev/ttyACM0");
  nh.param<int>("baudrate", baudrate, 115200);
  nh.param<std::string>("frame_id", frameId, std::string("imu"));
  nh.param<int>("imu_rate", requestedImuRate, 100);
  nh.param<int>("filter_rate", requestedFilterRate, 100);
  nh.param<bool>("enable_magnetometer", enableMagnetometer, true);
  nh.param<bool>("enable_filter", enableFilter, false);
  nh.param<bool>("enable_mag_update", enableMagUpdate, false);
  nh.param<bool>("enable_accel_update", enableAccelUpdate, true);
  nh.param<bool>("verbose", verbose, false);
  
  if (requestedFilterRate < 0 || requestedImuRate < 0) {
    ROS_ERROR("imu_rate and filter_rate must be > 0");
    return -1;
  }
  
  pubIMU = nh.advertise<dscl_msgs::Imu9DOF>("imu", 1);
  pubPressure = nh.advertise<sensor_msgs::FluidPressure>("pressure", 1);

  if (enableFilter) {
    pubFilter = nh.advertise<dscl_msgs::FilterOutput>("filter", 1);
  }

  //  new instance of the IMU
  Imu imu(device, verbose);
  try {
    imu.connect();

    ROS_INFO("Selecting baud rate %u", baudrate);
    imu.selectBaudRate(baudrate);

    ROS_INFO("Fetching device info.");
    imu.getDeviceInfo(info);
    std::map<std::string,std::string> map = info.toMap();
    for (const std::pair<std::string,std::string>& p : map) {
      ROS_INFO("\t%s: %s", p.first.c_str(), p.second.c_str());
    }

    ROS_INFO("Idling the device");
    imu.idle();

    //  read back data rates
    uint16_t imuBaseRate, filterBaseRate;
    imu.getIMUDataBaseRate(imuBaseRate);
    ROS_INFO("IMU data base rate: %u Hz", imuBaseRate);
    imu.getFilterDataBaseRate(filterBaseRate);
    ROS_INFO("Filter data base rate: %u Hz", filterBaseRate);

    //  calculate decimation rates
    if (static_cast<uint16_t>(requestedImuRate) > imuBaseRate) {
      throw std::runtime_error("imu_rate cannot exceed " + 
                               std::to_string(imuBaseRate));
    }
    if (static_cast<uint16_t>(requestedFilterRate) > filterBaseRate) {
      throw std::runtime_error("filter_rate cannot exceed " + 
                               std::to_string(filterBaseRate));
    }
    
    const uint16_t imuDecimation = imuBaseRate / requestedImuRate;
    const uint16_t filterDecimation = filterBaseRate / requestedFilterRate;
    
    ROS_INFO("Selecting IMU decimation: %u", imuDecimation);
    std::bitset<4> imuSources = Imu::IMUData::Accelerometer |
                                Imu::IMUData::Gyroscope |
                                Imu::IMUData::Barometer;
    if (enableMagnetometer)
    {
      ROS_INFO("Enabling magnetometer");
      imuSources |= Imu::IMUData::Magnetometer;
    }
    else
    {
      ROS_INFO("Disabling magnetometer");
    }
    imu.setIMUDataRate(imuDecimation, imuSources);

    ROS_INFO("Selecting filter decimation: %u", filterDecimation);
    imu.setFilterDataRate(filterDecimation, Imu::FilterData::Quaternion |
                          Imu::FilterData::Bias |
                          Imu::FilterData::AngleUnertainty |
                          Imu::FilterData::BiasUncertainty);

    ROS_INFO("Enabling IMU data stream");
    imu.enableIMUStream(true);

    if (enableFilter) {
      ROS_INFO("Enabling filter data stream");
      imu.enableFilterStream(true);

      ROS_INFO("Enabling filter measurements");
      imu.enableMeasurements(enableAccelUpdate, enableMagUpdate);

      ROS_INFO("Enabling gyro bias estimation");
      imu.enableBiasEstimation(true);
    } else {
      ROS_INFO("Disabling filter data stream");
      imu.enableFilterStream(false);
    }
    imu.setIMUDataCallback(publishData);
    imu.setFilterDataCallback(publishFilter);

    //  configure diagnostic updater
    if (!nh.hasParam("diagnostic_period")) {
      nh.setParam("diagnostic_period", 0.2);  //  5hz period
    }
    
    updater.reset(new diagnostic_updater::Updater());
    const std::string hwId = info.modelName + "-" + info.modelNumber;
    updater->setHardwareID(hwId);
    
    //  calculate the actual rates we will get
    double imuRate = imuBaseRate / (1.0 * imuDecimation);
    double filterRate = filterBaseRate / (1.0 * filterDecimation);
    imuDiag = configTopicDiagnostic("imu",&imuRate);
    if (enableFilter) {
      filterDiag = configTopicDiagnostic("filter",&filterRate);
    }
    
    updater->add("diagnostic_info", 
                 boost::bind(&updateDiagnosticInfo, _1, &imu));
    
    ROS_INFO("Resuming the device");
    imu.resume();

    while (ros::ok()) {
      imu.runOnce();
      updater->update();
    }
    imu.disconnect();
  }
  catch (Imu::io_error &e) {
    ROS_ERROR("IO error: %s\n", e.what());
  }
  catch (Imu::timeout_error &e) {
    ROS_ERROR("Timeout: %s\n", e.what());
  }
  catch (std::exception &e) {
    ROS_ERROR("Exception: %s\n", e.what());
  }

  return 0;
}
