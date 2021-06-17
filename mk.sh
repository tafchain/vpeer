#!/bin/bash
	
make plat=linux mariadb=1 release=1 use_sign=1 native=1 clean && make plat=linux mariadb=1 release=1 native=1 use_sign=1 -j24
