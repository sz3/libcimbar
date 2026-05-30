
var _cacheName = 'cimbar-js-v%VERSION%';
var _cacheFiles = [
  '/',
  '/index.html',
  '/cimbar_js.js',
  '/cimbar_js.wasm',
  '/favicon.ico',
  '/icon-192x192.png',
  '/icon-512x512.png',
  '/main.js',
  '/pwa.json'
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
    caches.match(e.request, { ignoreSearch: true }).then(function (response) {
      return response || fetch(e.request);
    })
  );
});

// clean old caches
self.addEventListener('activate', function (e) {
  e.waitUntil(
    caches.keys().then(function (names) {
      return Promise.all(
        names.map(function (cn) {
          if (cn !== _cacheName) {
            return caches.delete(cn);
          }
        })
      );
    }).then(function () {
      return self.clients.claim(); 
    })
  );
});
