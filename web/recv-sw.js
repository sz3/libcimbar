
var _cacheName = 'cimbar-recv-js-v%VERSION%';
var _cacheFiles = [
  '/',
  '/recv.html',
  '/cimbar_js.js',
  '/cimbar_js.wasm',
  '/favicon.ico',
  '/icon-192x192.png',
  '/icon-512x512.png',
  '/icon-512x512-maskable.png',
  '/recv.js',
  '/recv-worker.js',
  '/pwa-recv.json',
  '/zstd.js'
];

// fetch files
self.addEventListener('install', function (e) {
  e.waitUntil(
    caches.open(_cacheName).then(function (cache) {
      return cache.addAll(_cacheFiles);
    })
  );
  self.skipWaiting();
});

// serve from cache
self.addEventListener('fetch', function (e) {
  e.respondWith(
    caches.match(e.request).then(function (response) {
      return response || fetch(e.request);
    })
  );
});

// clean old caches
self.addEventListener('activate', function (e) {
  e.waitUntil(function () {
    caches.keys().then(function (names) {
      for (var i in names)
        if (names[i] != _cacheName)
          caches.delete(names[i]);
    });
  });
});
