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

static void CyclePanels(RGBMatrix *matrix, RGBMatrix::Options *matrix_options) {
  Color colors[] = {color_red, color_green, color_blue};

  int panel_cols = matrix_options->chain_length;
  int panel_rows = matrix_options->parallel;

  // Swap panel_cols and panel_rows if 'Rotate' parameter is '90' or '270'.
  string mapping = matrix_options->pixel_mapper_config ? matrix_options->pixel_mapper_config : "";
  if (!mapping.find("Rotate:90") || !mapping.find("Rotate:270"))
    swap(panel_cols, panel_rows);

  // Traverse panels row-wise.
  for (int col = 0; col < panel_cols; col++) {
    for (int row = 0; row < panel_rows; row++) {

      int upper_x = matrix_options->cols * col;
      int upper_y = matrix_options->rows * row;
      int lower_x = matrix_options->cols * col + matrix_options->cols - 1;
      int lower_y = matrix_options->rows * row + matrix_options->rows - 1;

      // Display colors individually on panel.
      int n_colors = sizeof(colors) / sizeof(colors[0]);
      for (int i = 0; i < n_colors; i++) {
        for (int x = upper_x; x <= lower_x; x++)
          for (int y = upper_y; y <= lower_y; y++)
          matrix->SetPixel(x, y, colors[i].r, colors[i].g, colors[i].b);
	      sleep(1);
      }

      matrix->Clear();
      
      // Display (<col>,<row>) on panel.
      char text[24];
      snprintf(text, sizeof(text), "(%d,%d)", col, row);
      DrawText(matrix, font, upper_x, lower_y, color_white, &color_black, text);
      sleep(1);

      matrix->Clear();
    }
  }
}

int main(int argc, char *argv[]) {
  RGBMatrix::Options matrix_options;
  
  RGBMatrix *matrix = CreateMatrixFromFlags(&argc, &argv, &matrix_options);
  if (matrix == NULL)
    return 1;

  const char *font_file = "fonts/6x12.bdf";
  if (!font.LoadFont(font_file)) {
    fprintf(stderr, "Failed to load font: '%s'\n", font_file);
    return 1;
  }
  
  // Set handler for external interrupts.
  signal(SIGINT, InterruptHandler);

  cout << "Press Ctrl+C to exit.\n";
  do
    CyclePanels(matrix, &matrix_options);
  while (!interrupt_received);

  delete matrix;

  return 0;
}