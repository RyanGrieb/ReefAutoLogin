#!/bin/bash

gcc -g Driver/driver.c CurlReader/curlreader.c UnixTime/unixtime.c  -o ReefAutoLogin -lcurl -ljson-c
echo Done!
