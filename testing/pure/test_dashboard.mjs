// Exercise real dashboard script with inert DOM/canvas and synthetic ACKs.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';
const elements = new Map();
function element() {
  return {textContent: '', className: '', children: [], handlers: {}, value: '',
    classList: {contains: () => false, toggle() {}}, style: {setProperty() {}},
    addEventListener(name, fn) { this.handlers[name] = fn; },
    cells: new Map(), attributes: {}, setAttribute(name, value) { this.attributes[name] = String(value); }, querySelector(selector) { if (!this.cells.has(selector)) this.cells.set(selector, element()); return this.cells.get(selector); },
    querySelectorAll() { return []; }, replaceChildren() {}, prepend() {},
    append() {}, getContext() { return new Proxy({}, {get: () => () => {}}); },
    clientWidth: 600, clientHeight: 220};
}
const get = id => {
  if (!elements.has(id)) elements.set(id, element());
  return elements.get(id);
};
const buttons = [1, 2, 3, 4].map(action => ({dataset: {action}, disabled: true}));
get('directions').querySelectorAll = () => buttons;
get('map').querySelectorAll = () => ['A', 'B', 'C', 'D'].map(value => ({...element(), value}));
const timers = new Map();
let timer = 0, request;
const context = vm.createContext({
  document: {getElementById: get, createElement: element, body: element(), documentElement: element()},
  Date, Set, JSON, Number, String, Math, devicePixelRatio: 1,
  matchMedia: () => ({matches: true}), getComputedStyle: () => ({getPropertyValue: () => ''}),
  setInterval() {}, setTimeout(fn) { timers.set(++timer, fn); return timer; },
  clearTimeout(id) { timers.delete(id); }, addEventListener() {}, EventSource: class {},
  fetch: async (url, options) => {
    request = {url, ...JSON.parse(options.body)};
    // ACK can arrive on SSE before the HTTP response returns its sequence.
    vm.runInContext('showAck({sequence: 17, ok: true, text: "ACK OK"})', context);
    return {ok: true, json: async () => ({sequence: 17})};
  }
});
const html = fs.readFileSync(new URL('../../scripts/dashboard.html', import.meta.url), 'utf8');
vm.runInContext(html.match(/<script>([\s\S]*?)<\/script>/)[1], context);
assert(buttons.every(b => b.disabled));
vm.runInContext('lastFrame = Date.now(); mission = "FOLLOW_LINE"; robotOnline = true; gate()', context);
assert(buttons.every(b => !b.disabled));
await get('directions').handlers.click({target: {closest: () => buttons[2]}});
assert.deepEqual(request, {url: '/direction', action: 3});
assert.match(get('direction-status').textContent, /Straight accepted/);
assert.equal(timers.size, 0);
vm.runInContext('mission = "NAVIGATE"; gate()', context);
assert(buttons.every(b => b.disabled));
vm.runInContext('mission = "FOLLOW_LINE"; lastFrame = Date.now() - 3000; gate()', context);
assert(buttons.every(b => b.disabled));
console.log('dashboard: PASS (live/state gating, request action, early ACK race)');

// The sensor ledger must preserve zero, distinguish invalid/missing values,
// and identify retained results without inventing raw measurements.
context.document.body.classList.remove = () => {};
const sample = {
  sequence: 0, mission: 'STOPPED', motion: 'IDLE', action_id: 0,
  line: 'VISIBLE', faults: [], mapped: true, left: 65535, right: 0,
  speed_left: 0, speed_right: -12, direction: 'reverse',
  barcode: 'A', barcode_action: 'LEFT', front_mm: 0,
  obstacle_width_mm: 120, obstacle_depth_mm: 0,
  hump_count: 0, hump_max_mm: 0
};
context.sample = sample;
vm.runInContext('showTelemetry(sample)', context);
assert.equal(get('sensor-speed-left').textContent, '0 mm/s');
assert.equal(get('sensor-speed-right').textContent, '-12 mm/s');
assert.equal(get('sensor-front').textContent, '0 mm');
assert.equal(get('sensor-front-valid').textContent, 'Valid echo');
assert.equal(String(get('sensor-count-left').textContent), '65535');
assert.equal(get('sensor-width').textContent, '120 mm');
assert.equal(get('sensor-depth').textContent, 'No result');
assert.equal(get('sensor-height').textContent, '0 mm');
assert.equal(get('sensor-barcode').textContent, 'A');
assert.match(get('sensor-frame').textContent, /STOPPED.*IDLE.*Action ID 0/);
Object.assign(sample, {speed_left: null, speed_right: null, front_mm: null,
  hump_max_mm: null, barcode: '-', mapped: false});
vm.runInContext('showTelemetry(sample)', context);
assert.equal(get('sensor-speed-left').textContent, 'Awaiting sample');
assert.equal(get('sensor-front-valid').textContent, 'No valid echo');
assert.equal(get('sensor-height').textContent, 'No valid maximum');
assert.equal(get('sensor-barcode').textContent, 'None decoded');
assert.equal(get('sensor-mapped').textContent, 'No');
vm.runInContext('lastFrame = Date.now() - 3000; sensorFreshness()', context);
assert.match(get('sensor-freshness').textContent, /stale.*last received/);
vm.runInContext('lastFrame = 0; sensorFreshness()', context);
assert.equal(get('sensor-freshness').textContent, 'No readings received.');
console.log('sensor ledger: PASS (valid zero, missing values, units, barcode, frame context, stale/empty)');

// Every firmware Mission state has one current marker, including STOPPED.
for (const state of ['READY', 'FOLLOW_LINE', 'NAVIGATE', 'SCAN', 'BYPASS', 'REACQUIRE', 'STOPPED']) {
  sample.mission = state;
  vm.runInContext('showTelemetry(sample)', context);
  assert.equal(get('mission-current').textContent, state);
  const current = [...elements].filter(([id, el]) => id.startsWith('mission-') && el.attributes['aria-current'] === 'true');
  assert.deepEqual(current.map(([id]) => id), ['mission-' + state]);
}
vm.runInContext('mission = "READY"; robotOnline = true; lastFrame = Date.now(); gate()', context);
assert.equal(get('send').disabled, false);
vm.runInContext('inputs[1].value = "A"; gate()', context);
assert.equal(get('send').disabled, true);
vm.runInContext('inputs[1].value = "B"; robotOnline = false; gate()', context);
assert.equal(get('send').disabled, true);
vm.runInContext('robotOnline = true; gate()', context);
await get('map').handlers.submit({preventDefault() {}});
assert.deepEqual(request, {url: '/map', letters: 'ABCD'});
assert.match(get('ackhead').textContent, /Installed/);
assert.match(get('ackmsg').textContent, /A \/ B \/ C \/ D/);
assert.equal(timers.size, 0);
console.log('one-screen dashboard: PASS (seven Mission markers, custom mapping, duplicate/offline gating, early mapping ACK)');

assert.deepEqual([...html.matchAll(/<tr id="row-([^"]+)"/g)].map(m => m[1]),
  ['Network', 'Motion', 'Line', 'Terrain', 'Obstacle']);
const cell = (owner, name) => get('row-' + owner).querySelector('.' + name).textContent;
Object.assign(sample, {line:'VISIBLE', faults:[], direction:'straight', speed_left:0, speed_right:12, hump_max_mm:0});
vm.runInContext('showTelemetry(sample)', context);
assert.equal(cell('Motion','reading'), 'Forward');
assert.equal(cell('Motion','detail'), 'L 0 / R 12 mm/s');
assert.equal(cell('Line','detail'), 'Calibrated');
assert.equal(cell('Terrain','detail'), 'Calibrated');
for (const direction of ['reverse','left','right','stopped']) {
  sample.direction = direction;
  vm.runInContext('showTelemetry(sample)', context);
  assert.equal(cell('Motion','reading'), direction);
}
Object.assign(sample, {line:'UNKNOWN', hump_max_mm:null, direction:null, speed_left:null});
vm.runInContext('showTelemetry(sample)', context);
assert.equal(cell('Line','detail'), 'Calibration unknown');
assert.equal(cell('Terrain','detail'), 'Calibration unknown');
assert.equal(cell('Motion','reading'), 'Awaiting sample');
assert.equal(cell('Motion','detail'), 'Speed unavailable');
sample.faults = ['Line: IR invalid', 'Terrain: IMU calibration failed', 'Mission: line lost'];
vm.runInContext('showTelemetry(sample)', context);
assert.equal(cell('Line','detail'), 'Fault / not calibrated');
assert.equal(cell('Terrain','detail'), 'Fault / not calibrated');
assert.equal(get('mission-current').className, 'bad');
console.log('subsystem health: PASS (S1–S5 order, separate Mission faults, direction/speed, calibration evidence/fault/unknown)');
