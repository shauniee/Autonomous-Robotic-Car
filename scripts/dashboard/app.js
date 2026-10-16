"use strict";

const byId = (id) => document.getElementById(id);
const put = (id, value) => { byId(id).textContent = value; };
const number = (value) => typeof value === "number" && Number.isFinite(value);
const fmt = (value, unit = "", digits = 0) => number(value) ? `${value.toFixed(digits)}${unit}` : "—";
const pair = (value, unit = "") => Array.isArray(value) && value.length === 2 && value.every(number)
  ? `${fmt(value[0])} / ${fmt(value[1])}${unit}` : "—";
const readable = (value) => typeof value === "string" && value ? value.replaceAll("_", " ").toLowerCase().replace(/^./, (c) => c.toUpperCase()) : "Waiting";
const lastReported = (value) => value && value !== "Waiting" && value !== "—" &&
  !value.endsWith(" · last reported") ? `${value} · last reported` : value;
const feedLabel = (value, live) => live ? value : lastReported(value);
const enums = { scan: ["Idle", "Scanning", "Complete", "Failed", "Cancelled"],
  calibration: ["Not started", "Calibrating", "Complete", "Failed", "Cancelled"],
  action: ["Idle", "Active", "Complete", "Failed"],
  navigation: ["Unmapped", "Left", "Right", "Straight", "U-turn"] };
const label = (group, value) => enums[group][value] ?? "Waiting";
let lastKey = null;
let lastRun = null;
let points = [];
let encounters = new Map();

function chart() {
  const values = points.flatMap((p) => [p.left, p.right]).filter(number);
  const max = values.length ? Math.max(100, Math.ceil(Math.max(...values.map(Math.abs)) / 50) * 50) : 100;
  const path = (key) => {
    let drawing = false;
    return points.map((p, i) => {
      if (!number(p[key])) { drawing = false; return ""; }
      const x = 42 + (i / 59) * 624;
      const y = 95 - (p[key] / max) * 71;
      const command = drawing ? "L" : "M";
      drawing = true;
      return `${command}${x.toFixed(1)},${y.toFixed(1)}`;
    }).join(" ");
  };
  byId("left-trace").setAttribute("d", path("left"));
  byId("right-trace").setAttribute("d", path("right"));
  put("axis-max", values.length ? max : "—");
  put("axis-min", values.length ? -max : "—");
  byId("chart-empty").hidden = values.length > 1;
}

function addEncounter(kind, record) {
  if (!Array.isArray(record) || record.length !== 7 || !record.every(number)) return;
  const [id, sampledAt, revision, a, b, value, detail] = record;
  if (id < 1 || id > 4294967295) return;
  const key = `${kind}:${id}`;
  if (encounters.has(key)) return;
  encounters.set(key, {kind, id, sampledAt, revision, a, b, value, detail});
  while (encounters.size > 16) encounters.delete(encounters.keys().next().value);
}

function renderEvents(s, data) {
  const run = s.mission?.run;
  if (run !== lastRun) {
    lastRun = run;
    encounters.clear();
    points = [];
    lastKey = null;
  }
  put("run-id", number(run) ? `Run ${run}` : "No run yet");
  const events = s.events ?? {};
  put("barcode-count", fmt(events.barcodes));
  put("hump-count", fmt(events.humps));
  put("mapping-revision", fmt(s.barcode?.revision));
  addEncounter("barcode", events.latest_barcode);
  addEncounter("hump", events.latest_hump);
  for (const kind of ["barcode", "hump"]) {
    const history = data.histories?.[kind];
    if (history?.run === run) history.events?.forEach((record) => addEncounter(kind, record));
  }
  const list = byId("event-list");
  list.replaceChildren();
  if (encounters.size === 0) {
    const empty = document.createElement("p");
    empty.className = "empty-copy";
    empty.textContent = "Accepted barcode and hump encounters will appear here.";
    list.append(empty);
  }
  for (const event of [...encounters.values()].reverse().slice(0, 8)) {
    const row = document.createElement("div"); row.className = "event-row";
    const symbol = document.createElement("span"); symbol.className = "event-symbol";
    const text = document.createElement("div"); text.className = "event-text";
    const title = document.createElement("strong");
    const note = document.createElement("span");
    const id = document.createElement("span"); id.className = "event-id"; id.textContent = `#${event.id}`;
    if (event.kind === "barcode") {
      const letter = event.value >= 65 && event.value <= 90 ? String.fromCharCode(event.value) : "?";
      symbol.textContent = letter;
      title.textContent = `Barcode ${letter} · ${label("navigation", event.detail >>> 1)}`;
      note.textContent = `Mapping revision ${event.revision}`;
    } else {
      symbol.textContent = "⌃";
      title.textContent = `Hump · ${fmt(event.value, " mm")}`;
      note.textContent = `Position ${fmt(event.a)} / ${fmt(event.b)} mm`;
    }
    text.append(title, note); row.append(symbol, text, id); list.append(row);
  }
  const dropped = Array.isArray(events.dropped) ? events.dropped.reduce((sum, n) => sum + (number(n) ? n : 0), 0) : 0;
  put("event-note", events.overflow ? "A producer counter saturated. Consult the saved MQTT log." :
    dropped ? `${dropped} older records evicted from robot history. This page lists received encounters.` :
    "Latest received encounters · retained for this browser session and run");
}

function render(data) {
  const broker = data.broker_connected === true;
  const state = data.robot_status;
  const status = byId("robot-status"); status.dataset.state = state;
  status.textContent = ({live: "Robot live", stale: "Readings stale", offline: "Robot offline", waiting: "Waiting for robot"})[state] ?? "Connecting";
  const brokerStatus = byId("broker-status"); brokerStatus.dataset.connected = broker;
  brokerStatus.textContent = broker ? "Broker connected" : "Broker reconnecting";
  put("broker-address", `${data.broker}:${data.port}`);
  put("updated", data.elapsed_ms === null ? "Waiting for readings" : `Last snapshot ${(data.elapsed_ms / 1000).toFixed(1)}s ago`);
  put("message-count", `${data.messages} messages received`);
  put("feed-info", `${data.root} · updates automatically`);
  const notice = byId("notice"); notice.dataset.live = state === "live";
  put("notice-text", data.test_feed ? "Synthetic test feed for dashboard verification. These are not measured robot results." :
    !broker ? `${data.connection_note}. Start Mosquitto and keep this dashboard terminal open.` :
    !data.telemetry ? "Broker connected. Waiting for the Pico to publish its first live reading." :
    state === "live" ? `Receiving live robot data. ${data.telemetry.mission?.test_inhibited ? "TEST firmware is active; motor outputs are inhibited." : "Readings update automatically."}` :
    "Live robot data is unavailable. Current measurements are hidden as they expire; accepted run results are retained.");
  const s = data.telemetry ?? {};
  // Show the latest reported sample between 500 ms MQTT publications. Its wire
  // age/validity is authoritative; a stale dashboard feed never becomes live.
  const fresh = (group, limit) => state === "live" && number(group?.age_ms) &&
    group.age_ms >= 0 && group.age_ms <= limit;
  const motion = s.motion ?? {}, line = s.line ?? {}, terrain = s.terrain ?? {}, obstacle = s.obstacle ?? {};
  const motionOK = fresh(motion, 60) && motion.valid === true;
  const lineOK = fresh(line, 60) && line.valid === true;
  const terrainOK = fresh(terrain, 250) && terrain.valid === true;
  const frontOK = fresh(obstacle, 250) && number(obstacle.front_mm);
  put("mission-state", s.mission ? feedLabel(readable(s.mission.state), state === "live") : "Waiting");
  put("mission-reason", s.mission ? `${readable(s.mission.reason)}${state !== "live" ? " · last reported" : ""}` : "The first reading will appear here.");
  const speed = motionOK && Array.isArray(motion.speed_mm_s) && motion.speed_mm_s.length === 2 && motion.speed_mm_s.every(number) ? motion.speed_mm_s : null;
  put("travel-speed", speed ? fmt((speed[0] + speed[1]) / 2) : "—");
  put("left-speed", speed ? fmt(speed[0]) : "—"); put("right-speed", speed ? fmt(speed[1]) : "—");
  put("front-distance", frontOK ? fmt(obstacle.front_mm) : "—");
  put("front-note", frontOK ? "Fresh forward sonar return" : "No fresh valid sonar reading");
  put("max-height", terrain.run === s.mission?.run ? fmt(terrain.max_height_mm) : "—");
  put("wheel-distance", motionOK ? pair(motion.distance_mm, " mm") : "—");
  put("heading", motionOK && number(motion.yaw_mdeg) ? fmt(motion.yaw_mdeg / 1000, "°", 1) : "—");
  put("motion-action", motionOK ? label("action", motion.status) : "No fresh reading");
  put("line-condition", lineOK ? readable(line.condition) : "No fresh reading");
  for (const [index, side] of ["left", "right"].entries()) {
    const value = lineOK && Array.isArray(line.fraction) ? line.fraction[index] : null;
    put(`ir-${side}`, number(value) ? fmt(value * 100, "% black") : "—");
    const meter = byId(`ir-${side}-meter`); meter.value = number(value) ? Math.max(0, Math.min(1, value)) : 0;
    meter.setAttribute("aria-invalid", !number(value));
    meter.setAttribute("aria-valuetext", number(value) ? fmt(value * 100, "% black") : "No fresh reading");
  }
  put("line-error", lineOK ? fmt(line.error, "", 3) : "—");
  put("ir-raw", fresh(line, 60) && line.raw ? pair([line.raw.left, line.raw.right]) : "—");
  put("ir-calibration", s.line ? feedLabel(readable(line.capture), state === "live") : "Waiting");
  put("pitch", terrainOK && number(terrain.pitch_mdeg) ? fmt(terrain.pitch_mdeg / 1000, "°", 1) : "—");
  put("roll", terrainOK && number(terrain.roll_mdeg) ? fmt(terrain.roll_mdeg / 1000, "°", 1) : "—");
  put("current-height", terrainOK ? fmt(terrain.height_mm, " mm") : "—");
  put("turn-rate", terrainOK && motionOK && number(terrain.turn_mdeg_s) ? fmt(terrain.turn_mdeg_s / 1000, "°/s", 1) : "—");
  const movement = ["Stationary", "Accelerating", "Turning", "Climbing", "Descending", "Impact", "Moving"];
  put("movement", terrainOK && motionOK ? movement[terrain.movement] ?? "Unknown" : "No fresh reading");
  put("imu-calibration", s.calibration ?
    feedLabel(label("calibration", s.calibration.status), state === "live") : "Waiting");
  put("imu-samples", feedLabel(fmt(s.calibration?.samples), state === "live"));
  put("scan-status", s.obstacle ?
    feedLabel(label("scan", obstacle.scan_status), state === "live") : "Waiting");
  // A stationary profile is historical, independent of the forward reading.
  put("obstacle-width", obstacle.profile_valid === true ? fmt(obstacle.width_mm, " mm") : "—");
  put("obstacle-closest", obstacle.profile_valid === true ? fmt(obstacle.closest_mm, " mm") : "—");
  put("obstacle-gaps", obstacle.profile_valid === true ? pair(obstacle.gap_mm, " mm") : "—");
  renderEvents(s, data);
  const key = data.received_at;
  if (key !== null && key !== undefined && key !== lastKey) {
    // Wire validity is evaluated at reception for the history, not at browser poll time.
    const valid = motion.valid === true && number(motion.age_ms) && motion.age_ms <= 60 &&
      Array.isArray(motion.speed_mm_s) && motion.speed_mm_s.length === 2 && motion.speed_mm_s.every(number);
    points.push({left: valid ? motion.speed_mm_s[0] : null, right: valid ? motion.speed_mm_s[1] : null});
    if (points.length > 60) points.shift();
    lastKey = key;
  }
  chart();
  put("raw-payload", data.telemetry ? JSON.stringify(data.telemetry, null, 2) : "Waiting for the first live robot snapshot.");
}

function showDisconnected() {
  const status = byId("robot-status");
  status.dataset.state = "offline";
  status.textContent = "Dashboard offline";
  const brokerStatus = byId("broker-status");
  brokerStatus.textContent = "Dashboard disconnected";
  brokerStatus.dataset.connected = false;
  byId("notice").dataset.live = false;
  put("notice-text", "The dashboard connection was lost. Restart the dashboard command in your terminal.");
  const liveFields = ["travel-speed", "front-distance", "left-speed", "right-speed",
    "wheel-distance", "heading", "line-error", "ir-left",
    "ir-right", "ir-raw", "pitch", "roll", "current-height", "turn-rate"];
  for (const id of liveFields) put(id, "—");
  put("motion-action", "No fresh reading");
  put("front-note", "No fresh valid sonar reading");
  put("line-condition", "No fresh reading");
  put("movement", "No fresh reading");
  for (const id of ["mission-state", "mission-reason", "ir-calibration", "imu-calibration", "imu-samples", "scan-status"]) {
    put(id, lastReported(byId(id).textContent));
  }
  for (const side of ["left", "right"]) {
    const meter = byId(`ir-${side}-meter`);
    meter.value = 0;
    meter.setAttribute("aria-invalid", true);
    meter.setAttribute("aria-valuetext", "No fresh reading");
  }
}

async function refresh() {
  try {
    const response = await fetch("/api/state", {cache: "no-store", signal: AbortSignal.timeout(4000)});
    if (!response.ok) throw new Error("Dashboard unavailable");
    render(await response.json());
  } catch (error) {
    showDisconnected();
  } finally {
    window.setTimeout(refresh, 500);
  }
}

refresh();
