sensorium
=========

KiwiBurn Art Project 2014

I had a great ambition to blog about the construction of this, but
time constraints meant I was too busy just trying to make things work.

Now 2015 is coming around I need to revisit the code (mostly to not have to
count the LEDs on each of my strips again!) so I thought I'd make sure all
code and bits are at least online.

My 2015 Kiwiburn project is [Tensegrity](https://github.com/ferrouswheel/tensegrity)

## Teensy 3.1 sketch

The set up involves two Teensy 3.1's connected by their SPI pins.
They both have `Sensorium/Sensorium.ino` loaded on it. One with
`masterTeensy = true`, the other with `masterTeensy = false`.

The master sends updates with

- 8 sync bytes of 255,
- dial reading
- the framecount.

Both teensy's read the "big button state" which Kiwiburn participants could
use to turn the sensorium on and off. The only the master reads the dial.

I have no idea why I decided control both with the button, but communicate the
dial state by SPI.

There are some nasty hacks in this code due to time constraints and weekend of
trying to get something working before the festival!
