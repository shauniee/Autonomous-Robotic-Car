"use strict";

(() => {
  const TELEMETRY_STALE_MS = 1500;
  const HEARTBEAT_STALE_MS = 3000;
  const IR_FRESH_MS = 60;
  const MAX_POINTS = 120;
  const points = [];
  let observation = null;
  let observationAt = 0;
  let streamOpen = false;
  let lastSequence = null;
  const element = id => document.getElementById(id);
  const text = (id, value) => { element(id).textContent = value; };
  const finite = value => typeof value === "number" && Number.isFinite(value);
  const display = value => finite(value) ? String(value) : "—";
  const signed = value => finite(value) ? (value > 0 ? "+" : "") + value.toFixed(2) : "—";
  const percent = value => finite(value) && value >= 0 && value <= 0.35 ? (value * 100).toFixed(1) + "%" : "—";
  const human = value => typeof value === "string" && value ? value.toLowerCase().replaceAll("_", " ") : "unknown";
  const ageText = value => finite(value) ? (value < 1000 ? Math.round(value) + " ms ago" : (value / 1000).toFixed(1) + " s ago") : "—";

  function tag(id, label, tone = "neutral") {
    text(id, label);
    element(id).dataset.tone = tone;
  }

  function sensor(side, fractions, raw) {
    const fraction = fractions && finite(fractions[side]) && fractions[side] >= 0 && fractions[side] <= 1 ? fractions[side] : null;
    text(side + "-value", fraction === null ? "—" : signed(2 * fraction - 1));
    text(side + "-raw", "ADC " + display(raw && raw[side]));
    element(side + "-fill").style.width = fraction === null ? "0%" : (fraction * 100) + "%";
  }

  function addHistory(state) {
    const data = state.telemetry;
    if (!data || data.sequence === lastSequence || state.telemetry_age_ms > TELEMETRY_STALE_MS) return;
    lastSequence = data.sequence;
    const valid = data.sample_fresh === true && data.acquisition_valid === true;
    const fractions = valid ? data.fractions : null;
    points.push({time: Date.now(), left: fractions && finite(fractions.left) ? 2 * fractions.left - 1 : null,
      right: fractions && finite(fractions.right) ? 2 * fractions.right - 1 : null,
      error: valid && finite(data.error) ? data.error : null});
    if (points.length > MAX_POINTS) points.shift();
    drawChart();
  }

  function render() {
    const state = observation || {};
    const elapsed = observation ? performance.now() - observationAt : 0;
    const telemetryAge = finite(state.telemetry_age_ms) ? state.telemetry_age_ms + elapsed : null;
    const heartbeatAge = finite(state.heartbeat_age_ms) ? state.heartbeat_age_ms + elapsed : null;
    const broker = streamOpen && state.broker_connected === true;
    const live = broker && finite(heartbeatAge) && heartbeatAge <= HEARTBEAT_STALE_MS;
    const freshFeed = broker && finite(telemetryAge) && telemetryAge <= TELEMETRY_STALE_MS;
    const data = state.telemetry || {};
    const stateNames = {READY: "Ready to start", FOLLOW_LINE: "Following line", RECOVER_LINE: "Recovering line", STOPPED: "Stopped"};
    const hasData = typeof data.state === "string";
    text("state-title", hasData ? (stateNames[data.state] || human(data.state)) : "Waiting for data");
    text("state-reason", hasData ? human(data.reason) : "Keep this page open while the robot connects.");
    text("state-symbol", data.state === "FOLLOW_LINE" ? "↑" : data.state === "RECOVER_LINE" ? "↶" : data.state === "STOPPED" ? "■" : "◌");
    document.querySelector(".state-card").dataset.stale = String(hasData && !freshFeed);
    element("test-status").hidden = data.test_inhibited !== true;
    text("telemetry-age", ageText(telemetryAge));
    text("feed-status", !hasData ? "No observations yet" : freshFeed ? "Receiving new observations" : "Last observation · stale");
    text("broker-status", !streamOpen ? "Dashboard reconnecting…" : broker ? "Broker connected" : "Broker unavailable · retrying");
    tag("live-status", live ? "Robot live" : !hasData && heartbeatAge === null ? "Awaiting robot" : "Heartbeat unavailable", live ? "good" : hasData ? "warn" : "neutral");
    element("live-status").prepend(document.createElement("i"));
    text("dashboard-status", streamOpen ? "Local dashboard connected" : "Local dashboard reconnecting");

    const sampleUsable = data.sample_fresh === true && data.acquisition_valid === true;
    const fractions = sampleUsable ? data.fractions : null;
    sensor("left", fractions, data.raw);
    sensor("right", fractions, data.raw);
    text("sensor-error", fractions ? signed(data.error) : "—");
    text("sample-age", finite(data.sample_age_ms) ? data.sample_age_ms + " ms" : "—");
    tag("line-status", human(data.line), data.line === "visible" && sampleUsable ? "good" : hasData ? "warn" : "neutral");
    element("sensor-card").dataset.stale = String(!freshFeed || !sampleUsable);
    text("sample-validity", !hasData ? "Waiting for a calibrated sensor observation." : !freshFeed ? "Feed is stale. Values above are the last reported observation." : !sampleUsable ? "IR acquisition is invalid or stale at publication." : !fractions ? "Calibration or capture is not yet providing a valid reading." : "Valid calibrated reading at publication.");

    const requested = data.requested_duty || {};
    const motion = data.motion && typeof data.motion === "object" ? data.motion : null;
    const applied = motion && motion.applied_duty ? motion.applied_duty : {};
    text("requested-left", percent(requested.left));
    text("requested-right", percent(requested.right));
    text("applied-left", percent(applied.left));
    text("applied-right", percent(applied.right));
    ["left", "right"].forEach(side => {
      element("applied-" + side + "-bar").style.width = finite(applied[side]) && applied[side] >= 0 && applied[side] <= 0.35 ? (applied[side] / 0.35 * 100) + "%" : "0%";
    });
    const motionFresh = motion && finite(motion.age_ms) && motion.age_ms <= IR_FRESH_MS;
    tag("brake-status", !motion ? "Unknown" : !motionFresh ? "Report stale" : motion.braked === true ? "Brake output" : "PWM applied", motionFresh ? "good" : "neutral");
    element("motor-card").dataset.stale = String(!freshFeed || !motionFresh);
    text("stop-latch", "Stop latch: " + (!motion || typeof motion.stopped !== "boolean" ? "unknown" : motion.stopped ? "set" : "clear"));
    text("motion-age", "Output age at publish: " + (motion && finite(motion.age_ms) ? motion.age_ms + " ms" : "—"));

    const calibration = data.calibration || {};
    tag("calibration-status", !hasData ? "Unknown" : calibration.black_set && calibration.white_set ? "References set" : calibration.black_set ? "White pending" : "Black pending", calibration.black_set && calibration.white_set ? "good" : "neutral");
    ["left", "right"].forEach(side => {
      const black = calibration.black_set && calibration.black ? calibration.black[side] : null;
      const white = calibration.white_set && calibration.white ? calibration.white[side] : null;
      text("black-" + side, display(black));
      text("white-" + side, display(white));
      text("contrast-" + side, finite(black) && finite(white) ? String(Math.abs(black - white)) : "—");
    });
    text("capture-status", "Capture result: " + (hasData ? human(data.capture) : "waiting for data") + ".");
    text("heartbeat-age", ageText(heartbeatAge));
    text("reported-status", state.status && typeof state.status.online === "boolean" ? (state.status.online ? "Online" : "Offline") + (state.status_retained ? " · cached" : " · reported") : "—");
    text("ir-sequence", display(data.ir_sequence));
    const health = state.heartbeat || state.status || {};
    text("dropped-publish", display(health.dropped_publish));
  }

  function drawChart() {
    const canvas = element("history-chart");
    const rect = canvas.getBoundingClientRect();
    const ratio = window.devicePixelRatio || 1;
    canvas.width = Math.round(rect.width * ratio);
    canvas.height = Math.round(rect.height * ratio);
    const context = canvas.getContext("2d");
    context.scale(ratio, ratio);
    const left = 27, right = rect.width - 9, top = 12, bottom = rect.height - 25;
    const width = right - left, height = bottom - top;
    context.font = "9px -apple-system, sans-serif";
    context.fillStyle = "#8a958d";
    [-1, 0, 1].forEach(value => {
      const y = top + (1 - value) * height / 2;
      context.beginPath(); context.moveTo(left, y); context.lineTo(right, y);
      context.strokeStyle = "#e9ede7"; context.lineWidth = 1; context.stroke();
      context.fillText(value > 0 ? "+" + value : String(value), 2, y + 3);
    });
    context.fillText("Earlier", left, bottom + 19);
    context.fillText("Latest", right - 28, bottom + 19);
    const haveReadings = points.some(point => finite(point.left) || finite(point.right));
    element("chart-empty").hidden = haveReadings;
    if (!points.length) return;
    const start = Math.min(points[0].time, points[points.length - 1].time - 60000);
    const duration = Math.max(60000, points[points.length - 1].time - start);
    [["left", "#24806b"], ["right", "#ad7338"], ["error", "#647ab8"]].forEach(([key, color]) => {
      context.beginPath();
      let continuing = false;
      points.forEach(point => {
        if (!finite(point[key]) || point[key] < -1 || point[key] > 1) { continuing = false; return; }
        const x = left + (point.time - start) / duration * width;
        const y = top + (1 - point[key]) * height / 2;
        if (continuing) context.lineTo(x, y); else context.moveTo(x, y);
        continuing = true;
      });
      context.strokeStyle = color;
      context.lineWidth = key === "error" ? 1.2 : 1.8;
      context.setLineDash(key === "error" ? [4, 3] : []);
      context.stroke();
    });
    context.setLineDash([]);
  }

  const stream = new EventSource("/events");
  stream.onopen = () => { streamOpen = true; render(); };
  stream.onerror = () => { streamOpen = false; render(); };
  stream.addEventListener("snapshot", event => {
    try {
      const state = JSON.parse(event.data);
      if (!state || typeof state !== "object") return;
      observation = state;
      observationAt = performance.now();
      addHistory(state);
      render();
    } catch (_) { /* Malformed bridge events never replace a usable view. */ }
  });
  window.addEventListener("resize", drawChart);
  setInterval(render, 250);
  drawChart();
  render();
})();
