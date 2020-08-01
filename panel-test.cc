#include "led-matrix.h"

#include <graphics.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;
using rgb_matrix::Color;
using rgb_matrix::Font;

using namespace std;

Color color_black = {0, 0, 0};
Color color_blue = {0, 0, 255};
Color color_green = {0, 255, 0};
Color color_red = {255, 0, 0};
Color color_white = {255, 255, 255};

Font small_font;

volatile bool interrupt_received = false;

static void InterruptHandler(int signal) {
  cout << "\nInterrupt...\n";
  interrupt_received = true;
}

static void FillCanvasArea(Canvas *canvas, Color *color, int x0, int y0, int x1, int y1) {
  for (int x = x0; x <= x1; x++)
    for (int y = y0; y <= y1; y++)
      canvas->SetPixel(x, y, color->r, color->g, color->b);
}

static void TestPanels(Canvas *canvas, RGBMatrix::Options *options) {
  int sleep_time_s = 1;

  Color colors[] = {color_red, color_green, color_blue};
  int n_colors = sizeof(colors) / sizeof(colors[0]);

  // traverse panels row-wise
  for (int col = 0; col < options->chain_length; col++) {
    for (int row = 0; row < options->parallel; row++) {

      // get upper left (x0,y0) and lower right (x1,y1) pixels of panel
      int x0 = options->cols * col;
      int y0 = options->rows * row;
      int x1 = options->cols * col + options->cols - 1;
      int y1 = options->rows * row + options->rows - 1;

      // display colors individually on panel
      for (int i = 0; i < n_colors; i++) {
	      FillCanvasArea(canvas, &colors[i], x0, y0, x1, y1);
	      sleep(sleep_time_s);
      }
      FillCanvasArea(canvas, &color_black, x0, y0, x1, y1); // clear panel
      
      // display (<col>,<row>) on panel
      char text[24];
      snprintf(text, sizeof(text), "(%d,%d)", col, row);
      rgb_matrix::DrawText(canvas, small_font, x0, y1, color_white, &color_black, text);
      sleep(sleep_time_s);
      
    }
  }
}

int main(int argc, char *argv[]) {
  RGBMatrix::Options options;
  
  Canvas *canvas = rgb_matrix::CreateMatrixFromFlags(&argc, &argv, &options);
  if (canvas == NULL)
    return 1;

  if (!small_font.LoadFont("./6x12.bdf")) {
    cout << "Error loading font.\n";
    return 2;
  }
  
  // set up signal handler for Ctrl+C presses
  signal(SIGINT, InterruptHandler);

  // loop until there's an interrupt 
  cout << "Press Ctrl+C to exit.\n";
  do {
    TestPanels(canvas, &options);
    canvas->Clear();
  } while (!interrupt_received);

  // clean up
  delete canvas;

  return 0;
}