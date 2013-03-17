#! /bin/sh

valgrind --show-reachable=yes --leak-check=full --read-var-info=yes --track-origins=yes --malloc-fill=93 --free-fill=b9 --error-limit=no ./httping -l -6 -M vps001.vanheusden.com
