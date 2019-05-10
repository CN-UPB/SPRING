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
#from optimization.ci import calc_sample_mean
import csv

matplotlib.rc('font',family='serif')
font = FontProperties(size=22)
font_smaller = FontProperties(size=20)
font_ticks = FontProperties(size=18)

# infile = "stat_08_2910.csv"
infiles = ["stat_08_0111-n100.csv","stat_08_0111-n200.csv","stat_08_0111-n500.csv","stat_08_0111-n1000.csv"]

data = dict()
for ind,infile in enumerate(infiles):
    data[ind] = []
    with open(infile, "rb" ) as infilehandle:
        header = [h.strip() for h in infilehandle.next().split(';')]
        reader = csv.DictReader(infilehandle,fieldnames=header,delimiter=";")
        for line in reader:
            l = {}
            for k in header:
                if k not in ["Alg"]:
                    l[k] = float(line[k])
                else:
                    l[k] = str(line[k]).strip()
            data[ind].append(l)

print len(data[0])

# metric = "Node overloads"
# metric = "Link overloads"
# metric = "Total latency"
# metric = "Instance creations"
# metric = "Instance deletions"
# metric = "Max CPU overload"
# metric = "Max memory overload"
# metric = "Max data rate overload"
# metric = "Total CPU load"
# metric = "Total memory load"
metric = "Total data rate"



# 8-1 node cap = 200 
# 8-2 node cap = 150 

# events = range(1,int(len(data[0])/2)+1)
events = range(1,int(len(data[0]))+1)
#events = range(1,16)

# milp_plot_data = []
heuristic_plot_data = dict()

for i,res in data.iteritems():
    heuristic_plot_data[i] = []
    for e in events:
        for d in res:
            # if d["Alg"]=="milp" and d["Event"]==e:
            #     milp_plot_data.append(d[metric])
            if d["Alg"]=="heuristic" and d["Event"]==e:
                heuristic_plot_data[i].append(d[metric])

N = len(events)
ind = numpy.arange(N)  # the x locations for the groups
width = 0.35       # the width of the bars

# font = FontProperties(size=18)
# font_smaller = FontProperties(size=15)

F = P.figure()
AX1 = F.add_subplot(111)

#mbars = AX1.bar(ind, milp_plot_data, width, color='red')
#hbars = AX1.bar(ind + width, heuristic_plot_data, width, color='black', hatch='////')
# mline, = AX1.plot(events,milp_plot_data,linewidth=1,color='red',marker='s')
# hline, = AX1.plot(events,heuristic_plot_data,linewidth=1,color='black',marker='o')
hline100, = AX1.plot(events,heuristic_plot_data[0],linewidth=1,color='black',marker='>')
hline200, = AX1.plot(events,heuristic_plot_data[1],linewidth=1,color='red',marker='s')
hline500, = AX1.plot(events,heuristic_plot_data[2],linewidth=1,color='blue',marker='^')
hline1000, = AX1.plot(events,heuristic_plot_data[3],linewidth=1,color='green',marker='o')



#AX1.set_ylim([0.0,1600.0])

AX1.set_xlabel("Events", fontproperties=font)
AX1.set_ylabel(metric, fontproperties=font)

a = [10,20,30,40,50,60,70]
# AX1.set_xticks(ind + width)
AX1.set_xticks(a)
# AX1.set_xticklabels(requests)
# AX1.set_xticklabels(events)
AX1.set_xticklabels(a)


#AX1.legend((mbars, hbars), ('MILP', 'Heuristic'),
AX1.legend((hline100,hline200,hline500,hline1000), ('100 Nodes','200 Nodes','500 Nodes','1000 Nodes'),
    # loc='upper center', bbox_to_anchor=(0.5, 1.07),
    # loc='lower right',
    loc='upper left',
          ncol=1, fancybox=True, shadow=True, framealpha=0.6)
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
                ha='center', va='bottom')

# autolabel(hbars)
# autolabel(obars)

#P.show()

# P.savefig("plots/plot_" + str(hmetric) + "_" + str(date) + ".pdf", bbox_inches='tight')
# P.savefig("plot.pdf", bbox_inches='tight')
P.savefig("plots/scenario_streaming_08-01Nov-heuronly/plot_"+metric+".pdf", bbox_inches='tight')
# F = P.figure(2)
# F.legend(plotlines, plotlabels, loc='upper left', shadow=False, fancybox=True, prop=font, ncol=3)

# bb = Bbox.from_bounds(0, 0, 40, 40)
# P.savefig('plot_legend.pdf', bbox_inches=bb)
