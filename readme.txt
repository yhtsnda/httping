Installation:
------------
make install

to build without SSL support:
	make -f Makefile.nossl install

to build with "tcp fast open" support, add the following line to the makefile:
	TFO=yes
Please note that TCP fast open requires a Linux kernel of version 3.7 or more
recent.

to build without SSL support, add the following line to the makefile:
	SSL=no


Usage:
-----
httping www.vanheusden.com


See:
	httping -h
for a list of commandline switches. Also check the man-page.

plot-json.py is a script to convert the json-output of httping to a script for gnuplot.

If this script fails with the following error:
	ValueError: Expecting object: [...]
then make sure the json-file ends with a ']' (without the quotes).
In sm cases this character is missing.


Thanks to Thanatos for cookie and authentication support.
Many thanks to Olaf van der Spek for lots of bug-reports, testing, ideas and suggestions.


For everything more or less related to 'httping', please feel free
to contact me on: folkert@vanheusden.com

Please support my opensource development: http://www.vanheusden.com/wishlist.php
Or send any surplus bitcoins to 1N5Sn4jny4xVwTwSYLnf7WnFQEGoVRmTQF
