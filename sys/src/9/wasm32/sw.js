/*
 * The standalone PWA's service worker: an empty fetch handler is what
 * makes the page installable.  It caches nothing and lets every request
 * go to the network as usual, so updates and the disk's range requests
 * behave exactly as before.
 */
self.addEventListener('fetch', () => {});
