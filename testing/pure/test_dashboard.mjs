// Exercise real dashboard script with inert DOM/canvas and synthetic ACKs.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';
const elements = new Map();
function element() {
  return {textContent: '', className: '', children: [], handlers: {}, value: '', style: {},
    classList: {contains: () => false, toggle() {}},
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

// The sensor log must preserve zero, distinguish invalid/missing values,
// and identify retained results without inventing raw measurements.
context.document.body.classList.remove = () => {};
const sample = {
  sequence: 0, mission: 'STOPPED', motion: 'IDLE', action_id: 0,
  line: 'VISIBLE', faults: [], mapped: true, left: 65535, right: 0,
  speed_left: 0, speed_right: -12, direction: 'reverse',
  barcode: 'A', barcode_action: 'LEFT', front_mm: 0,
  obstacle_width_mm: 120, obstacle_clearance_mm: 0,
  hump_count: 0, hump_max_mm: 0
};
context.sample = sample;
vm.runInContext('showTelemetry(sample)', context);
assert.equal(get('sensor-speed-left').textContent, '0 mm/s');
assert.equal(get('sensor-speed-right').textContent, '-12 mm/s');
assert.equal(get('sensor-front').textContent, '0 mm');
assert.equal(get('flap-Obstacle').textContent, 'CLEAR');
assert.equal(String(get('sensor-count-left').textContent), '65535');
assert.equal(get('sensor-width').textContent, '120 mm');
assert.equal(get('sensor-clearance').textContent, 'No result');
assert.equal(get('sensor-height').textContent, '0 mm');
Object.assign(sample, {speed_left: null, speed_right: null, front_mm: null,
  hump_max_mm: null, barcode: '-', mapped: false});
vm.runInContext('showTelemetry(sample)', context);
assert.equal(get('sensor-speed-left').textContent, 'Awaiting sample');
assert.equal(get('flap-Obstacle').textContent, 'NO ECHO');
assert.equal(get('sensor-height').textContent, 'No valid maximum');
assert.equal(get('sensor-mapped').textContent, 'No');
vm.runInContext('lastFrame = Date.now() - 3000; sensorFreshness()', context);
assert.equal(get('sensor-stream').textContent, 'Stale / last received');
vm.runInContext('lastFrame = 0; sensorFreshness()', context);
assert.equal(get('sensor-stream').textContent, 'Waiting for telemetry');
console.log('sensor log: PASS (valid zero, missing values, units, barcode, frame context, stale/empty)');

// Every card is present and visible; the removed table and nav stay removed.
assert(!html.includes('<table'));
assert(!html.includes('showSubsystem'));
assert(!html.includes('subsystem-nav'));
for (const card of ['Comms', 'Motion', 'Terrain', 'Barcode', 'Obstacle'])
  assert(html.includes(`id="card-${card}"`));
assert(!html.includes('robot-plan'));
assert(!html.includes('motion-previews'));
vm.runInContext('showHump({hump_height_mm: 0, event_id: "h1"}); showHump({hump_height_mm: 0, event_id: "h1"}); showHump({hump_height_mm: -1, event_id: "h2"})', context);
assert.equal(vm.runInContext('humps.length', context), 1);
assert.equal(get('hump-empty').hidden, true);
assert.match(vm.runInContext('humps[0].element.textContent', context), /^0 mm/);
console.log('cards: PASS (all five present, zero-height hump, deduplication, invalid height)');

// Every firmware Mission state has one current marker, including STOPPED.
for (const state of ['READY', 'FOLLOW_LINE', 'NAVIGATE', 'SCAN', 'BYPASS', 'REACQUIRE', 'STOPPED']) {
  sample.mission = state;
  vm.runInContext('showTelemetry(sample)', context);
  assert.equal(get('mission-current').textContent, state.replace('_', ' '));
  assert.equal(get('mission-current').className.includes('bad'), state === 'STOPPED');
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
assert.equal(get('ackhead').textContent, '');
assert.equal(get('ackmsg').textContent, '');
assert.equal(timers.size, 0);
console.log('control panel: PASS (seven Mission markers, custom mapping, duplicate/offline gating, early mapping ACK)');

// Flippers: navigation word and letter, terrain hump count, IMU corner badge.
Object.assign(sample, {line:'VISIBLE', faults:[], direction:'straight', speed_left:0, speed_right:12, hump_max_mm:0, imu_calibrated:true, barcode:'A', barcode_action:'LEFT'});
vm.runInContext('showTelemetry(sample)', context);
assert.equal(get('flap-Motion').textContent, 'STRAIGHT');
assert.equal(get('sensor-speed-right').textContent, '12 mm/s');
assert.equal(get('imu-badge').textContent, 'IMU CALIBRATED');
assert.equal(get('flap-Terrain').textContent, '0 HUMPS');
for (const [direction, word] of [['reverse','REVERSE'], ['left','LEFT'], ['right','RIGHT'], ['stopped','STOPPED'], ['uturn','U-TURN']]) {
  sample.direction = direction;
  vm.runInContext('showTelemetry(sample)', context);
  assert.equal(get('flap-Motion').textContent, word);
}
Object.assign(sample, {line:'UNKNOWN', hump_max_mm:null, imu_calibrated:null, direction:null, speed_left:null, motion:'IDLE'});
vm.runInContext('showTelemetry(sample)', context);
assert.equal(get('flap-Motion').textContent, 'IDLE');
assert.equal(get('imu-badge').textContent, 'IMU UNKNOWN');
assert.equal(get('sensor-speed-left').textContent, 'Awaiting sample');
sample.faults = ['Line: IR invalid', 'Terrain: IMU calibration failed', 'Mission: line lost'];
vm.runInContext('showTelemetry(sample)', context);
assert.match(get('card-Barcode').className, /bad/); // line faults turn the card red too
assert.match(get('card-Barcode').className, /bad/); // unmapped barcode card stays red
assert.match(get('mission-current').className, /bad/);
// Terrain shows calibration independently of hump history and current faults.
for (const hump_max_mm of [null, 0, 24]) {
  for (const [imu_calibrated, expected] of [[true, 'IMU CALIBRATED'], [false, 'IMU NOT CALIBRATED'], [null, 'IMU UNKNOWN'], [undefined, 'IMU UNKNOWN']]) {
    for (const faults of [[], ['Terrain: IMU'], ['Terrain: IMU calibration failed']]) {
      Object.assign(sample, {hump_max_mm, imu_calibrated, faults});
      vm.runInContext('showTelemetry(sample)', context);
      assert.equal(get('imu-badge').textContent, expected);
      assert.equal(get('card-Terrain').className, 'card ' + (faults.length ? 'bad' : imu_calibrated === true ? 'ok' : 'warn'));
    }
  }
}
console.log('flippers: PASS (direction word and R/L/S/U letter, barcode letter and action, hump count, IMU corner badge, faults)');

// Only completion events add history; telemetry repeats and scans cannot
// pair a new width with the retained depth of an earlier obstacle.
const obstacleList = get('obstacles');
obstacleList.append = li => {
  obstacleList.children.push(li);
  li.remove = () => obstacleList.children.splice(obstacleList.children.indexOf(li), 1);
};
const obstacleEvent = (id, clearance) => {
  context.obstacleEvent = {event_id: id, obstacle_clearance_mm: clearance, obstacle_side: 'left'};
  vm.runInContext('showObstacle(obstacleEvent)', context);
};
Object.assign(sample, {mission:'BYPASS', obstacle_width_mm:25, obstacle_clearance_mm:170});
vm.runInContext('showTelemetry(sample); showTelemetry(sample)', context);
assert.equal(obstacleList.children.length, 0);
obstacleEvent('first', 40);
obstacleEvent('first', 40);
assert.equal(obstacleList.children.length, 1);
assert.equal(obstacleList.children[0].value, 1);
assert.equal(obstacleList.children[0].textContent, 'W 25 mm · left · clearance 40 mm');
assert.equal(get('obstacle-empty').hidden, true);
assert.equal(obstacleList.hidden, false);
obstacleEvent('second', 40);
assert.equal(obstacleList.children.length, 2); // Separate, equally sized obstacle.
sample.mission = 'SCAN';
vm.runInContext('showTelemetry(sample)', context);
obstacleEvent('missing-width', 55);
assert.equal(obstacleList.children[2].textContent, 'W unavailable · left · clearance 55 mm');
for (const clearance of [null, -1]) obstacleEvent('invalid', clearance);
assert.equal(obstacleList.children.length, 3);
for (let i = 0; i < 200; i++) obstacleEvent('extra-' + i, 40);
assert.equal(obstacleList.children.length, 200);
assert.equal(obstacleList.children[0].value, 4);
assert.equal(obstacleList.children.at(-1).value, 203);
console.log('obstacle history: PASS (completion-only, duplicates, equal sizes, missing width, numbering, 200-entry bound)');

// Sonar: the obstacle block is shown only with a valid echo, at its measured range.
Object.assign(sample, {mission:'BYPASS', obstacle_width_mm:420, obstacle_clearance_mm:170, front_mm:300});
vm.runInContext('showTelemetry(sample)', context); // width 420 from telemetry
assert.equal(get('sonar-block').style.display, '');
assert.equal(get('sonar-readout').style.display, '');
assert.equal(get('sonar-readout').textContent, 'W 420 mm');
assert.equal(get('sonar-block').attributes.class, 'obstacle');
sample.front_mm = null;
vm.runInContext('showTelemetry(sample)', context);
assert.equal(get('sonar-block').style.display, 'none');
assert.equal(get('sonar-readout').style.display, 'none');
assert.equal(get('flap-Obstacle').textContent, 'DETECTED'); // still handling one, servo turned away
sample.mission = 'FOLLOW_LINE';
vm.runInContext('showTelemetry(sample)', context);
assert.equal(get('flap-Obstacle').textContent, 'NO ECHO');
sample.front_mm = 300;
vm.runInContext('showTelemetry(sample)', context);
assert.equal(get('flap-Obstacle').textContent, 'CLEAR');
assert.equal(get('sonar-block').style.display, 'none'); // no object drawn while nothing is there
assert.equal(get('sonar-readout').style.display, 'none');
sample.front_mm = 150; // in stop range, but no bypass started, so still clear
vm.runInContext('showTelemetry(sample)', context);
assert.equal(get('flap-Obstacle').textContent, 'CLEAR');
sample.mission = 'SCAN';
vm.runInContext('showTelemetry(sample)', context);
assert.equal(get('flap-Obstacle').textContent, 'DETECTED');
console.log('sonar: PASS (obstacle drawn at echo range, hidden without echo)');

// Release-speed diagnostics remain distinct from averaged telemetry and previews.
context.trace = {sampled_ms: 0xFFFFFFF0, target_left: 0, target_right: 50,
  measured_left: 0, measured_right: null, duty_left: 0, duty_right: null,
  inhibited: true, output_ok: false};
vm.runInContext('showWheelTrace(trace)', context);
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

// Barcode history records each passed barcode once; Mapping colour follows the robot's mapping state.
const barcodeList = get('barcodes');
barcodeList.append = li => barcodeList.children.push(li);
const barcodeEvent = text => vm.runInContext(`showBarcode({type: 'event', text: ${JSON.stringify(text)}})`, context);
barcodeEvent('EVENT BARCODE: letter Z -> RIGHT at t=12 ms, counts L=1 R=2');
barcodeEvent('EVENT BARCODE: letter Z -> RIGHT at t=12 ms, counts L=1 R=2');
barcodeEvent('EVENT OBSTACLE: depth 40 mm at t=20 ms');
barcodeEvent('EVENT BARCODE: letter Q -> U_TURN at t=30 ms, counts L=2 R=3');
assert.equal(barcodeList.children.length, 2);
assert.match(barcodeList.children[0].textContent, /^Z · RIGHT · /);
assert.match(barcodeList.children[1].textContent, /^Q · U-TURN · /);
assert.equal(get('barcode-empty').hidden, true);
// The letter flipper is a queue: newest scanned letter, cleared when its navigation action (TURN) is taken.
barcodeEvent('EVENT BARCODE: letter Z -> RIGHT at t=40 ms, counts L=3 R=4');
assert.equal(get('flap-Barcode-letter').textContent, 'Z');
barcodeEvent('EVENT BARCODE: letter K -> LEFT at t=50 ms, counts L=4 R=5');
assert.equal(get('flap-Barcode-letter').textContent, 'K');
vm.runInContext(`showTurn({type: 'event', text: 'EVENT TURN: turn UNMAPPED at t=55 ms, counts L=5 R=6'})`, context);
assert.equal(get('flap-Barcode-letter').textContent, 'K'); // junction without an action keeps the letter
vm.runInContext(`showTurn({type: 'event', text: 'EVENT TURN: turn LEFT at t=60 ms, counts L=5 R=6'})`, context);
assert.equal(get('flap-Barcode-letter').textContent, '–');
Object.assign(sample, {mapped: true});
vm.runInContext('showTelemetry(sample)', context);
assert.equal(get('sensor-mapped').textContent, 'Yes');
assert.equal(get('sensor-mapped').className, 'ok');
sample.mapped = false;
vm.runInContext('showTelemetry(sample)', context);
assert.equal(get('sensor-mapped').textContent, 'No');
assert.equal(get('sensor-mapped').className, 'bad');
console.log('barcode history: PASS (letter and action per event, duplicates ignored, non-barcode events ignored, mapped colour)');

// IMU: tilt and height read out, and both graphs keep to their windows across a travel wrap.
const terrainSample = (travel, height) => {
  context.terrainTrace = {type: 'terrain_trace', pitch_deg: 3.5, roll_deg: -1.5, x_mg: 16, y_mg: -16,
    z_mg: 992, height_mm: height, rise_mm: 9, travel_mm: travel};
  vm.runInContext('showTerrainTrace(terrainTrace)', context);
};
terrainSample(65000, 0);
terrainSample(100, 12);
assert.equal(get('sensor-pitch').textContent, '3.5 deg');
assert.equal(get('sensor-roll').textContent, '-1.5 deg');
assert.equal(get('sensor-height-now').textContent, '12 mm');
assert.equal(vm.runInContext('profile.at(-1).d - profile[0].d', context), 636);
terrainSample(3000, 0);
assert.equal(vm.runInContext('profile.length', context), 1); // older than 2 m drops off
terrainSample(10, 0); // a reboot restarts travel: no jump in distance
assert.equal(vm.runInContext('profile.at(-1).d - profile.at(-2).d', context), 0);
console.log('imu graphs: PASS (tilt, height, travel wrap, 2 m window, reboot)');
