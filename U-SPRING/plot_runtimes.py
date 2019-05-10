from __future__ import division
import pickle
import sys
from pprint import pprint
import numpy
import pylab as P
from datetime import datetime
import matplotlib
from matplotlib.transforms import Bbox
from matplotlib.font_manager import FontProperties
from collections import defaultdict
from ci import calc_sample_mean
import csv

matplotlib.rc('font',family='serif')
font = FontProperties(size=22)
font_smaller = FontProperties(size=20)
font_ticks = FontProperties(size=18)

infile = "runtimes.csv"

data = []
with open(infile, "rb" ) as infilehandle:
    header = [h.strip() for h in infilehandle.next().split(';')]
    reader = csv.DictReader(infilehandle,fieldnames=header,delimiter=";")
    for line in reader:
        l = {}
        for k in header:
            if k not in ["Algorithm"]:
                l[k] = float(line[k])
            else:
                l[k] = str(line[k]).strip()
        data.append(l)

milp_conversion = []
milp_solving = []
milp_conversion_back = []
heuristic_construction = []

for d in data:
    if d["Algorithm"]=="MILP conversion":
        milp_conversion.append(d["Runtime"])
    if d["Algorithm"]=="MILP solving":
        milp_solving.append(d["Runtime"])
    if d["Algorithm"]=="MILP conversion back":
        milp_conversion_back.append(d["Runtime"])
    if d["Algorithm"]=="Heuristic construction":
        heuristic_construction.append(d["Runtime"])

print "MILP conversion",calc_sample_mean(milp_conversion)
print "MILP solving",calc_sample_mean(milp_solving)
print "MILP conversion back",calc_sample_mean(milp_conversion_back)
print "Heuristic construction",calc_sample_mean(heuristic_construction)

N = len(heuristic_construction)
ind = numpy.arange(N)  # the x locations for the groups
width = 0.35       # the width of the bars

# font = FontProperties(size=18)
# font_smaller = FontProperties(size=15)

F = P.figure()
AX1 = F.add_subplot(111)

mbars = AX1.bar(ind, milp_solving, width, color='blue')
hbars = AX1.bar(ind + width, heuristic_construction, width, color='gray', hatch='////')

#AX1.set_ylim([0.0,1600.0])

AX1.set_xlabel("Runs", fontproperties=font)
AX1.set_ylabel("Runtime", fontproperties=font)


AX1.set_xticks(ind + width)
# AX1.set_xticklabels(requests)
AX1.set_xticklabels(range(1,len(heuristic_construction)+1))


AX1.legend((mbars, hbars), ('MILP', 'Heuristic'),
    # loc='upper center', bbox_to_anchor=(0.5, 1.07),
    loc='lower right',
          ncol=2, fancybox=True, shadow=True, framealpha=0.6)
for tick in AX1.xaxis.get_major_ticks():
    # tick.label1.set_fontsize(14)
    tick.label1.set_fontproperties(font_ticks)
for tick in AX1.yaxis.get_major_ticks():
    # tick.label1.set_fontsize(14)
    tick.label1.set_fontproperties(font_ticks)


def autolabel(bars):
    # attach some text labels
    for bar in bars:
        height = bar.get_height()
        AX1.text(bar.get_x() + bar.get_width()/2., 1.05*height,
                '%0.4f' % height,
                # '%0.4f' % (height / 60000),
                ha='center', va='bottom')

# autolabel(hbars)
# autolabel(obars)

#P.show()

# P.savefig("plots/plot_" + str(hmetric) + "_" + str(date) + ".pdf", bbox_inches='tight')
P.savefig("runtimes.pdf", bbox_inches='tight')
# F = P.figure(2)
# F.legend(plotlines, plotlabels, loc='upper left', shadow=False, fancybox=True, prop=font, ncol=3)

# bb = Bbox.from_bounds(0, 0, 40, 40)
# P.savefig('plot_legend.pdf', bbox_inches=bb)
