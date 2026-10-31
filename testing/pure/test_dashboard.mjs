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
    querySelectorAll() { return []; }, replaceChildren() {}, prepend() {}, focus() {},
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
  document: {getElementById: get, createElement: element, body: element(), documentElement: element(), addEventListener() {}},
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
assert.equal(get('send').disabled, true);
assert.match(html, /<form id="map" novalidate>/);
assert(!html.includes('IR calibration ·'));
assert(!html.includes('press and hold GP20 through capture'));
assert(!html.includes('setupStep'));
console.log('setup: PASS (digital IR verification, encoder PID guidance, mapping directly available)');
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

// Motion previews only affect the schematic; encoder numbers remain live.
const previewButtons = ['left', 'right', 'straight', 'uturn'].map(motion => ({...element(), dataset: {motion}}));
get('motion-previews').querySelectorAll = () => previewButtons;
get('motion-live').id = 'motion-live';
get('motion-live').dataset = {};
for (const direction of ['straight', 'reverse', 'left', 'right', 'stopped', null, 'uturn']) {
  sample.direction = direction;
  vm.runInContext('showTelemetry(sample)', context);
  assert.equal(get('encoder-motion').attributes['data-motion'],
    direction && direction !== 'uturn' ? direction : 'stopped');
}
const previousRequest = request;
for (const button of previewButtons) {
  get('motion-previews').handlers.click({target: {closest: () => button}});
  assert.equal(get('encoder-motion').attributes['data-motion'], button.dataset.motion);
  assert.equal(get('encoder-motion').attributes['data-preview'], 'true');
  assert.equal(get('motion-source').textContent, 'Preview only');
  assert.equal(button.attributes['aria-pressed'], 'true');
  Object.assign(sample, {direction: 'right', speed_left: 0, speed_right: -12});
  vm.runInContext('showTelemetry(sample)', context);
  assert.equal(get('encoder-motion').attributes['data-motion'], button.dataset.motion);
  assert.equal(get('sensor-speed-left').textContent, '0 mm/s');
  assert.equal(get('sensor-speed-right').textContent, '-12 mm/s');
}
assert.equal(request, previousRequest);
get('motion-previews').handlers.click({target: {closest: () => get('motion-live')}});
assert.equal(get('encoder-motion').attributes['data-motion'], 'right');
assert.equal(get('encoder-motion').attributes['data-preview'], 'false');
assert.equal(get('motion-source').textContent, 'Live');
vm.runInContext('lastFrame = Date.now() - 3000; renderEncoder()', context);
assert.equal(get('encoder-motion').attributes['data-live'], 'false');
assert.equal(get('motion-source').textContent, 'Last reading');
vm.runInContext('lastFrame = Date.now(); robotOnline = false; renderEncoder()', context);
assert.equal(get('encoder-motion').attributes['data-live'], 'false');
vm.runInContext('robotOnline = true', context);
for (const direction of ['left', 'right', 'straight', 'uturn']) {
  assert(html.includes(`id="arrow-${direction}"`));
}
assert.match(html, /prefers-reduced-motion: reduce/);
assert.match(html, /aria-label="U-turn" title="U-turn" disabled><svg/);
console.log('encoder schematic: PASS (live directions, unknown/U-turn not inferred, four local previews, live readings retained, stale/offline pause, arrow accessibility, reduced-motion rule)');

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
Object.assign(sample, {line:'VISIBLE', faults:[], direction:'straight', speed_left:0, speed_right:12, hump_max_mm:0, imu_calibrated:true});
vm.runInContext('showTelemetry(sample)', context);
assert.equal(cell('Motion','reading'), 'Forward');
assert.equal(cell('Motion','detail'), 'L 0 / R 12 mm/s');
assert.equal(cell('Line','detail'), 'Digital status');
assert.equal(cell('Terrain','detail'), 'Calibrated');
for (const direction of ['reverse','left','right','stopped']) {
  sample.direction = direction;
  vm.runInContext('showTelemetry(sample)', context);
  assert.equal(cell('Motion','reading'), direction);
}
Object.assign(sample, {line:'UNKNOWN', hump_max_mm:null, imu_calibrated:null, direction:null, speed_left:null});
vm.runInContext('showTelemetry(sample)', context);
assert.equal(cell('Line','detail'), 'Reading unavailable');
assert.equal(cell('Terrain','detail'), 'Calibration unknown');
assert.equal(cell('Motion','reading'), 'Awaiting sample');
assert.equal(cell('Motion','detail'), 'Speed unavailable');
sample.faults = ['Line: IR invalid', 'Terrain: IMU calibration failed', 'Mission: line lost'];
vm.runInContext('showTelemetry(sample)', context);
assert.equal(cell('Line','detail'), 'Sensor fault');
assert.equal(cell('Terrain','detail'), 'Calibration unknown');
assert.equal(get('mission-current').className, 'bad');
// Calibration is reported independently of hump history and current faults.
for (const hump_max_mm of [null, 0, 24]) {
  for (const [imu_calibrated, expected] of [[true, 'Calibrated'], [false, 'Not calibrated'], [null, 'Calibration unknown'], [undefined, 'Calibration unknown']]) {
    for (const faults of [[], ['Terrain: IMU'], ['Terrain: IMU calibration failed']]) {
      Object.assign(sample, {hump_max_mm, imu_calibrated, faults});
      vm.runInContext('showTelemetry(sample)', context);
      assert.equal(cell('Terrain','detail'), expected);
      assert.equal(get('row-Terrain').className, faults.length ? 'bad' : imu_calibrated === true ? 'ok' : 'warn');
    }
  }
}
console.log('subsystem health: PASS (S1–S5 order, separate Mission faults, direction/speed, calibration evidence/fault/unknown)');

// Only completion events add history; telemetry repeats and scans cannot
// pair a new width with the retained depth of an earlier obstacle.
const obstacleList = get('obstacles');
obstacleList.append = li => {
  obstacleList.children.push(li);
  li.remove = () => obstacleList.children.splice(obstacleList.children.indexOf(li), 1);
};
const obstacleEvent = (id, depth) => {
  context.obstacleEvent = {event_id: id, obstacle_depth_mm: depth};
  vm.runInContext('showObstacle(obstacleEvent)', context);
};
Object.assign(sample, {mission:'BYPASS', obstacle_width_mm:25, obstacle_depth_mm:170});
vm.runInContext('showTelemetry(sample); showTelemetry(sample)', context);
assert.equal(obstacleList.children.length, 0);
obstacleEvent('first', 40);
obstacleEvent('first', 40);
assert.equal(obstacleList.children.length, 1);
assert.equal(obstacleList.children[0].value, 1);
assert.equal(obstacleList.children[0].textContent, '25mm × 40mm');
assert.equal(get('obstacle-empty').hidden, true);
assert.equal(obstacleList.hidden, false);
obstacleEvent('second', 40);
assert.equal(obstacleList.children.length, 2); // Separate, equally sized obstacle.
sample.mission = 'SCAN';
vm.runInContext('showTelemetry(sample)', context);
obstacleEvent('missing-width', 55);
assert.equal(obstacleList.children[2].textContent, 'Width unavailable × 55mm');
for (const depth of [0, null, -1]) obstacleEvent('invalid', depth);
assert.equal(obstacleList.children.length, 3);
for (let i = 0; i < 200; i++) obstacleEvent('extra-' + i, 40);
assert.equal(obstacleList.children.length, 200);
assert.equal(obstacleList.children[0].value, 4);
assert.equal(obstacleList.children.at(-1).value, 203);
assert(!html.includes('<time id="obstacle'));
console.log('obstacle history: PASS (completion-only, duplicates, equal sizes, missing width, numbering, 200-entry bound)');

// Release-speed diagnostics remain distinct from averaged telemetry and previews.
context.trace = {sampled_ms: 0xFFFFFFF0, target_left: 0, target_right: 50,
  measured_left: 0, measured_right: null, duty_left: 0, duty_right: null,
  inhibited: true, output_ok: false};
vm.runInContext('showWheelTrace(trace)', context);
assert.equal(get('trace-target-left').textContent, '0 mm/s');
assert.equal(get('trace-speed-left').textContent, '0 mm/s');
assert.equal(get('trace-speed-right').textContent, 'Unavailable');
assert.equal(get('trace-duty-left').textContent, '0%');
assert.equal(get('trace-duty-right').textContent, 'Unavailable');
assert.match(get('trace-status').textContent, /TEST: motors inhibited.*output failed/);
context.trace.sampled_ms = 4; // 20 ms, across uint32 wrap.
vm.runInContext('showWheelTrace(trace)', context);
assert(!get('trace-status').textContent.includes('sample gap'));
context.trace.sampled_ms = 64;
vm.runInContext('showWheelTrace(trace)', context);
assert.match(get('trace-status').textContent, /sample gap 60 ms/);
vm.runInContext('traceReceived = Date.now() - 3000; traceFreshness()', context);
assert.match(get('trace-status').textContent, /stale/);
console.log('wheel trace: PASS (valid zero, missing data, inhibit/failure, wrap, sample loss, staleness)');
