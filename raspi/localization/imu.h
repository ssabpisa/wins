#ifndef IMU_H
#define IMU_H

#include <Eigen/Dense>

#include "common_utils.h"

namespace wins {

#define IMUS 1
#define READINGS 100
#define STATE_SIZE (IMUS * 4)
#define SVARS (IMUS * 6)

enum ImuVariant {
  IMU_VARIANT_KALMAN_VANILLA,
  IMU_VARIANT_KALMAN_DISTANCE_AVG,
  IMU_VARIANT_KALMAN_VELOCITY_AVG,
  IMU_VARIANT_KALMAN_ALL_AVERAGE
};


struct ImuResult {
  vector<vector<double>> readings;
};

class Imu{
 private:
  static Eigen::MatrixXd R;  // Estimated sensor
  static Eigen::MatrixXd Q;  // Estimated process error covariance.

  static Eigen::MatrixXd H;
  static Eigen::MatrixXd H_t;

  static vector<double> current_state;
  static vector<double> current_variance;
  static vector<double> sensor_variance;
  static vector<double> process_variance;

  static ImuResult imu_buffer_;
  static mutex imu_buffer_mutex_;
  static atomic_bool calibrated_;
  static Eigen::Quaternion<double> north_quat_inverse_;
  static double initial_yaw_;

 public:
  static Eigen::MatrixXd X;      // Current state estimate.
  static Eigen::MatrixXd P;  // Current covariance estimate.

  static void AddReading(double ax, double ay, double az,
      double qw, double qx, double qy, double qz);
  static void Calibrate();
  static vector<double> RelativeToNorth(double w, double x, double y, double z);
  static Eigen::Quaternion<double> GetNorthQuat();
  static void SetNorthQuat(double w, double x, double y, double z);
  static void Init();
  static PointEstimate DoKalman(const ImuResult& imu_result,
      double duration, ImuVariant v = IMU_VARIANT_KALMAN_VELOCITY_AVG);
  static PointEstimate EstimateLocation(double duration,
      ImuVariant v = IMU_VARIANT_KALMAN_VELOCITY_AVG);
  // static PointEstimate EstimateLocation1(PointEstimate current);
};

}

#endif // IMU_H
