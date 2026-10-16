import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import vm from "node:vm";

const source = new URL("../../scripts/dashboard/app.js", import.meta.url);
const htmlPath = new URL("../../scripts/dashboard/index.html", import.meta.url);
const samplePath = new URL("./data/dashboard-sample.json", import.meta.url);
const app = readFileSync(source, "utf8").replace(/\nrefresh\(\);\s*$/, "\n");
const html = readFileSync(htmlPath, "utf8");
const sample = JSON.parse(readFileSync(samplePath, "utf8"));

class Element {
  constructor() {
    this.textContent = "";
    this.dataset = {};
    this.attributes = new Map();
    this.children = [];
    this.value = 0;
    this.hidden = false;
  }

  setAttribute(name, value) { this.attributes.set(name, value); }
  append(...children) { this.children.push(...children); }
  replaceChildren(...children) { this.children = [...children]; }
}

const ids = [...html.matchAll(/\bid="([^"]+)"/g)].map((match) => match[1]);
const elements = new Map(ids.map((id) => [id, new Element()]));
const document = {
  getElementById(id) { return elements.get(id) ?? null; },
  createElement() { return new Element(); },
};
const state = (robotStatus, receivedAt) => ({
  broker: "127.0.0.1", port: 1883, root: "robot/dashboard-check/test",
  test_feed: true, broker_connected: true, connection_note: "Connected",
  robot_status: robotStatus, received_at: receivedAt, elapsed_ms: 20,
  heartbeat_age_ms: 10, messages: 5, rejected: 0, telemetry: sample,
  mapping: null, histories: {},
});
const responses = [state("live", 10000), state("stale", 10000),
  state("live", 10001), new Error("HTTP server stopped"), state("live", 10002)];
const context = {
  document,
  fetch: async () => {
    const response = responses.shift();
    if (response instanceof Error) throw response;
    return { ok: true, json: async () => response };
  },
  AbortSignal: { timeout: () => ({}) },
  window: { setTimeout() {} },
};
vm.runInNewContext(app, context, { filename: "app.js" });

const text = (id) => elements.get(id).textContent;
const currentFields = [
  "travel-speed", "front-distance", "left-speed", "right-speed", "wheel-distance",
  "heading", "line-error", "ir-left", "ir-right", "ir-raw",
  "pitch", "roll", "current-height", "turn-rate",
];
const assertCleared = () => currentFields.forEach((id) => assert.equal(text(id), "—", id));

await context.refresh();
assert.equal(text("robot-status"), "Robot live");
assert.equal(text("travel-speed"), "118");
assert.equal(text("mission-state"), "Follow line");
assert.equal(text("imu-calibration"), "Complete");

await context.refresh();
assert.equal(text("robot-status"), "Readings stale");
assertCleared();
assert.equal(text("motion-action"), "No fresh reading");
assert.equal(text("mission-state"), "Follow line · last reported");
assert.equal(text("imu-calibration"), "Complete · last reported");
assert.equal(text("scan-status"), "Complete · last reported");
assert.equal(text("max-height"), "38");
assert.equal(text("obstacle-width"), "120 mm");
assert.equal(text("barcode-count"), "3");
assert.equal(text("hump-count"), "2");
assert.equal(elements.get("event-list").children.length, 2);

await context.refresh();
assert.equal(text("robot-status"), "Robot live");
assert.equal(text("mission-state"), "Follow line");
assert.equal(text("imu-calibration"), "Complete");
assert.equal(text("travel-speed"), "118");

await context.refresh();
assert.equal(text("robot-status"), "Dashboard offline");
assertCleared();
assert.equal(text("motion-action"), "No fresh reading");
assert.equal(text("front-note"), "No fresh valid sonar reading");
assert.equal(text("line-condition"), "No fresh reading");
assert.equal(text("movement"), "No fresh reading");
assert.equal(text("mission-state"), "Follow line · last reported");
assert.equal(text("mission-reason"), "None · last reported");
assert.equal(text("imu-samples"), "100 · last reported");
assert.equal(text("max-height"), "38");
assert.equal(text("obstacle-width"), "120 mm");
assert.equal(text("barcode-count"), "3");
assert.equal(text("hump-count"), "2");
assert.equal(elements.get("ir-left-meter").attributes.get("aria-valuetext"), "No fresh reading");
assert.equal(elements.get("event-list").children.length, 2);
assert.match(text("raw-payload"), /"speed_mm_s": \[\s*120,\s*116\s*\]/);
assert.match(html, /Last received MQTT payload/);

await context.refresh();
assert.equal(text("robot-status"), "Robot live");
assert.equal(text("mission-state"), "Follow line");
assert.equal(text("imu-samples"), "100");
assert.equal(text("travel-speed"), "118");
console.log("[WEB] Dashboard stale/disconnect/recovery UI PASS");
