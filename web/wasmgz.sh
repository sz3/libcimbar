#!/bin/bash
# run from within web dir
# does various renames for cache busting
# reproducible build, hopefully

RENAME_FILES=$(ls cimbar_js.js cimbar_js.wasm main.js send.js send-worker.js recv.js recv-worker.js zstd.js pwa*.json)
GSUB_FILES="index.html recv.html sw.js recv-sw.js $(echo $RENAME_FILES | sed 's/cimbar_js\.wasm//g')"

SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH:-$(date +%s)}
VERSION=${VERSION:-$(date --utc -d "@$SOURCE_DATE_EPOCH" '+%Y-%m-%dT%H%M')}
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

# reproducible (hopefully) tar.gz
find . -mindepth 1 | env LC_ALL=C sort | tar --transform='s|^\./||' --mtime="@${SOURCE_DATE_EPOCH:-0}" --owner=0 --group=0 --numeric-owner  --pax-option=exthdr.name=%d/PaxHeaders/%f,atime:=0,ctime:=0 -c -T - | gzip -n > ../cimbar.wasm.tar.gz
