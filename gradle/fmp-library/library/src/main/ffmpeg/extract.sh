#!/bin/sh

tar -zxvf ffmpeg-2.5.3.tar.gz
mv ffmpeg-2.5.3 ffmpeg
for i in `find diffs -type f`; do
	(cd ffmpeg && patch -p1 < ../$i)
done
