#!/usr/bin/env node
// Unit tests of the actual workerurl() implementation and tools/platstamp.
// Run: node tools/test/workerurl.mjs
import assert from 'node:assert/strict';
import { createHash, webcrypto } from 'node:crypto';
import { execFileSync } from 'node:child_process';
import { readFileSync, mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';

const srcPath = 'sys/src/9/wasm32/platform.js';
const original = readFileSync(srcPath, 'utf8');
const expectedHash = createHash('sha256').update(original).digest('hex');
const tmp = mkdtempSync(join(tmpdir(), 'plan9-workerurl-'));
let stamped;
try {
	const target = join(tmp, 'platform.js');
	execFileSync('bash', ['tools/platstamp', srcPath, target]);
	stamped = readFileSync(target, 'utf8');
	assert.ok(stamped.includes("const PLATHASH = '" + expectedHash + "';"));
	assert.equal(stamped.replace("const PLATHASH = '" + expectedHash + "';", "const PLATHASH = '@PLATHASH@';"), original);
} finally {
	rmSync(tmp, { recursive: true, force: true });
}

const match = original.match(/async function workerurl\(\) \{[\s\S]*?\n\}/);
assert.ok(match, 'the real workerurl() must be present');
const body = match[0].replace('const me = import.meta.url;', 'const me = requestedURL;');
function fixture(responses, requestedURL = 'https://example.invalid/platform.js', hash = expectedHash) {
	const calls = [];
	const received = [];
	const fetch = async (_url, { cache }) => {
		calls.push(cache);
		const r = responses.shift();
		if (r instanceof Error) throw r;
		return r;
	};
	const URL = { createObjectURL(blob) { received.push(blob); return 'blob:checked'; } };
	const make = new Function('requestedURL', 'PLATHASH', 'fetch', 'crypto', 'TextEncoder', 'Blob', 'URL',
		body + '\nreturn workerurl;');
	return { workerurl: make(requestedURL, hash, fetch, webcrypto, TextEncoder, Blob, URL),
		calls, received };
}
const ok = text => ({ ok: true, status: 200, text: async () => text });
const httpError = status => ({ ok: false, status });
const badContent = stamped.replace('function cpu(', 'function broken_cpu(');
assert.notEqual(badContent, stamped);

const good = fixture([ok(stamped)]);
assert.equal(await good.workerurl(), 'blob:checked');
assert.deepEqual(good.calls, ['force-cache']);
assert.equal(await good.received[0].text(), stamped);

const retry = fixture([ok(badContent), ok(stamped)]);
assert.equal(await retry.workerurl(), 'blob:checked');
assert.deepEqual(retry.calls, ['force-cache', 'no-store']);

const bad = fixture([ok(badContent), ok(badContent)]);
await assert.rejects(bad.workerurl(), /SHA-256 mismatch/);
assert.equal(bad.received.length, 0);

const unavailable = fixture([httpError(404), httpError(500)]);
await assert.rejects(unavailable.workerurl(), /HTTP 500/);
assert.equal(unavailable.received.length, 0);

const network = fixture([new Error('network down'), new Error('network down')]);
await assert.rejects(network.workerurl(), /network down/);
assert.equal(network.received.length, 0);

const unstamped = fixture([], 'https://example.invalid/platform.js', '@PLATHASH@');
await assert.rejects(unstamped.workerurl(), /not SHA-256 stamped/);
assert.equal(unstamped.calls.length, 0);

const disk = fixture([], 'blob:firmware', '@PLATHASH@');
assert.equal(await disk.workerurl(), 'blob:firmware');
assert.equal(disk.calls.length, 0);

console.log('workerurl: 7 checks passed');
