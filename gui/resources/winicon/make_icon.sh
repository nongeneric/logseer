#!/bin/bash

for size in 16 32 48 64
do
    convert -resize ${size}x${size} -background transparent ../logseer.svg logseer-${size}.png
done

convert *.png logseer.ico
rm *.png
