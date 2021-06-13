
var _cacheName = 'cimbar-js-v0.5.8';
var _cacheFiles = [
  '/index.html',
  '/cimbar_js.js',
  '/cimbar_js.wasm',
  '/main.js'
];

// fetch files
self.addEventListener('install', function(e) {
  e.waitUntil(
    caches.open(_cacheName).then(function(cache) {
      return cache.addAll(_cacheFiles);
    })
  );
  self.skipWaiting();
});

// serve from cache
self.addEventListener('fetch', function(e) {
  e.respondWith(
    caches.match(e.request).then(function(response) {
      return response || fetch(e.request);
    })
  );
});

// clean old caches
self.addEventListener('activate', function(e) {
  e.waitUntil(caches.keys().then(function(keyList) {
    Promise.all(keyList.map(function(key) {
      if (key === _cacheName) {
        return;
      }
      caches.delete(key);
    }));
  })());
});
