#ifndef DISPLAY_H
#define DISPLAY_H

#ifndef ISOLATED_TEST
#include "map.h"
#endif

#include "ST7565.h"

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
  PAGE_SHUT_DOWN,
  PAGE_MENU,
  PAGE_DESTINATION_PROMPT,
  PAGE_NAVIGATING,
  PAGE_DONE
};

class Display {
 private:
  ST7565 glcd_;

  FontSize font_size_;
  Alignment alignment_;
  int cursor_;
  int current_line_;
  bool flushed_;
  Page current_page_;

  void ClearLine(int line);
  void ClearScreen();
  char GetChar();
  char GetCharAndEcho();
  void PutString(string s, bool clear = false);
  string GetStringAndEcho();
  void Flush();
  void SetFont(FontSize size, Alignment al, int expected_width = 5);
  void SetCurrentLine(int line);
  void IncrmLine();

  Display();

  Page Menu();
  Page DestinationPrompt();
  Page Navigating();
  Page Done();
  Page ShutDown();
 public:
  Page FirstPage();
  Page ShowPage(Page);
  Page CurrentPage();

  void SaveAsBitmap(string saveas);

  static Display& GetInstance();

  // Prevent copying.
  Display(const Display&) = delete;
  void operator=(Display const&) = delete;
};

}

#endif
