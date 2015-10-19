#!/bin/sh

for base in build/ffmpeg/*; do
	# Remove symlinks, just keep the main versions
	mv $base/lib $base/lib-orig
	mkdir -p $base/lib
	for i in $base/lib-orig/lib*.so; do
		cp $base/lib-orig/`basename $i` $base/lib
	done
	rm -rf $base/lib-orig

	# Remove unnecessary stuff
	rm -rf $base/share
done

