'use strict';

const version = 'v20160440';
const __DEVELOPMENT__ = false;
const __DEBUG__ = false;
const offlineResources = [
  '/',
  '/offline.html',
  '/offline.svg'
];

const ignoreFetch = [
  /https?:\/\/cdn.bootcss.com\//,
  /https?:\/\/static.duoshuo.com\//,
  /https?:\/\/www.google-analytics.com\//,
  /https?:\/\/dn-lbstatics.qbox.me\//,
  /https?:\/\/ajax.cloudflare.com\//,
];


//////////
// Install
//////////
function onInstall(event) {
  log('install event in progress.');

  event.waitUntil(updateStaticCache());
}

function updateStaticCache() {
  return caches
    .open(cacheKey('offline'))
    .then((cache) => {
      return cache.addAll(offlineResources);
    })
    .then(() => {
      log('installation complete!');
    });
}

////////
// Fetch
////////
function onFetch(event) {
  const request = event.request;

  if (shouldAlwaysFetch(request)) {
    event.respondWith(networkedOrOffline(request));
    return;
  }

  if (shouldFetchAndCache(request)) {
    event.respondWith(networkedOrCached(request));
    return;
  }

  event.respondWith(cachedOrNetworked(request));
}

function networkedOrCached(request) {
  return networkedAndCache(request)
    .catch(() => { return cachedOrOffline(request) });
}

// Stash response in cache as side-effect of network request
function networkedAndCache(request) {
  return fetch(request)
    .then((response) => {
      var copy = response.clone();
      caches.open(cacheKey('resources'))
        .then((cache) => {
          cache.put(request, copy);
        });

      log("(network: cache write)", request.method, request.url);
      return response;
    });
}

function cachedOrNetworked(request) {
  return caches.match(request)
    .then((response) => {
      log(response ? '(cached)' : '(network: cache miss)', request.method, request.url);
      return response ||
        networkedAndCache(request)
          .catch(() => { return offlineResponse(request) });
    });
}

function networkedOrOffline(request) {
  return fetch(request)
    .then((response) => {
      log('(network)', request.method, request.url);
      return response;
    })
    .catch(() => {
      return offlineResponse(request);
    });
}

function cachedOrOffline(request) {
  return caches
    .match(request)
    .then((response) => {
      return response || offlineResponse(request);
    });
}

function offlineResponse(request) {
  log('(offline)', request.method, request.url);
  if (request.url.match(/\.(jpg|png|gif|svg|jpeg)(\?.*)?$/)) {
    return caches.match('/offline.svg');
  } else {
    return caches.match('/offline.html');
  }
}

///////////
// Activate
///////////
function onActivate(event) {
  log('activate event in progress.');
  event.waitUntil(removeOldCache());
}

function removeOldCache() {
  return caches
    .keys()
    .then((keys) => {
      return Promise.all( // We return a promise that settles when all outdated caches are deleted.
        keys
         .filter((key) => {
           return !key.startsWith(version); // Filter by keys that don't start with the latest version prefix.
         })
         .map((key) => {
           return caches.delete(key); // Return a promise that's fulfilled when each outdated cache is deleted.
         })
      );
    })
    .then(() => {
      log('removeOldCache completed.');
    });
}

function cacheKey() {
  return [version, ...arguments].join(':');
}

function log() {
  if (developmentMode()) {
    console.log("SW:", ...arguments);
  }
}

function shouldAlwaysFetch(request) {
  return __DEVELOPMENT__ ||
    request.method !== 'GET' ||
      ignoreFetch.some(regex => request.url.match(regex));
}

function shouldFetchAndCache(request) {
  return ~request.headers.get('Accept').indexOf('text/html');
}

function developmentMode() {
  return __DEVELOPMENT__ || __DEBUG__;
}

log("Hello from ServiceWorker land!", version);

self.addEventListener('install', onInstall);

self.addEventListener('fetch', onFetch);

self.addEventListener("activate", onActivate);

___
<div style="width:690.45px"><div style="display:inline-block;width:110px"><a rel="external" href="http://creativecommons.org/licenses/by/2.5/cn/" target="_blank"><img style="border-width:0" src="https://i.creativecommons.org/l/by/2.5/cn/88x31.png"></a></div><div style="display:inline-block;width:580px;">
    本文采用<a rel="external" href="http://creativecommons.org/licenses/by/2.5/cn/" target="_blank">知识共享署名 2.5 中国大陆许可协议</a>进行许可，欢迎转载，但转载请注明来自[Agehua’s Blog](https://agehua.github.io/)，并保持转载后文章内容的完整。本人保留所有版权相关权利。</div></div>

本文链接：http://agehua.github.io/sw.js