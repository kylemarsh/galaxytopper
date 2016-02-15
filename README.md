# galaxytopper
:tophat: Firmware and associated goodies for my space-tastic top hat!

* The firmware itself is galaxytopper.ino
* Python script that uses astropy to create plots of astronomical data is
  grb_plotter.py
* Astronomical data to plot is in grbs[_trunc].votable.xml
  * this is an export from http://www.asdc.asi.it/grbgbm/
  * plotted astronomy data are in plots/
* .gitignore includes "shiftbrite" and "ticker" because I have those two
  libraries symlinked in from elsewhere:
  * https://github.com/kylemarsh/shiftbrite
  * https://github.com/kylemarsh/rgb_ticker@fc4638a3bab55e2259d5e120f80a0b4b6e69e16d
    * (NB: I'm pretty sure rgb_ticker used this way leaks memory;
      the memory management in kylemarsh/rgb_ticker@c4c391f7c31f5d3c864bb9a9a88f033d1a1bc00b
      is bugged and I haven't gone back to figure out what's wrong yet. just
      roll back a commit from there and it'll work (albeit leakily))

Inspiration for the Galaxy Topper is Chris Knight's
[instructable](http://www.instructables.com/id/My-hat-its-full-of-stars/).
