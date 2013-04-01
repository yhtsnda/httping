#! /bin/sh

valgrind --show-reachable=yes --leak-check=full --read-var-info=yes --track-origins=yes --malloc-fill=93 --free-fill=b9 --error-limit=no ./httping --graph-limit 250 -K -i 0.1 -W --aggregate 3,5,7 -Q belle
