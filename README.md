# 3DIotMap

US data visualization using a 3D printed topological map and LED matrices.

## Map Viewer

The map viewer displays a US COVID-19 heat map. The color of a city depends on
the number of positive cases of the state it resides in. The number of positive
cases is mapped from a logarithmic (min_positive_cases..max_positive_cases)
scale to a linear (yellow..red) scale. The map can be transformed by supplying
the LED matrix coordinates of three reference cities, displayed in white.

### Building

The map viewer requires the
[Eigen library](http://eigen.tuxfamily.org/index.php?title=Main_Page).
To "install" Eigen, download and extract its source.

```bash
cd ~/Downloads

wget https://gitlab.com/libeigen/eigen/-/archive/3.3.7/eigen-3.3.7.tar.bz2

tar -xf eigen-3.3.7.tar.bz2
```

When you run `make`, use the directory's path for the `EIGEN` variable.

``` bash
make map-viewer EIGEN="~Downloads/eigen-3.3.7/"
```

### Usage

```bash
sudo ./map-viewer [options]

Options:
    --ref-string <str> : Comma-separated reference string with format,
        "<city1>,<st1>,<x1>,<y1>,<city2>,<st2>,<x2>,<y2>,<city3>,<st3>,<x3>,<y3>"
        (default="Olympia,WA,19,4,Augusta,ME,109,10,Austin,TX,60,51")
    --led-cols         : Number of columns in one panel (default=32).
    --led-rows         : Number of rows in one panel (default=32).
    --led-chain        : Number of daisy-chained panels (default=1).
    --led-parallel     : Number of parallel chains (range=1..3, default=1).
```

The LED matrix coordinate system has the top left pixel at the origin; the
x-axis points from left to right and the Y axis points from top to bottom.

### Demo

```bash
sudo ./map-viewer --led-cols 64 --led-rows 64 --led-chain 2
--ref-string "Olympia,WA,19,4,Augusta,ME,109,10,Austin,TX,60,51"
```

![map viewer demo](img/map-viewer-demo.jpg)

## Panel Test

The panel test cycles colors and displays column and row numbers on all panels,
one at a time.

### Building

```bash
make panel-test
```

### Usage

```bash
sudo ./panel-test [options]

Options:
    --led-cols         : Number of columns in one panel (default=32).
    --led-rows         : Number of rows in one panel (default=32).
    --led-chain        : Number of daisy-chained panels (default=1).
    --led-parallel     : Number of parallel chains (range=1..3, default=1).
    --led-pixel-mapper : Semicolon-separated list of pixel-mappers to arrange
                         pixels.
                         Available: "Rotate:<degrees>"
```

### Demo

```bash
sudo ./panel-test --led-cols 64 --led-rows 64 --led-chain 2
```

![panel test demo](img/panel-test-demo.gif)

## Acknowledgements

* [Henner Zeller](https://github.com/hzeller/rpi-rgb-led-matrix) -
Raspberry Pi LED Matrix library
* [SimpleMaps](https://simplemaps.com/data/us-cities) -
US cities database
* [The COVID Tracking Project](https://covidtracking.com/data/download) -
COVID-19 state data
