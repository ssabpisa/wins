#ifndef DISPLAY_H
#define DISPLAY_H

#ifndef ISOLATED_TEST
#include "map.h"
#endif

#include "ST7565.h"
#define RETURN_CHARACTER '#'

namespace wins {

using namespace std;

enum FontSize {
  FONT_SIZE_MEDIUM,
  FONT_SIZE_LARGE,
  FONT_SIZE_XLARGE
};

enum Alignment {
  ALIGNMENT_LEFT,
  ALIGNMENT_CENTER
};

enum Page {
  PAGE_SPLASH,
  PAGE_CALIBRATE_PROMPT,
  PAGE_NOCALIBRATE_WARN,
  PAGE_CALBRATING,
  PAGE_MENU,
  PAGE_DESTINATION_PROMPT,
  PAGE_NAVIGATING,
  PAGE_SHUT_DOWN,
  PAGE_DONE,
  PAGE_MAP_SCAN,
  PAGE_DATADUMP
};

typedef std::pair<int,int> Coord;

class Display {
 private:

  FontSize font_size_;
  Alignment alignment_;
  int cursor_;
  int current_line_;
  bool flushed_;
  Page current_page_;
  mutex glcd_mutex;
  void setWorldPixel(uint8_t x, uint8_t y, uint8_t color);
  void drawWorldCircle(uint8_t x0, uint8_t y0, uint8_t r,
                        uint8_t color);
  void drawWorldLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color);
  void ClearLine(int line);
  void ClearScreen();
  char GetChar();
  char GetCharAndEcho();
  void PutString(string s, bool clear = false);
  string GetStringAndEcho();
  void SetFont(FontSize size, Alignment al, int expected_width = 5);
  void SetCurrentLine(int line);
  void IncrmLine();

  Coord map_box_ = Coord(0,0); //box location (top left)
  Coord map_indi_ = Coord(64, 32);  //indicator location
  Coord map_hint_ = Coord(64, 32); 
//  Coord ToScreenCoordinate(Coord sid);
//  Coord ToSidCoordinate(Coord screen);

  void MapLoadWorld(string mapfile);
  void resetWorld();
  Display();

  Page Splash();
  Page CalibratePrompt();
  Page NoCalibrateWarn();
  Page Calibrating();
  Page Menu();
  Page DestinationPrompt();
  Page Navigating();
  Page Done();
  Page ShutDown();
  Page MapScan();
  Page DataDump();

 protected:
  ST7565 glcd_;
  virtual void Flush();

 public:
  Page FirstPage();
  Page ShowPage(Page);
  Page CurrentPage();

  void SaveAsBitmap(string saveas);
  unique_ptr<uint8_t> GetBufferCopy();
  void updateBound(Coord N);
  void MapUpdateHint(Coord sc);
  void MapUpdateIndicator(Coord screen_coord);
  void MapSetVisibleBound(int x, int y);
  void MapDrawVisible();

  static Display& GetInstance();

  // Prevent copying.
  Display(const Display&) = delete;
  void operator=(Display const&) = delete;
};

}

#endif
