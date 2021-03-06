#include "test.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>

#include "cereal/archives/binary.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/vector.hpp"
#include "common_utils.h"
#include "display.h"
#include "global.h"
#include "imu.h"
#include "kdtree/kdtree.hpp"
#include "keypad_handler.h"
#include "location.h"
#include "test_helpers.h"
#include "map.h"
#include "navigation.h"
#include "point.h"
#include "scan_result.h"
#include "gamma.hpp"
#include "wifi_estimate.h"

#include <cassert>

namespace wins {

void Test(int argc, char *orig_argv[]) {
  vector<string> argv(orig_argv, orig_argv + argc);

  if (string(argv[2]) == "make_binary") {
    assert(argc == 5);
    Map::TryConvertJSONMap(argv[3], argv[4]);
  } else if (string(argv[2]) == "data") {
    assert(argc == 5);
    Map::InitMap(argv[3]);
    ofstream out_file;
    out_file.open("results.csv");

    vector<unique_ptr<Point>> test_points;
    ifstream is(argv[4], ios::binary);
    cereal::BinaryInputArchive archive(is);

    archive(test_points);
    is.close();

    auto w = WiFiEstimate();
    int count = 0;
    for (auto&& point : test_points) {
      printf("Ground Truth: %2.0f, %2.0f\n", point->x, point->y);
      for (auto&& scan : point->scans) {
        out_file << to_string(point->x) + "," + to_string(point->y) + ",";
        count += 1;
        if (count == 15) {
          cout <<15;
        }
        for (auto&& estimate : w.ClosestByMahalanobis(
            scan, (WiFiVariant)(WIFI_VARIANT_TOP1),
            point->x, point->y)) {
        //for (auto&& estimate : WiFiEstimate::MostProbableNotClubbed(scan)) {
          out_file << estimate.to_string();
          out_file << ",";
        }
        /*
        out_file << WifiEstimate::ClosestByMahalanobis(
            &scan, WIFI_VARIANT_CHI_SQ).to_string();
        out_file << ",";
        out_file << WifiEstimate::ClosestByMahalanobis(
            &scan, WIFI_VARIANT_NONE).to_string();
        out_file << ",";
        PointEstimate p2 = WifiEstimate::MostProbableClubbed(scan);
        out_file << p2.to_string();
        out_file << ",";
        PointEstimate p3 = WifiEstimate::MostProbableNotClubbed(scan);
        out_file << p3.to_string();
        */
        out_file << "\n";
      }
    }
    out_file.close();
  }
  else if (string(argv[2]) == "sample") {
      vector<unique_ptr<Point>> points;
      points.emplace_back(unique_ptr<Point>(new Point {10, 0, {1,1,1,1},
        {{"mac11", {100, 90}}, {"mac12", {150, 95}}},
        {{{"mac11", 60.0}, {"mac12", 70.0}},
         {{"mac11", 65.0}, {"mac12", 75.0}}}, 1, 1}));
      points.emplace_back(unique_ptr<Point>(new Point { 9, 1, {1,1,1,1},
        {{"mac11", {110, 91}}, {"mac12", {160, 96}}},
        {{{"mac11", 60.0}, {"mac12", 70.0}},
         {{"mac11", 65.0}, {"mac12", 75.0}}}, 1, 1}));

      ofstream os("sample.json");
      cereal::JSONOutputArchive archive(os);

      archive(points);
  }
  else if (string(argv[2]) == "math") {
    cout << "chi(2, 15) = " << pchisq(2, 15) << "\n";
    cout << "chi(15, 15) = " << pchisq(15, 15) << "\n";
    cout << "chi(15, 2) = " << pchisq(15, 2) << "\n";
    cout << "dnorm(1, 1, 1) = " << dnorm(0, 0, 1) << "\n";
    cout << "pnorm1(1, 0, 0) = " << pnorm1(0, false, false) << "\n";
    cout << "pnorm1(1, 0, 0) = " << pnorm1(0, false, true) << "\n";
    cout << "pnorm1(1, 0, 0) = " << pnorm1(0, true, true) << "\n";
    cout << "pnorm1(1, 0, 0) = " << pnorm1(0, true, true) << "\n";
  }
  else if (string(argv[2]) == "wifi") {
    std::vector<int> EEchan = { 1, 6, 11, 48, 149, 36, 40, 157, 44, 153,161};

    WifiScan w(EEchan, "wlp2s0");
    vector<Result> results = w.Fetch();

    for(auto& kv : results) {
      std::cout << kv.name << " = " << kv.signal << std::endl;
    }
  }
  else if (string(argv[2]) == "learn") {
    learn_helper(argc, argv);
  }
  else if (string(argv[2]) == "learn_all") {
    argc += 1;
    argv.push_back("-");
    remove("analysis_summary.csv");
    for (int i = 0; i < 12; ++i) {
      argv[5] = std::to_string(i);
      learn_helper(argc, argv);
    }
  }
  else if (string(argv[2]) == "learn_filter") {
    assert(argc == 9);
    Global::FilterableDistance = stoi(argv[6]);
    Global::FilterBiasX = stoi(argv[7]);
    Global::FilterBiasY = stoi(argv[8]);
    argc = 6;
    learn_helper(argc, argv);
  }
  else if (string(argv[2]) == "learn_fall") {
    assert(argc == 8);
    Global::FilterableDistance = stoi(argv[5]);
    Global::FilterBiasX = stoi(argv[6]);
    Global::FilterBiasY = stoi(argv[7]);
    argc = 6;
    remove("analysis_summary.csv");
    for (int i = 0; i < 12; ++i) {
      argv[5] = std::to_string(i);
      learn_helper(argc, argv);
    }
  }
  else if (string(argv[2]) == "learn_fdebug") {
    assert(argc == 8);
    Global::FilterableDistance = stoi(argv[5]);
    Global::FilterBiasX = stoi(argv[6]);
    Global::FilterBiasY = stoi(argv[7]);
    argc = 6;
    remove("analysis_summary.csv");
    for (int i = 6; i < 12; ++i) {
      argv[5] = std::to_string(i);
      learn_helper(argc, argv);
    }
  }
  else if (string(argv[2]) == "learn_debug") {
    argc += 1;
    argv.push_back("-");
    remove("analysis_summary.csv");
    for (int i = 6; i < 12; ++i) {
      argv[5] = std::to_string(i);
      learn_helper(argc, argv);
    }
  }
  else if (string(argv[2]) == "learn_good") {
    argc += 1;
    argv.push_back("-");
    argv[5] = "1";
    learn_helper(argc, argv);
    argv[5] = "3";
    learn_helper(argc, argv);
    argv[5] = "7";
    learn_helper(argc, argv);
    argv[5] = "9";
    learn_helper(argc, argv);
  }
  else if (string(argv[2]) == "llocs_all") {
    argc += 1;
    argv.push_back("-");
    for (int i = 20; i < 24; ++i) {
      argv[5] = std::to_string(i);
      learn_helper(argc, argv);
    }
  }
  else if (string(argv[2]) == "nav") {
    vector<unique_ptr<Point>> points;
    for (double i = 0; i < 10; ++i) {
      for (double j = 0; j < 10; ++j) {
        points.push_back(unique_ptr<Point>(new Point({i, j})));
      }
    }
    Map::TestInitMap(move(points));
    Navigation::TrySetDestinationFromCoords("9.0, 9.0");
    Location::TestSetCurrentNode(Map::NodeNearest(0, 0));
    Navigation::UpdateRoute();
    for (auto iter = Navigation::route_begin(); iter != Navigation::route_end();
        ++iter) {
      printf("%3.0f %3.0f\n", (*iter)->point->x, (*iter)->point->y);
    }
  }
  else if (string(argv[2]) == "full") {
    string file_name = "Menu.bmp";
    std::ofstream stream(file_name.c_str(),std::ios::binary);
    if (!stream) {
      std::cout << "bitmap_image::save_image(): Error - Could not open file "
          << file_name << " for writing!" << std::endl;
      return;
    }
    stream.close();
    assert(argc == 4);
    Global::MapFile = string(argv[3]);
    thread main_thread = thread(&Global::RunMainLoop);
    //auto& display = Display::GetInstance();
    auto& keypad_handler = KeypadHandler::GetInstance();

    //while(display.CurrentPage() != PAGE_MENU);
    //this_thread::sleep_for(chrono::seconds(1));

    // Save main menu.
    //display.SaveAsBitmap("Menu.bmp");

    while (true) {
      string line;
      getline(cin, line);
      if (line.size() == 0) {
         continue;
      }
      keypad_handler.FakeStringEnter(line);
    }
  } else if (string(argv[2]) == "imu") {
    assert(argc == 8);
    Global::IMU_R = stoi(argv[4]);
    Global::IMU_QD = stoi(argv[5]);
    Global::IMU_QV = stoi(argv[6]);
    Global::IMU_QA = stoi(argv[7]);

    Global::MapFile = string(argv[3]);
    thread main_thread = thread(&Global::RunMainLoop);
    auto& keypad_handler = KeypadHandler::GetInstance();
    while (true) {
      string line;
      getline(cin, line);
      if (line.size() == 0) {
         continue;
      }
      keypad_handler.FakeStringEnter(line);
    }
//  } else if (string(argv[2]) == "data_dump") {
//    assert(argc == 5);
//    Global::ShouldDumpData = true;
//    Global::DumpFile = string(argv[4]);
//
//    Global::MapFile = string(argv[3]);
//    thread main_thread = thread(&Global::RunMainLoop);
//    auto& keypad_handler = KeypadHandler::GetInstance();
//    while (true) {
//      string line;
//      getline(cin, line);
//      if (line.size() == 0) {
//         continue;
//      }
//      keypad_handler.FakeStringEnter(line);
//    }
  } else if (string(argv[2]) == "learn_imu") {
    assert(argc == 7);
    Map::InitMap(argv[3]);

    string line;
    char buffer[200];
    sprintf(buffer, "%7s %7s %7s %7s %7s %7s %7s %7s\n",
        "mean", "std", "xrot", "yrot", "zrot", "accsc", "deltat", "imur");
    cout << buffer;
    Global::NoSleep = true;
    Imu::Calibrate();
    vector<vector<double>> results;
    for (double zrot = 0; zrot < 360; zrot += 90) {
      for (double xrot = 0; xrot < 360; xrot += 90) {
        for (double yrot = 0; yrot < 360; yrot += 90) {
          for (double imuscale = 1; imuscale <= 50; imuscale += 20) {
            for (double deltat = 0.02; deltat <= 0.02; deltat += 0.03) {
              for (double imur = 0.7; imur < 1; imur += 0.2) {
                Global::InitWiFiReadings = 0;

                Global::IMU_X_Correction = xrot * M_PI / 180;
                Global::IMU_Y_Correction = yrot * M_PI / 180;
                Global::IMU_Z_Correction = zrot * M_PI / 180;
                Global::IMU_R = imur;
                Global::IMU_ACC_SCALE = imuscale;
                Global::IMU_DELTA_T = deltat;
                Imu::Init();

                ifstream data_file(argv[4]);
                getline (data_file, line);
                auto parts = split(line, ',');
                double imu_reads = stoi(parts[0]);
                double max_x = stod(parts[1]);
                double max_y = stod(parts[2]);
                int cur_reading = 0;
                vector<double> distances;

                if (argv[6][0] == 'r') {
                  Imu::X(0,0) = -2;
                  Imu::X(1,0) = 44;
                } else {
                  Imu::X(0,0) = -2;
                  Imu::X(1,0) = 15;
                }
                while (data_file) {
                  cur_reading += AddNextSet(data_file, NULL);
                  Imu::EstimateLocation(0.5);
                  double x;
                  double y;
                  if (argv[6][0] == 'r') {
                    x = -2 - (cur_reading / imu_reads) * max_x;
                    y = 44 - (cur_reading / imu_reads) * max_y;
                  } else {
                    x = -2 + (cur_reading / imu_reads) * max_x;
                    y = 15 + (cur_reading / imu_reads) * max_y;
                  }
                  //cout << Imu::X(0,0) << ", " << Imu::X(1,0) << "\n";
                  //cout << cur_reading << ", " << imu_reads << ", " << max_x;
                  //cout << x << ", " << y << "\n";
                  distances.push_back(sqrt(pow(Imu::X(0,0) - x, 2) +
                      pow(Imu::X(1,0) - y, 2)));
                }
                sprintf(buffer, "%7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.3f "
                    "%7.4f\n",
                    mean(distances), stddev(distances),
                    xrot, yrot, zrot, imuscale, deltat, imur);
                results.push_back(vector<double>({mean(distances),
                      stddev(distances), xrot, yrot, zrot, imuscale,
                      deltat, imur}));
                cout << buffer;
              }
            }
          }
        }
      }
    }
    sort(results.begin(), results.end(),
        [](const vector<double> &a, const vector<double> &b) -> bool {
            return (a[0] + a[1]) < (b[0] + b[1]);
        });
    ofstream out_file(argv[5]);
    sprintf(buffer, "%7s %7s %7s %7s %7s %7s %7s %7s\n",
        "mean", "std", "xrot", "yrot", "zrot", "accsc", "deltat", "imur");
    out_file << buffer;
    for (auto& r : results) {
      sprintf(buffer, "%7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.4f\n",
          r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
      out_file << buffer;
    }
    out_file.close();
  } else if (string(argv[2]) == "walk_learn") {
    assert(argc == 7);
    Map::InitMap(argv[3]);

    string line;
    char buffer[200];
    sprintf(buffer, "%7s %7s %7s %7s %7s\n",
        "mean", "std", "qf", "wexp1", "wexp2");
    cout << buffer;

    vector<vector<double>> results;

    Global::NoSleep = true;
    Global::DurationOverride = 1000;
    Imu::Calibrate();

    double rfactor = 20;
    for (double qfactor = 5; qfactor <= 30; qfactor += 5) {
      for (double wexp1 = 2; wexp1 <= 10; wexp1 += 4) {
        for (double wexp2 = 0; wexp2 <= 10; wexp2 += 4) {
          Global::InitWiFiReadings = 0;

          Global::LocationRFactor = rfactor;
          Global::LocationQFactor = qfactor;
          Global::WiFiExp1 = wexp1;
          Global::WiFiExp2 = wexp2;

          ifstream data_file(argv[4]);
          getline (data_file, line);
          auto parts = split(line, ',');
          double imu_reads = stoi(parts[0]);
          double max_x = stod(parts[1]);
          double max_y = stod(parts[2]);
          int cur_reading = 0;
          vector<double> distances;

          auto fakescanners =
              Location::TestInit(vector<vector<Result>>(), 1);
          assert(fakescanners.size() == 1);

          if (argv[6][0] == 'r') {
            Imu::X(0,0) = -2;
            Imu::X(1,0) = 44;
          } else {
            Imu::X(0,0) = -2;
            Imu::X(1,0) = 15;
          }
          while (data_file) {
            cur_reading += AddNextSet(data_file, fakescanners[0]);
            Location::UpdateEstimate();
            double x;
            double y;
            //cout << Imu::X(0,0) << ", " << Imu::X(1,0) << "\n";
            //cout << cur_reading << ", " << imu_reads << ", " << max_x << "\n";
            //cout << x << ", " << y << "\n------\n";
            if (argv[6][0] == 'r') {
              x = -2 - (cur_reading / imu_reads) * max_x;
              y = 44 - (cur_reading / imu_reads) * max_y;
            } else {
              x = -2 + (cur_reading / imu_reads) * max_x;
              y = 15 + (cur_reading / imu_reads) * max_y;
            }
            distances.push_back(sqrt(pow(Imu::X(0,0) - x, 2) +
                pow(Imu::X(1,0) - y, 2)));
          }
          sprintf(buffer, "%7.2f %7.2f %7.2f %7.2f %7.2f\n",
              mean(distances), stddev(distances),
              qfactor, wexp1, wexp2);
          cout << buffer;
          results.push_back(vector<double>({mean(distances),
                stddev(distances), qfactor, wexp1, wexp2}));
        }
      }
    }
    sort(results.begin(), results.end(),
        [](const vector<double> &a, const vector<double> &b) -> bool {
            return (a[0] + a[1]) < (b[0] + b[1]);
        });
    ofstream out_file(argv[5]);
    sprintf(buffer, "%7s %7s %7s %7s %7s\n",
        "mean", "std", "yf", "wexp1", "wexp2");
    out_file << buffer;
    for (auto& r : results) {
      sprintf(buffer, "%7.2f %7.2f %7.2f %7.2f %7.2f\n",
          r[0], r[1], r[2], r[3], r[4]);
      out_file << buffer;
    }
    out_file.close();
  }
}

}
