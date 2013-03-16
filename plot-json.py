#! /usr/bin/python

import sys
import json
import os

fin = sys.argv[1]

print "Loading from %s" % (fin)

fh = open(fin)
json_data = json.load(fh)

print "Number of rows: %d" % (len(json_data))

fdata = fin + ".dat"
print "Writing data to %s" % (fdata)

data_fh = open(fdata, "w")

host='?'

for row in json_data:
	data_fh.write("%f %f\n" % (float(row['start_ts']), float(row['total_s'])))
	host=row['host']

data_fh.close()

fscript = fin + ".sh"
print "Writing script to %s" % (fscript)

fpng = fin + ".png"

script_fh = open(fscript, "w")

script_fh.write("#! /bin/sh\n\n")
script_fh.write("gnuplot <<EOF > " + fpng + "\n")
script_fh.write("set term png size 800,600\n")
script_fh.write("set autoscale\n")
script_fh.write("set timefmt \"%s\"\n")
script_fh.write("set xdata time\n")
script_fh.write("set format x \"%H:%M:%S\"\n")
script_fh.write("plot \"" + fdata + "\" using 1:2 with lines title \"" + host + "\"\n")
script_fh.write("EOF\n")

os.chmod(fscript, 0755)
script_fh.close()

print "Now invoke %s to generate %s" % (fscript, fpng)
