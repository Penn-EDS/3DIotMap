#include "led-matrix.h"

#include <graphics.h>
#include <iostream>
#include <signal.h>
#include <string>
#include <unistd.h>

using namespace std;
using namespace rgb_matrix;

Color color_black = {0, 0, 0};
Color color_blue = {0, 0, 255};
Color color_green = {0, 255, 0};
Color color_red = {255, 0, 0};
Color color_white = {255, 255, 255};

Font font;

volatile bool interrupt_received = false;

static void InterruptHandler(int signal) {
  cout << "\nInterrupt...\n";
  interrupt_received = true;
}

static void CyclePanels(Canvas *canvas, RGBMatrix::Options *options, int pause_s) {
  Color colors[] = {color_red, color_green, color_blue};

  int panel_cols = options->chain_length;
  int panel_rows = options->parallel;

  // swap cols and rows if 'Rotate' parameter is 90 or 270
  string mapping = options->pixel_mapper_config ? options->pixel_mapper_config : "";
  if (!mapping.find("Rotate:90") || !mapping.find("Rotate:270"))
    swap(panel_cols, panel_rows);

  // traverse panels row-wise
  for (int col = 0; col < panel_cols; col++) {
    for (int row = 0; row < panel_rows; row++) {

      // upper left (x0,y0) pixel of panel
      int x0 = options->cols * col;
      int y0 = options->rows * row;

      // lower right (x1, y1) pixel of panel
      int x1 = options->cols * col + options->cols - 1;
      int y1 = options->rows * row + options->rows - 1;

      // display colors individually on panel
      int n_colors = sizeof(colors) / sizeof(colors[0]);
      for (int i = 0; i < n_colors; i++) {
        for (int x = x0; x <= x1; x++)
          for (int y = y0; y <= y1; y++)
          canvas->SetPixel(x, y, colors[i].r, colors[i].g, colors[i].b);
	      sleep(pause_s);
      }

      canvas->Clear();
      
      // display (<col>,<row>) on panel
      char text[24];
      snprintf(text, sizeof(text), "(%d,%d)", col, row);
      DrawText(canvas, font, x0, y1, color_white, &color_black, text);
      sleep(pause_s);

      canvas->Clear();
    }
  }
}

int main(int argc, char *argv[]) {
  RGBMatrix::Options options;
  
  Canvas *canvas = CreateMatrixFromFlags(&argc, &argv, &options);
  if (canvas == NULL)
    return 1;

  const char *font_file = "fonts/6x12.bdf";
  if (!font.LoadFont(font_file)) {
    fprintf(stderr, "Failed to load font: '%s'\n", font_file);
    return 1;
  }
  
  // set up signal handler for Ctrl+C presses
  signal(SIGINT, InterruptHandler);

  cout << "Press Ctrl+C to exit.\n";
  do {
    CyclePanels(canvas, &options, 1);
  } while (!interrupt_received);

  delete canvas;

  return 0;
}