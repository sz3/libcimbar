#!/bin/bash
# run from within web dir
# does various renames for cache busting

RENAME_FILES=$(ls cimbar_js.js cimbar_js.wasm main.js recv*.js zstd.js sw*.js pwa*.json)
GSUB_FILES="index.html recv.html $(echo $RENAME_FILES | sed 's/cimbar_js\.wasm//g')"

VERSION=${VERSION:-$(date --iso-8601=hours --utc | awk -F+ '{print $1}')}
echo $VERSION

mkdir $VERSION
cd $VERSION

for f in $(echo $RENAME_FILES $GSUB_FILES | xargs -n 1 | sort -u); do
	cp ../$f $f
done

for f in $(echo $GSUB_FILES | xargs -n 1 | sort -u); do
	for ren in $(echo $RENAME_FILES | xargs -n 1 | sort -u); do
		renew=$(echo $ren"."$VERSION | awk -F. '{print $1 "." $3 "." $2}')
		sed -i "s/$ren/$renew/g" $f
	done
done

for f in $(ls sw*.js); do
	sed -i "s/VERSION/$VERSION/g" $f
done

for f in $(echo $RENAME_FILES | xargs -n 1 | sort -u); do
	newname=$(echo $f"."$VERSION | awk -F. '{print $1 "." $3 "." $2}')
	mv $f $newname
done

tar -czvf ../cimbar.wasm.tar.gz *
