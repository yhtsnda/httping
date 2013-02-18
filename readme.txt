Installation:
------------
make install

to build without SSL support:
	make -f Makefile.nossl install

to build with "tcp fast open" support, add the following to the makefile:
	TFO=yes
Please note that TCP fast open requires a Linux kernel of version 3.7 or more
recent.


Usage:
-----
httping www.vanheusden.com


Thanks to Thanatos for cookie and authentication support.


For everything more or less related to 'httping', please feel free
to contact me on: folkert@vanheusden.com

Please support my opensource development: http://www.vanheusden.com/wishlist.php
Or send any surplus bitcoins to 1N5Sn4jny4xVwTwSYLnf7WnFQEGoVRmTQF
