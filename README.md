## About

This US map will be a 3D printed topological projection of the entire United States. Use a grid of LEDs in series which will be programmed to light up in specific locations in accordance with the data from the Health Language Processing Lab.

## Utilities

### Panel Test

Cycles colors and displays column and row numbers on panels.

#### Building

```
make panel-test
```

#### Usage

```
sudo ./panel-test [options]

Options:
    --led-cols         : Number of columns in one panel (default=32).
    --led-rows         : Number of rows in one panel (default=32).
    --led-chain        : Number of daisy-chained panels (default=1).
    --led-parallel     : Number of parallel chains (range=1..3, default=1).
    --led-pixel-mapper : Semicolon-separated list of pixel-mappers to arrange pixels. 
                         Available: "Rotate:<degrees>" 
```

## Acknowledgements

* Henner Zeller ([hzeller/rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix))