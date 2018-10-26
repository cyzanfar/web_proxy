cmake_minimum_required(VERSION 3.12)
project(hw2_handout)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)
include_directories(tiny)

add_executable(hw2_handout
        tiny/cgi-bin/adder.c
        tiny/cgi-bin/Makefile
        tiny/csapp.c
        tiny/csapp.h
        tiny/home.html
        tiny/Makefile
        tiny/README
        tiny/tiny.c
        csapp.c
        csapp.h
        driver.sh
        free-port.sh
        Makefile
        nop-server.py
        port-for-user.pl
        proxy.c
        README)
