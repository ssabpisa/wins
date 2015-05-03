#include <fstream>
#include <thread>

#include "global.h"
#include "imu.h"
#include "kalman.h"
#include "log.h"

namespace wins {

using namespace Eigen;

#define HIGH_VARIANCE 10000
#define ACC_PRECISION 0
#define GYRO_PRECISION 0

namespace {
  double var(vector<double> v) {
    std::vector<double> diff(v.size());
    std::transform(v.begin(), v.end(), diff.begin(),
                   std::bind2nd(std::minus<double>(), mean(v)));
    double sq_sum = std::inner_product(diff.begin(), diff.end(),
        diff.begin(), 0.0);
    double var = (sq_sum / v.size());
    return var;
  }

  void ParseIMU(vector<uint8_t> pic_data, Vector3d& acc,
      Quaternion<double>& quat) {
    acc(0) = ACC_PRECISION * pic_data[0] * 256.0 + pic_data[1];
    acc(1) = ACC_PRECISION * pic_data[2] * 256.0 + pic_data[3];
    acc(2) = ACC_PRECISION * pic_data[4] * 256.0 + pic_data[5];
    quat.w() = GYRO_PRECISION * pic_data[6] * 256.0 + pic_data[7];
    quat.x() = GYRO_PRECISION * pic_data[8] * 256.0 + pic_data[9];
    quat.y() = GYRO_PRECISION * pic_data[10] * 256.0 + pic_data[11];
    quat.z() = GYRO_PRECISION * pic_data[12] * 256.0 + pic_data[13];
    quat.normalize();
  }
}

MatrixXd Imu::X(SVARS, 1);
MatrixXd Imu::P(SVARS, SVARS);
MatrixXd Imu::R(OBSERVATIONS, OBSERVATIONS);
MatrixXd Imu::Q(SVARS, SVARS);
MatrixXd Imu::H(OBSERVATIONS, SVARS);
MatrixXd Imu::H_t(SVARS, OBSERVATIONS);

vector<double> Imu::current_state;
vector<double> Imu::current_variance;
vector<double> Imu::sensor_variance;
vector<double> Imu::process_variance;

ImuResult Imu::imu_buffer_;
mutex Imu::imu_buffer_mutex_;
atomic_bool Imu::calibrated_(false);
Quaternion<double> Imu::north_quat_inverse_ = Quaternion<double>::Identity();

void Imu::Init() {
  // Observation matrix.
  H.setZero();
  for (int i = 0; i < IMUS; ++i) {
    H(i * 2, 4) = 1;
    H(i * 2 + 1, 5) = 1;
  }
  H_t = H.transpose();

  X.setZero();
  P = HIGH_VARIANCE * MatrixXd::Identity(SVARS, SVARS);

  // Velocity is zero at start.
  P(1,1) = 0;
  P(2,2) = 0;

  R = MatrixXd::Identity(OBSERVATIONS, OBSERVATIONS);
  R(0,0) = Global::IMU_R;
  R(1,1) = Global::IMU_R;
  Q = MatrixXd::Identity(SVARS, SVARS);
  Q(0,0) = Global::IMU_QD;
  Q(1,1) = Global::IMU_QD;
  Q(2,2) = Global::IMU_QV;
  Q(3,3) = Global::IMU_QV;
  Q(4,4) = Global::IMU_QA;
  Q(5,5) = Global::IMU_QA;
}

void Imu::AddReading(double ax, double ay, double az,
    double qw, double qx, double qy, double qz) {
//  Vector3d raw_acc;
//  Quaternion<double> quat;
//  ParseIMU(pic_data, raw_acc, quat);
//
//  // Rotate acceleration vector to the default orientation of the gyro first
//  // (YPR all zeroes), and then rotate it north orientation.
//  Matrix3d rot_matrix = (quat * north_quat_inverse_).toRotationMatrix();
//  Vector3d acc = rot_matrix * raw_acc;
//
  lock_guard<mutex> lock(imu_buffer_mutex_);
  //FILE_LOG(logIMU) << "Adding reading ax: " << ax
  //                              << ", ay: " << ay
  //                              << ", az: " << az
  //                              << ", qw: " << qw
  //                              << ", qx: " << qx
  //                              << ", qy: " << qy
  //                              << ", qz: " << qz << "\n";
  imu_buffer_.readings.push_back({ ax, ay, az, qw, qx, qy, qz });
  //FILE_LOG(logIMU) << "Buffer size: " << imu_buffer_.readings.size() << "\n";
  if (Global::DataDump.load() and calibrated_) {
    lock_guard<mutex> lock(Global::DumpMutex);
    ofstream dumpfile(Global::DumpFile, ofstream::app);
    dumpfile << "IMU," << ax << "," << ay << "," << az << ","
             << qw << "," << qx << "," << qy << "," << qz << "\n";
    dumpfile.close();
  }
}

void Imu::Calibrate() {
  calibrated_ = false;

  // Collect imu readings for 1 sec.
  this_thread::sleep_for(chrono::seconds(1));

  // Average IMU readings.
  Vector4d vals = Vector4d::Zero();
  lock_guard<mutex> lock(imu_buffer_mutex_);
  for (auto r : imu_buffer_.readings) {
    vals(0) += r[3];
    vals(1) += r[4];
    vals(2) += r[5];
    vals(3) += r[6];
  }
  vals /= imu_buffer_.readings.size();

  // Set the average value as the quat representing north orentation.
  north_quat_inverse_ = Quaternion<double>(vals(0), vals(1), vals(2), vals(3))
      .inverse();

  // Clear IMU buffer and mark as calibrated.
  imu_buffer_.readings.clear();
  calibrated_ = true;
}

vector<double> Imu::RelativeToNorth(double w, double x, double y, double z) {
  Quaternion<double> q(w, x, y, z);
  Quaternion<double> qn = north_quat_inverse_ * q;
  return { qn.w(), qn.x(), qn.y(), qn.z() };
}

PointEstimate Imu::DoKalman(const ImuResult& imu_result, double duration,
    ImuVariant v) {
  vector<double> vals_x;
  vector<double> vals_y;
  for (auto r : imu_result.readings) {
    vals_x.push_back(r[0]);
    vals_y.push_back(r[1]);
    //FILE_LOG(logIMU) << "(" << r[0] << ", " << r[1] << ")|";
  }
  auto var_x = var(vals_x);
  auto var_y = var(vals_y);

  auto delta_t = duration / imu_result.readings.size();

  Matrix<double, SVARS, 1> x_sum = X;
  Matrix<double, SVARS, SVARS> p_sum = P;
  Matrix<double, OBSERVATIONS, 1> z_sum;
  z_sum.setZero();

  auto X_initial = X;

  // State transition matrix.
  Matrix<double, SVARS, SVARS> A;
  A << 1, 0, delta_t, 0, delta_t * delta_t / 2, 0,
       0, 1, 0, delta_t, 0, delta_t * delta_t / 2,
       0, 0, 1, 0, delta_t, 0,
       0, 0, 0, 1, 0, delta_t,
       0, 0, 0, 0, 1, 0,
       0, 0, 0, 0, 0, 1;
  Matrix<double, SVARS, SVARS> A_t = A.transpose();

  Eigen::IOFormat CleanFmt(4, 0, ", ", "\n", "[", "]");
  FILE_LOG(logIMU) << "-------------------------------";
  FILE_LOG(logIMU) << "var_x: " << var_x << "\n";
  FILE_LOG(logIMU) << "var_y: " << var_y << "\n";
  FILE_LOG(logIMU) << "delta_t: " << delta_t << "\n";
  FILE_LOG(logIMU) << "X before:\n" << X.format(CleanFmt) << "\n";
  FILE_LOG(logIMU) << "P before:\n" << P.format(CleanFmt) << "\n";
  for (auto& reading : imu_result.readings) {
    Matrix<double, OBSERVATIONS, 1> Z;

    // Copy X and Y acc values to measurement vector Z.
    for (int i = 0; i < OBSERVATIONS; ++i) {
      Z(i, 0) = reading[i];
    }

    KalmanUpdate(X, P, Z, A, A_t, H, H_t, R, Q);
    //FILE_LOG(logIMU) << "P intermediate:\n" << P.format(CleanFmt) << "\n";
    x_sum += X;
    p_sum += P;
    z_sum += Z;
  }
  auto average_state = x_sum.array() *
      1.0 / (imu_result.readings.size() + 1);
  auto average_error = p_sum.array() *
      1.0 / (imu_result.readings.size() + 1);
  auto average_z = z_sum.array() *
      1.0 / (imu_result.readings.size() + 1);
  FILE_LOG(logIMU) << "Average X:\n" << average_state.format(CleanFmt) << "\n";
  FILE_LOG(logIMU) << "Average Z:\n" << average_z.format(CleanFmt) << "\n";
  FILE_LOG(logIMU) << "P after:\n" << P.format(CleanFmt) << "\n";

  if (v == IMU_VARIANT_KALMAN_DISTANCE_AVG) {
    X.block<2,1>(0,0) = average_state.block<2,1>(0,0);
    P.block<2,2>(0,0) = average_error.block<2,2>(0,0);
  } else if (v == IMU_VARIANT_KALMAN_ALL_AVERAGE) {
    X = average_state;
    P = average_error;
  } else if (v == IMU_VARIANT_KALMAN_VELOCITY_AVG) {
    X.block<2,1>(0,0) = X_initial.block<2,1>(0,0).array() +
        average_state.block<2,1>(2,0).array() * duration;
    X.block<2,1>(2,0) = average_state.block<2,1>(2,0);
    P.block<4,4>(0,0) = average_error.block<4,4>(0,0);
  }
  FILE_LOG(logIMU) << "X after:\n" << X.format(CleanFmt) << "\n";
  FILE_LOG(logIMU) << "P fixed:\n" << P.format(CleanFmt) << "\n";

  return { X(0,0), P(0,0), X(1,0), P(1,1) };
}

PointEstimate Imu::EstimateLocation(double duration, ImuVariant v) {
  // If not calibrated don't use IMU.
  if (not calibrated_) {
    return { X(0,0), P(0,0), X(1,0), P(1,1) };
  }

  ImuResult results;

  // Create deep copy of imu_buffer_.
  {
    lock_guard<mutex> lock(imu_buffer_mutex_);
    results = imu_buffer_;
    FILE_LOG(logIMU) << "Buffer size: " << results.readings.size();
    imu_buffer_.readings.clear();
    FILE_LOG(logIMU) << "Clearing buffer on estimation\n";
  }
  return DoKalman(results, duration, v);
}
/*
PointEstimate Imu::EstimateLocation1(PointEstimate current) {
  auto imu_result = FetchAll();
  vector<double> sum(0, IMUS * 4);
  for (auto& reading : imu_result.readings) {
    // Predict how much error.
    auto variance_prediction = current_variance + process_variance;

    // Compare readings against prediction.
    auto innovation = reading - current_state;

    // Compare real error against prediction.
    auto innovation_variance = variance_prediction + sensor_variance;

    // Moderate the prediction.
    auto kalman_gain = variance_prediction / innovation_variance;

    current_state = current_state + kalman_gain * innovation;
    current_variance = (vector<double>(0, STATE_SIZE) - kalman_gain) *
                        variance_prediction;
  }
  auto average_state = sum / vector<double>(imu_result.readings.size(),
      STATE_SIZE);
} */

}
