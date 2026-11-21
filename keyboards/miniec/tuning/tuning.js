const element = (id) => document.getElementById(id);
const status = element("status");
const settings = element("settings");
const keys = element("keys");
const keyButtons = new Map();
const keyLabelMode = element("key-label-mode");
const press = element("press");
const release = element("release");
const offsetScope = element("offset-scope");
const samplePlot = element("sample-plot");
const plotContext = samplePlot.getContext("2d");
const plotMax = element("plot-max");
const plotTicks = [element("sample-peak"), element("plot-tick-75"), element("plot-tick-50"), element("plot-tick-25")];
const plotWindowMs = 10000;
const plotColors = ["#f3c4d0", "#fae2a6", "#cce7c3", "#c7def3"];
const plotSamples = [];
let plotBaseline, plotPeak;
const hid = navigator.hid;
const commands = { info: 1, read: 2, apply: 3, save: 4, readSaved: 5, restore: 6 };
const header = [0x4d, 0x45, 1];
let device, pending, selected, info, pollTimer, poll;
let requestId = 0;
let busy = false;
let unsaved = false;
let idleRange, pressedRange;
const measureIdle = element("measure-idle");
const measurePressed = element("measure-pressed");
const applyRecommended = element("apply-recommended");
const measurementStatus = element("measurement-status");
const backupSettings = element("backup-settings");
const backupButton = element("backup");
const backupNote = element("backup-note");
const backupFile = element("backup-file");
const restoreButton = element("restore");
const backupStatus = element("backup-status");

function receive(event) {
  if (event.reportId !== 0 || event.data.byteLength !== 32 || !pending) return;
  const bytes = new Uint8Array(event.data.buffer, event.data.byteOffset, event.data.byteLength);
  if (header.some((value, index) => bytes[index] !== value)) return;
  if (bytes[3] !== pending.command || bytes[4] !== pending.id || bytes[31] !== 0xa5) return;
  if (bytes[5] === 0) pending.resolve(bytes.slice(6));
  else pending.reject(new Error("キーボードが設定を受け付けませんでした。"));
}

async function request(command, ...payload) {
  const id = requestId = (requestId + 1) & 0xff;
  const packet = new Uint8Array(32);
  packet.set([...header, command, id, ...payload]);
  let timeout;
  const reply = new Promise((resolve, reject) => {
    pending = { id, command, resolve, reject };
    timeout = setTimeout(() => reject(new Error("キーボードから応答がありません。")), 1200);
  });
  try {
    const [, data] = await Promise.all([device.sendReport(0, packet), reply]);
    return data;
  } finally {
    clearTimeout(timeout);
    pending = undefined;
  }
}

function updateKeyLabel(key) {
  const label = keyLabelMode.value === "position" ? key.dataset.position : (key.dataset.sample ?? "—");
  if (key.textContent !== label) key.textContent = label;
}

keyLabelMode.addEventListener("change", () => {
  for (const key of keys.children) updateKeyLabel(key);
});

function drawSamplePlot(now) {
  if (!plotContext) return;
  const { width, height } = samplePlot;
  const bandHeight = height / plotColors.length;
  plotColors.forEach((color, index) => {
    plotContext.fillStyle = color;
    plotContext.fillRect(0, index * bandHeight, width, bandHeight);
  });

  const range = plotPeak > plotBaseline ? plotPeak - plotBaseline : 1;
  const plotY = (value) => 2 + (1 - Math.max(0, value - plotBaseline) / range) * (height - 4);
  plotContext.beginPath();
  plotSamples.forEach(({ time, value }, index) => {
    const x = (1 - (now - time) / plotWindowMs) * (width - 2);
    const y = plotY(value);
    if (index === 0) plotContext.moveTo(x, y);
    else plotContext.lineTo(x, y);
  });
  plotContext.strokeStyle = "#172539";
  plotContext.lineWidth = 3;
  plotContext.stroke();
  if (plotSamples.length) {
    const y = plotY(plotSamples.at(-1).value);
    plotContext.fillStyle = "#172539";
    plotContext.fillRect(width - 4, y - 2, 4, 4);
  }
}

function updatePlotScale() {
  const range = Math.max(0, plotPeak - plotBaseline);
  plotMax.textContent = plotPeak === undefined ? "—" : String(range);
  plotTicks.forEach((tick, index) => {
    tick.textContent = range > 0 ? String(range * (4 - index) / 4) : "—";
  });
}

function recordPlotSample(value, baseline) {
  const now = performance.now();
  if (baseline !== plotBaseline || value > plotPeak) {
    plotBaseline = baseline;
    plotPeak = Math.max(plotPeak ?? value, value);
    updatePlotScale();
  }
  plotSamples.push({ time: now, value });
  while (plotSamples[0].time < now - plotWindowMs) plotSamples.shift();
  drawSamplePlot(now);
}

function resetPlot() {
  plotSamples.length = 0;
  plotBaseline = plotPeak = undefined;
  updatePlotScale();
  drawSamplePlot(0);
}

element("reset-plot").addEventListener("click", resetPlot);
resetPlot();

function render(index, [sample, baseline, pressOffset, releaseOffset], fillInputs = false) {
  const key = keyButtons.get(index);
  key.dataset.sample = String(sample);
  updateKeyLabel(key);
  if (index !== selected) return;

  element("sample").value = sample;
  element("baseline").value = baseline;
  recordPlotSample(sample, baseline);
  if (fillInputs) {
    press.value = pressOffset;
    release.value = releaseOffset;
  }
  const down = baseline + pressOffset + 1;
  const up = baseline + releaseOffset - 1;
  element("thresholds").textContent = down > 255
    ? `適用中の押下閾値は測定範囲外です（${down} 以上）。押下オフセットを下げてください。`
    : `適用中の判定　押下 ${down} 以上 ／ 解放 ${up} 以下`;
}

async function run(action) {
  if (busy) return;
  busy = true;
  try {
    await poll;
    await action();
  } catch (error) {
    status.textContent = error.message;
  } finally {
    busy = false;
  }
}

function forgetDevice() {
  clearInterval(pollTimer);
  pending?.reject(new Error("USB 接続が切れました。"));
  device?.removeEventListener("inputreport", receive);
  device = undefined;
  for (const key of keyButtons.values()) {
    delete key.dataset.sample;
    updateKeyLabel(key);
  }
  element("sample").value = "—";
  element("baseline").value = "—";
  element("thresholds").textContent = "—";
  resetPlot();
  clearMeasurements();
  backupFile.value = "";
  restoreButton.disabled = backupSettings.disabled = true;
  backupNote.hidden = true;
  backupStatus.textContent = "";
  settings.disabled = true;
  element("connect").disabled = false;
}

function select(button, index) {
  return run(async () => {
    const data = await request(commands.read, index);
    if (index !== selected) {
      clearMeasurements();
      resetPlot();
    }
    selected = index;
    for (const key of keys.children) {
      if (!key.disabled) key.setAttribute("aria-pressed", String(key === button));
    }
    render(index, data, true);
  });
}

async function loadLayout() {
  const sources = [
    ["./layout.json", (data) => data],
    ["../keyboard.json", (data) => data.layouts?.LAYOUT?.layout],
  ];
  for (const [url, extract] of sources) {
    try {
      const response = await fetch(url);
      if (!response.ok) continue;
      const layout = extract(await response.json());
      if (Array.isArray(layout) && layout.length > 0) return layout;
    } catch {
      // Try the other source if this server does not provide usable JSON here.
    }
  }
  throw new Error("キー配置を読み込めません。ページを再読み込みしてください。");
}

function renderLayout(layout, side, rows, columns) {
  keys.replaceChildren();
  keyButtons.clear();
  const unit = (value) => `calc(${value} * var(--key-unit))`;
  // Padding leaves room for the corners of rotated thumb keys.
  keys.style.width = unit(Math.max(...layout.map(({ x }) => x)) + 1.5);
  keys.style.height = unit(Math.max(...layout.map(({ y }) => y)) + 1.5);
  const positions = [...layout].sort((a, b) => a.y - b.y || a.x - b.x);
  let firstKey;
  for (const { matrix: [row, column], x, y, r = 0 } of positions) {
    const hand = Math.floor(row / rows);
    const localRow = row % rows;
    const index = localRow * columns + column;
    const key = document.createElement("button");
    key.type = "button";
    key.dataset.position = `${localRow + 1}–${column + 1}`;
    updateKeyLabel(key);
    key.style.left = unit(x + .28);
    key.style.top = unit(y + .28);
    if (r) key.style.transform = `rotate(${r}deg)`;
    key.disabled = hand !== side;
    key.setAttribute("aria-label", `${hand === 0 ? "左側" : "右側"} ${localRow + 1}行 ${column + 1}列`);
    if (!key.disabled) {
      keyButtons.set(index, key);
      const isFirst = firstKey === undefined;
      if (isFirst) firstKey = index;
      key.setAttribute("aria-pressed", String(isFirst));
      key.addEventListener("click", () => select(key, index));
    }
    keys.append(key);
  }
  if (firstKey === undefined) throw new Error("接続した側のキー配置が見つかりません。");
  return firstKey;
}

element("connect").addEventListener("click", () => run(async () => {
  const [chosen] = await hid.requestDevice({
    filters: [
      { vendorId: 0xfeed, productId: 0x0012, usagePage: 0xff60, usage: 0x61 },
    ],
  });
  if (!chosen) return;
  element("connect").disabled = true;
  device = chosen;
  try {
    await device.open();
    device.addEventListener("inputreport", receive);
    info = await request(commands.info);
    const [side, rows, columns] = info;
    if (side > 1 || rows !== 3 || columns !== 6) throw new Error("対応する miniEC ファームウェアを確認してください。");
    const sideName = side === 0 ? "左側" : "右側";
    element("side").textContent = sideName;
    selected = renderLayout(await loadLayout(), side, rows, columns);
    render(selected, await request(commands.read, selected), true);
    settings.disabled = false;
    const savedOffsetsAvailable = supportsSavedOffsets();
    backupSettings.disabled = false;
    backupFile.disabled = !savedOffsetsAvailable;
    backupNote.hidden = savedOffsetsAvailable;
    backupButton.textContent = savedOffsetsAvailable ? "保存済みの設定をダウンロード" : "現在の設定をダウンロード";
    unsaved = false;
    status.textContent = `${sideName}に接続しました。`;
    const activeKeys = [...keyButtons.keys()];
    let nextKey = 0;
    let pollCount = 0;
    pollTimer = setInterval(() => {
      if (busy || poll || !device?.opened) return;
      // Alternate live graph samples with updates to the rest of the layout.
      const index = pollCount++ % 2 === 0 ? selected : activeKeys[nextKey++ % activeKeys.length];
      poll = request(commands.read, index)
        .then((data) => render(index, data))
        .catch((error) => {
          if (device) status.textContent = error.message;
        })
        .finally(() => {
          poll = undefined;
        });
    }, 30);
  } catch (error) {
    forgetDevice();
    if (chosen.opened) await chosen.close();
    throw error;
  }
}));

function validOffsets(down, up) {
  return Number.isInteger(up) && Number.isInteger(down) && 1 <= up && up < down && down <= 254;
}

async function applyOffsets(down, up, allKeys = false) {
  if (!validOffsets(down, up)) {
    throw new Error("1 ≤ 解放オフセット < 押下オフセット ≤ 254 にしてください。");
  }
  const indices = allKeys ? [...keyButtons.keys()] : [selected];
  if (allKeys) status.textContent = "接続中の側の全キーに適用しています。";
  let applied = 0;
  try {
    for (const index of indices) {
      const data = await request(commands.apply, index, down, up);
      applied++;
      unsaved = true;
      render(index, data, true);
    }
  } catch (error) {
    if (!allKeys) throw error;
    throw new Error(
      `一括適用が中断しました。反映を確認できたキーは ${applied}/${indices.length} 個です。${error.message} 接続を確認してから、もう一度適用してください。`,
    );
  }
  const target = allKeys ? `接続中の側の全${applied}キー` : "選んだキー";
  status.textContent = `${target}に反映しました。電源を切る前に保存してください。`;
}

function clearMeasurements() {
  idleRange = pressedRange = undefined;
  measureIdle.disabled = false;
  measurePressed.disabled = true;
  applyRecommended.disabled = true;
  measurementStatus.textContent = "まず、キーを離した状態を測ります。";
}

function formatRange({ min, max }) {
  return min === max ? String(min) : `${min}〜${max}`;
}

function recommendedOffsets(baseline) {
  const gap = pressedRange.min - idleRange.max;
  // Keep a margin on both sides of the firmware's strict comparisons.
  if (gap < 8) {
    throw new Error("離した値と押した値の差が小さすぎます。深さを保って測り直してください。");
  }
  const up = idleRange.max + Math.floor(gap / 3) - baseline;
  const down = idleRange.max + Math.ceil(gap * 2 / 3) - baseline;
  if (up < 1 || down > 254) {
    throw new Error(
      "基準値に対して有効なオフセットを設定できません。キーを離したままUSBを抜き差しし、測り直してください。",
    );
  }
  return { down, up };
}

async function measureKey(pressed) {
  const index = selected;
  applyRecommended.disabled = measureIdle.disabled = measurePressed.disabled = true;
  pressedRange = undefined;
  if (!pressed) idleRange = undefined;
  measurementStatus.textContent = pressed
    ? "測定中。キーを押したままにしてください。"
    : "測定中。キーには触れないでください。";
  try {
    const samples = [];
    let baseline;
    for (let i = 0; i < 20; i++) {
      if (i > 0) await new Promise((resolve) => setTimeout(resolve, 35));
      if (!device?.opened) return;
      const data = await request(commands.read, index);
      samples.push(data[0]);
      baseline = data[1];
      render(index, data);
    }
    const range = { min: Math.min(...samples), max: Math.max(...samples) };
    if (!pressed) {
      idleRange = range;
      measurementStatus.textContent = `離した値 ${formatRange(range)}。次に押した状態を測ります。`;
      return;
    }
    pressedRange = range;
    const ranges = `離した値 ${formatRange(idleRange)} ／ 押した値 ${formatRange(pressedRange)}。`;
    try {
      const { down, up } = recommendedOffsets(baseline);
      measurementStatus.textContent = `${ranges} 推奨オフセットは押下 ${down}、解放 ${up} です。`;
      applyRecommended.disabled = false;
    } catch (error) {
      measurementStatus.textContent = `${ranges} ${error.message}`;
    }
  } catch (error) {
    if (device) measurementStatus.textContent = error.message;
  } finally {
    measureIdle.disabled = false;
    measurePressed.disabled = !idleRange;
  }
}

measureIdle.addEventListener("click", () => run(() => measureKey(false)));
measurePressed.addEventListener("click", () => run(() => measureKey(true)));
applyRecommended.addEventListener("click", () => run(async () => {
  if (!idleRange || !pressedRange) return;
  const data = await request(commands.read, selected);
  // The idle baseline can move after measuring. Keep the measured boundaries.
  const { down, up } = recommendedOffsets(data[1]);
  await applyOffsets(down, up);
  measurementStatus.textContent = `押下 ${down}、解放 ${up} を適用しました。打鍵を試してから保存してください。`;
}));

element("offsets").addEventListener("submit", (event) => {
  event.preventDefault();
  const down = press.valueAsNumber;
  const up = release.valueAsNumber;
  const allKeys = offsetScope.value === "all";
  return run(() => applyOffsets(down, up, allKeys));
});
element("defaults").addEventListener("click", () => run(() => applyOffsets(info[3], info[4])));
element("save").addEventListener("click", () => run(async () => {
  if (!unsaved) {
    status.textContent = "新しく適用した変更はありません。";
    return;
  }
  await request(commands.save);
  unsaved = false;
  status.textContent = "この側の設定を保存しました。";
}));
function connectedSide() {
  return info[0] === 0 ? "left" : "right";
}

function supportsSavedOffsets() {
  return info[5] > 0 && info[5] * 2 === info[1] * info[2];
}

async function readCurrentOffsets() {
  const offsets = [];
  for (let index = 0; index < info[1] * info[2]; index++) {
    const [, , press, release] = await request(commands.read, index);
    if (!validOffsets(press, release)) throw new Error("現在のオフセットを読み取れませんでした。");
    offsets.push({ press, release });
  }
  return offsets;
}

async function readSavedOffsets() {
  const pageKeys = info[5];
  const offsets = [];
  for (const first of [0, pageKeys]) {
    const data = await request(commands.readSaved, first);
    for (let i = 0; i < pageKeys * 2; i += 2) {
      const [down, up] = [data[i], data[i + 1]];
      if (!validOffsets(down, up)) throw new Error("保存済みのオフセットを読み取れませんでした。");
      offsets.push({ press: down, release: up });
    }
  }
  return offsets;
}

function validateBackup(backup) {
  if (
    backup?.format !== "miniec-thresholds" || backup.version !== 1
    || backup.rows !== info[1] || backup.columns !== info[2]
    || !Array.isArray(backup.offsets) || backup.offsets.length !== info[1] * info[2]
    || !backup.offsets.every((pair) => validOffsets(pair?.press, pair?.release))
  ) {
    throw new Error("miniEC の有効なバックアップファイルを選んでください。");
  }
  if (backup.side !== connectedSide()) {
    throw new Error("ファイルと接続中の左右が一致しません。同じ側のバックアップを選んでください。");
  }
}

function runBackup(action) {
  return run(async () => {
    try {
      await action();
    } catch (error) {
      if (device) backupStatus.textContent = error.message;
    }
  });
}

backupButton.addEventListener("click", () => runBackup(async () => {
  const saved = supportsSavedOffsets();
  const source = saved ? "保存済み" : "現在";
  backupStatus.textContent = `${source}の設定を読み込んでいます。`;
  const backup = {
    format: "miniec-thresholds",
    version: 1,
    side: connectedSide(),
    rows: info[1],
    columns: info[2],
    offsets: saved ? await readSavedOffsets() : await readCurrentOffsets(),
  };
  const link = document.createElement("a");
  link.href = URL.createObjectURL(new Blob([JSON.stringify(backup, null, 2) + "\n"], { type: "application/json" }));
  link.download = `miniec-thresholds-${backup.side}.json`;
  link.click();
  setTimeout(() => URL.revokeObjectURL(link.href), 1000);
  backupStatus.textContent = `${source}の設定をダウンロードしました。`;
}));

backupFile.addEventListener("change", () => {
  restoreButton.disabled = !backupFile.files.length;
  backupStatus.textContent = backupFile.files.length ? "ファイルを選びました。「この側に復元」で書き込みます。" : "";
});

restoreButton.addEventListener("click", () => runBackup(async () => {
  const [file] = backupFile.files;
  if (!file) return;
  let backup;
  try {
    if (file.size > 16384) throw new Error();
    backup = JSON.parse(await file.text());
  } catch {
    throw new Error("バックアップの JSON を読み取れませんでした。");
  }
  validateBackup(backup);
  backupStatus.textContent = "設定を復元しています。";
  const pageKeys = info[5];
  await request(
    commands.restore,
    0,
    ...backup.offsets.slice(0, pageKeys).flatMap((pair) => [pair.press, pair.release]),
  );
  try {
    await request(
      commands.restore,
      pageKeys,
      ...backup.offsets.slice(pageKeys).flatMap((pair) => [pair.press, pair.release]),
    );
    unsaved = false;
    const saved = await readSavedOffsets();
    if (
      saved.some((pair, index) =>
        pair.press !== backup.offsets[index].press
        || pair.release !== backup.offsets[index].release
      )
    ) {
      throw new Error();
    }
  } catch {
    throw new Error("復元結果を確認できませんでした。接続を確認し、保存済みの値をバックアップして確認してください。");
  }
  render(selected, await request(commands.read, selected), true);
  backupFile.value = "";
  restoreButton.disabled = true;
  backupStatus.textContent = "全キーの設定を復元・保存しました。";
}));

if (hid) {
  hid.addEventListener("disconnect", (event) => {
    if (event.device !== device) return;
    forgetDevice();
    status.textContent = "USB 接続が切れました。再接続してください。";
  });
} else {
  element("connect").disabled = true;
  status.textContent =
    "デスクトップ版の Chrome または Edge で、このページを HTTPS または localhost から開いてください。";
}
