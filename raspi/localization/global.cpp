#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <signal.h>

#include "common_utils.h"
#include "display.h"
#include "global.h"
#include "keypad_handler.h"
#include "log.h"
#include "shutdown_exception.h"
#include "spi_manager.h"

namespace wins {

using namespace std;

string Global::MapFile = "/home/pi/wins/raspi/localization/data/"
    "ee_ab_map_8lin.dat";

WinsEvent Global::event_flags_;
condition_variable Global::display_event_pending_;
mutex Global::event_mutex_;
thread::id Global::mainthread_id_;

int Global::FilterableDistance = 10;
int Global::FilterBiasX = 5;
int Global::FilterBiasY = 5;
int Global::LocationRFactor = 20;
int Global::LocationQFactor = 25;
double Global::IMU_R = 0.9;
double Global::IMU_QD = 10;
double Global::IMU_QV = 100;
double Global::IMU_QA = 1000;
double Global::IMU_ACC_SCALE = 1;
double Global::IMU_X_Correction = 180;
double Global::IMU_Y_Correction = 0;
double Global::IMU_Z_Correction = 0;
double Global::IMU_DELTA_T = 0.02;
double Global::WiFiExp1 = 0;
double Global::WiFiExp2 = 0;
WiFiVariant Global::ScanVariant = WIFI_VARIANT_NONE;
bool Global::NoSleep = false;
int Global::DurationOverride = -1;
vector<string> Global::WiFiDevices({ "wlan0" });
int Global::InitWiFiReadings = 3;
int Global::ReadingsPerUpdate = 1;
double Global::Scale = (1/1.1);
atomic_bool Global::DataDump(false);
string Global::DumpFile = "dump.txt";
mutex Global::DumpMutex;
string Global::shutdown_command_ = "sudo shutdown -hP now";
bool Global::is_test_ = false;

void sighandler(int sig)
{
  FILE_LOG(logCRITICAL) << "Signal received. Initiating clean shut down...\n";
  Global::SetTestMode();
  Global::SetEventFlag(WINS_EVENT_SHUTTING_DOWN);
}

void Global::RunMainLoop() {
  Global::Init();

  Display& display = Display::GetInstance();
  Page curpage;
  Page nextpage = display.FirstPage();

  try {
    while(nextpage != PAGE_SHUT_DOWN) {
      // Just to be clear.
      curpage = nextpage;

      nextpage = display.ShowPage(curpage);
    }
  } catch(ShutDownException) {}
  display.ShowPage(PAGE_SHUT_DOWN);
  Global::ShutDown();
  Global::SetEventFlag(WINS_EVENT_SHUTDOWN_DONE);
  exit(0);
}

void Global::Init() {
  mainthread_id_ = this_thread::get_id();
  FILELog::LogSelect() = (TLogLevel)(logLOCATION | logIMU);
  signal(SIGABRT, &sighandler);
  signal(SIGTERM, &sighandler);
  signal(SIGINT, &sighandler);

  SPI::StartThread();
  KeypadHandler::StartThread();
  Map::StartNavigationThread();
}

void Global::Destroy() {
}

void Global::SetEventFlag(WinsEvent flag) {
  lock_guard<mutex> lock(event_mutex_);
  event_flags_ = (WinsEvent)(event_flags_ | flag);
  display_event_pending_.notify_all();
}

bool Global::IsFlagSet(WinsEvent flag) {
  return event_flags_ & flag;
}

BlockResult Global::BlockForEvent(WinsEvent type, int millis) {
  if (event_flags_ & type) {
    WinsEvent flags = event_flags_;
    if (not IsFlagSet(WINS_EVENT_SHUTTING_DOWN)) {
      event_flags_ = (WinsEvent)(event_flags_ & ~type);
    }
    return { cv_status::no_timeout, flags };
  }

  auto until = chrono::system_clock::now() + chrono::milliseconds(millis);
  cv_status result = cv_status::no_timeout;
  while (not(event_flags_ & type)) {
    unique_lock<mutex> lock(event_mutex_);
    if (millis >= 0) {
      result = display_event_pending_.wait_until(lock, until);
    } else {
      display_event_pending_.wait(lock);
    }
    lock.unlock();
    if (event_flags_ & WINS_EVENT_SHUTTING_DOWN and
        this_thread::get_id() == Global::GetMainThreadId()) {
      throw ShutDownException();
    }

    if (result == cv_status::timeout) {
      return { result, event_flags_ };
    }
  }
  WinsEvent flags = event_flags_;
  if (not IsFlagSet(WINS_EVENT_SHUTTING_DOWN)) {
    event_flags_ = (WinsEvent)(event_flags_ & ~type);
  }
  return { cv_status::no_timeout, flags };
}

thread::id Global::GetMainThreadId() {
  return mainthread_id_;
}

void Global::ShutDown() {
  cout << "IN Global::ShutDown ========================\n";
  cout << "is test " << is_test_ << "\n";
  if (is_test_ == false) {
    cout << "Shutting down...\n";
    system("sudo kill -9 $(pgrep -f winsd)");
    cout << "shutdown command: " << shutdown_command_ << "\n";
    system(shutdown_command_.c_str());
  }
  SetEventFlag(WINS_EVENT_SHUTTING_DOWN);
  Global::Destroy();
  KeypadHandler::TerminateThread();
  Map::TerminateThread();
  cout << "Teriminated threads______________\n";
  SPI::TerminateThread();
}

void Global::SetTestMode() {
  shutdown_command_ = ":";
  is_test_ = true;
}

bool Global::IsTest() {
  return is_test_;
}

}
