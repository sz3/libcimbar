#!/bin/bash
# run from within web dir
# does various renames for cache busting

VERSION=${VERSION:-$(date --iso-8601)}
echo $VERSION

mkdir $VERSION
cd $VERSION

cp ../cimbar_js.wasm cimbar_js-$VERSION.wasm
sed "s/cimbar_js.wasm/cimbar_js-$VERSION.wasm/g" ../cimbar_js.js > cimbar_js-$VERSION.js
sed "s/cimbar_js.js/cimbar_js-$VERSION.js/g" ../main.js > main-$VERSION.js

sed "s/main.js/main-$VERSION.js/g" ../index.html | sed "s/cimbar_js.js/cimbar_js-$VERSION.js/g" > index.html

sed "s/cimbar_js.js/cimbar_js-$VERSION.js/g" ../dec-worker.js > dec-worker-$VERSION.js
sed "s/cimbar_js.js/cimbar_js-$VERSION.js/g" ../dec.js | sed "s/dec-worker.js/dec-worker-$VERSION.js/g" > dec-$VERSION.js

sed "s/dec.js/dec-$VERSION.js/g" ../dec.html | sed "s/cimbar_js.js/cimbar_js-$VERSION.js/g" > dec.html

tar -czvf ../cimbar.wasm.tar.gz *
