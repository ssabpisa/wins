#include <functional>
#include <future>
#include <stdexcept>
#include <type_traits>

#include "global.h"
#include "imu.h"
#include "log.h"
#include "kalman.h"
#include "location.h"
#include "wifi_estimate.h"

namespace wins {

using namespace std;
using namespace Eigen;

#define HIGH_VARIANCE 10000

Eigen::MatrixXd Location::prev_X;
Eigen::MatrixXd Location::P;
Eigen::MatrixXd Location::A;
Eigen::MatrixXd Location::A_t;
Eigen::MatrixXd Location::Q;
Eigen::MatrixXd Location::const_R;

vector<unique_ptr<WiFiEstimate>> Location::wifi_estimators_;
PointEstimate Location::point_estimate_;
kdtree::node<Point*>* Location::current_node_ = nullptr;
LocationVariant Location::variant_ = (LocationVariant)
    (LOCATION_VARIANT_FIXED_R | LOCATION_VARIANT_UPDATE_IMU);
std::chrono::time_point<std::chrono::steady_clock> Location::last_update_time_;

vector<int> default_channels =
    { 1, 6, 11, 48, 149, 36, 40, 157, 44, 153, 161 };

vector<PointEstimate> Location::GetWiFiReadings(int count) {
  vector<PointEstimate> estimates;
  vector<future<PointEstimate>> handles;
  for (auto& estimator : wifi_estimators_) {
    auto handle = async(launch::async, &WiFiEstimate::EstimateLocation,
          estimator.get(), count, WIFI_VARIANT_NONE);
    handles.push_back(move(handle));
  }
  for (auto& result : handles) {
    estimates.push_back(result.get());
  }
  return estimates;
}

void Location::InitialEstimate() {
  auto estimates = GetWiFiReadings(Global::InitWiFiReadings);
  DoKalmanUpdate(false, estimates);
}

void Location::InitKalman() {
  A = MatrixXd::Zero(2,2);
  A_t = MatrixXd::Zero(2,2);
  P = HIGH_VARIANCE * MatrixXd::Identity(2,2);
}

void Location::Init() {
  InitKalman();
  for (auto device : Global::WiFiDevices) {
    wifi_estimators_.push_back(unique_ptr<WiFiEstimate>(
        new WiFiEstimate(unique_ptr<WifiScan>(
        new WifiScan(default_channels, device)))));
  }
  InitialEstimate();
}

void Location::DoKalmanUpdate(bool imu_valid, vector<PointEstimate>
    wifi_estimates, PointEstimate imu_estimate) {

  if (wifi_estimates.size() == 0) {
    FILE_LOG(logWARNING) << "No WIFI Estimates.";
    this_thread::sleep_for(chrono::seconds(1));
    return;
  }

  MatrixXd X(2,2);
  MatrixXd Z(wifi_estimates.size() * 2, 1);
  MatrixXd H(wifi_estimates.size() * 2, 2);
  MatrixXd H_t(2, wifi_estimates.size() * 2);
  MatrixXd R(wifi_estimates.size() * 2, wifi_estimates.size() * 2);

  if (imu_valid) {
    X = Imu::X.block<2,1>(0,0);
  } else {
    X = prev_X;
  }

  R.setConstant(HIGH_VARIANCE);
  for (size_t i = 0; i < wifi_estimates.size(); ++i) {
    Z(i*2, 0) = wifi_estimates[i].x_mean;
    Z(i*2+1, 0) = wifi_estimates[i].y_mean;
    H.block<2,2>(i*2, 0) << 1, 0,
                            0, 1;
    if (variant_ & LOCATION_VARIANT_FIXED_R) {
      R(i*2, i*2) = const_R(0);
      R(i*2+1, i*2+1) = const_R(1);
    } else {
      R(i*2, i*2) = wifi_estimates[i].x_var;
      R(i*2+1, i*2+1) = wifi_estimates[i].y_var;
    }
  }
  H_t = H.transpose();

  KalmanUpdate(X, P, Z, A, A_t, H, H_t, R, Q);

  if (variant_ & LOCATION_VARIANT_UPDATE_IMU) {
    Vector2d V = (X - prev_X) /
        chrono::duration_cast<std::chrono::milliseconds>(
        chrono::steady_clock::now() - last_update_time_).count();
    X.block<2,1>(2,0) = V;
    X.block<2,1>(0,0) = X;
  }
  last_update_time_ = chrono::steady_clock::now();
}

kdtree::node<Point*>* Location::GetCurrentNode() {
  return current_node_;
}

void Location::UpdateEstimate() {
  auto wifi_estimates = GetWiFiReadings(Global::InitWiFiReadings);
  auto imu_estimate = Imu::EstimateLocation();
  DoKalmanUpdate(true, wifi_estimates, imu_estimate);
}

}
