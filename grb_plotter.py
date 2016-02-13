# Code to generate a dotfield for my galaxy top hat*. Rather than placing my
# "stars" randomly, I wanted to use a pattern based in real astronomy data
# of some sort. In browsing online I realized for the first time how to read
# those elliptical maps of the sky you see astronomers use all the time. I
# decided to get data from the equator of one of those maps to put around my
# hat.
#
# Unfortunately, all the maps I could find were either too dense or too
# sparse so I realized I'd have to dig into the actual data myself and
# generate an appropriate map. I decided on detected Gamma Ray Bursts**
# because their distribution is reasonably uniform across the sky, not
# clustered on or away from the plane of the Milky Way.
#
# But we've detected over 1000 GRBs since we started looking in 2008...so I
# needed to trim the data down to the last couple of years of data to limit
# the number of points on my hat to about 200. I got my data here*** and
# tried a couple of different export formats before finding one (votable
# xml) that astropy would read properly.
#
# *The inspiration for my hat is Chris Knight's instructible, here:
#  http://www.instructables.com/id/My-hat-its-full-of-stars/
#
# **I highly recommend Phil Plait's Crash Course Astronomy (this is episode
#  40, on GRBs): https://www.youtube.com/watch?v=Z2zA9nPFN5A
#
# *** http://heasarc.gsfc.nasa.gov/db-perl/W3Browse/w3table.pl?tablehead=name%3Dfermigbrst&Action=More+Options

from astropy.table import Table
import astropy.units as u
import astropy.coordinates as coord
import numpy as np
import matplotlib

# On my mac I'm using a brew installed python, which is not installed as a
# "framework", causing pyplot to throw a fit. Explicitly telling matplotlib
# to use the Agg backend instead of _macosx means we can't have our code
# directly show the plot, but we can save it out to SVG just fine.
matplotlib.use('Agg')
import matplotlib.pyplot as plt

# I used this code to determine how many data points I had in each year.
# year_to_mjd maps the years from 2008-2016 (the bounds of my data) to the
# Modified Julean Date for 01 January of that year.

#year_to_mjd = {2016: 57388, 2008: 54466, 2009: 54832, 2010: 55197, 2011: 55562, 2012: 55927, 2013: 56293, 2014: 56658, 2015: 57023}
#by_year = {}
#t = Table.read('grbs.votable.xml')
#tt = [x for x in t if x['dec'] > -30 and x['dec'] < 30]
#for y in (2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015):
    #by_year[y] = [x for x in tt if x['trigger_time'] >= year_to_mjd[y] and x['trigger_time'] < year_to_mjd[y+1]]

# I never figured out how to filter an astropy table object, so I tweaked my
# input file (in this case in the votable xml format) in my text editor to
# reduce the data set down to just the points I was interested in. I'd have
# loved to be able to do this in code so that I could easily play with
# different plots to see which I liked best, but c'est la vie.
t = Table.read('grbs_trunc.votable.xml')

# One of the things I added to my data file was a column named "group" with
# values 1-6 that I manually assigned to my data points so that I had 6
# roughly even groups.  You could probably also do this using some
# conditions in your list comprehension.
colormap = {1: [0,0,0], 2:[1,0,0], 3:[0,1,0], 4:[0,0,1], 5: [1,1,0], 6:[1,0,1]}
colors = [colormap[x] for x in t['group']]

ra = coord.Angle(t['ra'].filled(np.nan))
ra = ra.wrap_at(180*u.degree)
dec = coord.Angle(t['dec'].filled(np.nan))

# Plot your points on *all* the available projections!
for p in ('rectilinear', 'polar', 'mollweide', 'aitoff', 'hammer', 'lambert'):
    # I used this aspect ratio for my recitlinear plot, because that gave me
    # the closest field for the dimensions of my project
    fig = plt.figure(figsize=(22.5, 3.25))
    axes = fig.add_subplot(111, projection=p)
    # s is the size of the dots. c is the colors to use (in my case it's a
    # list of lists, the inner lists consisting of RGB values). edgecolor is
    # the color of the stroke on the border of each dot (none means don't
    # draw one). alpha is the transparency given from 0 (invisible) to 1
    # (solid)
    axes.scatter(x=ra.radian, y=dec.radian, s=5, c=colors, edgecolor='none', alpha=0.75)
    axes.grid(True) # Do show a grid of dotted lines inside the plot
    axes.set_xticklabels([]) # don't show any labels on the x grid marks
    fig.savefig('plots/%s.svg' % p)

