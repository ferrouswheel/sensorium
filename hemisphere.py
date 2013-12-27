#!/usr/bin/env python
from math import pi, cos, sin
import sys

def drange(start, stop, step):
     r = start
     while r < stop:
     	yield r
     	r += step


spacing = 0.11  # m
lines = []
total_length = 0

radius = 0.7
halfpi = pi / 2.0
max_in_plane = 100

options = [(30, 15.26), (60, 24.95)]
option_index = 0

pixel_density, cost_per_m = options[option_index]
theta_inc = 0.12 # 06

scaling_factor = 2.5

cap_angle = 0.4646
row_count = 0

for theta in drange(-halfpi+cap_angle, halfpi-cap_angle, theta_inc):
    row_count += 1
    # Most at theta == 0
    inc = (2*pi) / ((halfpi - abs(theta) + 0.01) * max_in_plane)

    # cos(theta) ends up being the scaling factor for the arc size
    arc_radius = cos(theta) * radius
    arc_length = arc_radius * pi
    pixels_on_this_arc = arc_length * pixel_density
    print >> sys.stderr, "%.2f, %d" % (arc_length, pixels_on_this_arc)
    inc = pi / pixels_on_this_arc
    total_length += arc_length

    for phi in drange(-halfpi, halfpi, inc):
        x = cos(theta) * cos(phi) * radius * scaling_factor
        y = cos(theta) * sin(phi) * radius * scaling_factor
        z = sin(theta) * radius * scaling_factor

        #-90 <= theta <= 90
        #0 <= phi <= 360

        lines.append('  {"point": [%.2f, %.2f, %.2f]}' %
                         (x, y, z))

#for theta in drange(-halfpi, halfpi, 0.05):
    ## Most at theta == 0
    #inc = (2*pi) / ((halfpi - abs(theta) + 0.01) * max_in_plane)
    #for phi in drange(-halfpi, halfpi, inc):
        ## bottom -> top
        #x = cos(theta) * cos(phi) * radius
        #y = cos(theta) * sin(phi) * radius
        #z = sin(theta) * radius

        ##-90 <= theta <= 90
        ##0 <= phi <= 360

        #lines.append('  {"point": [%.2f, %.2f, %.2f]}' %
                         #(x, y, z))
print '[\n' + ',\n'.join(lines) + '\n]'
print >> sys.stderr, 'pixels = ', len(lines)
print >> sys.stderr, 'rows = ', row_count
print >> sys.stderr, 'total_length = ', total_length
print >> sys.stderr, 'total_cost = ', total_length * cost_per_m

