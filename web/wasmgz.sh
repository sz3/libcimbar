#!/bin/bash
# run from within web dir
# does various renames for cache busting

RENAME_FILES=$(ls cimbar_js.js cimbar_js.wasm main.js recv.js recv-worker.js zstd.js pwa*.json)
GSUB_FILES="index.html recv.html sw.js recv-sw.js $(echo $RENAME_FILES | sed 's/cimbar_js\.wasm//g')"

VERSION=${VERSION:-$(date --utc '+%Y-%m-%dT%H%M')}
echo $VERSION

mkdir $VERSION
cd $VERSION

for f in $(echo $RENAME_FILES $GSUB_FILES | xargs -n 1 | sort -u); do
	cp ../$f $f
done

for f in $(echo $GSUB_FILES | xargs -n 1 | sort -u); do
	for ren in $(echo $RENAME_FILES | xargs -n 1 | sort -u); do
		renew=$(echo $ren"."$VERSION | awk -F. '{print $1 "." $3 "." $2}')
		renold=$(echo $ren | sed 's/\./\\\./g')  # escape dots
		sed -i "s/$renold/$renew/g" $f
	done
done

for f in $(ls recv.html *sw.js); do
	sed -i "s/%VERSION%/$VERSION/g" $f
done

for f in $(echo $RENAME_FILES | xargs -n 1 | sort -u); do
	newname=$(echo $f"."$VERSION | awk -F. '{print $1 "." $3 "." $2}')
	mv $f $newname
done

tar -czvf ../cimbar.wasm.tar.gz *
