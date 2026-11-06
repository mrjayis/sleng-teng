// Teng-40 browser UI. The engine itself is teng40.wasm, running in worklet.js.
(() => {
  'use strict';

  // Keep in sync with ParamId in wasm_api.cpp.
  const PARAM = {
    tone: 0, vibrato: 1, sustain: 2, velocity: 3, transpose: 4, melodyVol: 5,
    rhythm: 6, rhythmOn: 7, tempo: 8, synchro: 9, autoBass: 10, bassOctave: 11,
    split: 12, bassVol: 13, drumVol: 14, lofi: 15, speaker: 16, masterVol: 17,
  };
  // Same defaults as the plugin.
  const DEFAULTS = {
    tone: 0, vibrato: 0, sustain: 0, velocity: 0, transpose: 0, melodyVol: 0.7,
    rhythm: 0, rhythmOn: 0, tempo: 82, synchro: 1, autoBass: 1, bassOctave: 0,
    split: 48, bassVol: 0.8, drumVol: 0.7, lofi: 0, speaker: 0, masterVol: 0.8,
  };
  const STORE_KEY = 'teng40-settings';

  const state = { ...DEFAULTS };
  try {
    const saved = JSON.parse(localStorage.getItem(STORE_KEY) || '{}');
    for (const k of Object.keys(DEFAULTS)) if (typeof saved[k] === 'number') state[k] = saved[k];
  } catch (_) { /* storage unavailable: use defaults */ }
  state.rhythmOn = 0; // never auto-start

  const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
  // Middle C (MIDI 60) is "C3", matching the plugin's labels.
  const noteName = (n) => NOTE_NAMES[n % 12] + (Math.floor(n / 12) - 2);

  let node = null;
  let ctx = null;
  let rhythms = [];
  const $ = (sel) => document.querySelector(sel);

  function save() {
    try { localStorage.setItem(STORE_KEY, JSON.stringify(state)); } catch (_) { /* ignore */ }
  }
  function send(msg) { if (node) node.port.postMessage(msg); }
  function setParam(name, value) {
    state[name] = value;
    send({ type: 'param', id: PARAM[name], value });
    save();
    refresh();
  }

  // ---------------------------------------------------------------- UI build
  function buildPresets(tones, rhy) {
    rhythms = rhy;
    const tg = $('#toneGrid');
    tones.forEach((name, i) => {
      const b = document.createElement('button');
      b.className = 'tone';
      b.setAttribute('role', 'radio');
      b.innerHTML = `<small>${String(i + 1).padStart(2, '0')}</small><span class="nm">${name}</span>`;
      b.addEventListener('click', () => setParam('tone', i));
      tg.appendChild(b);
    });
    const rg = $('#rhythmGrid');
    rhy.forEach((r, i) => {
      const b = document.createElement('button');
      b.className = 'rhy';
      b.setAttribute('role', 'radio');
      b.textContent = r.name.toUpperCase();
      b.addEventListener('click', () => setParam('rhythm', i));
      rg.appendChild(b);
    });
  }

  // Shrink long single words (Harpsichord, Glockenspiel) instead of breaking them.
  function fitToneNames() {
    document.querySelectorAll('.tone .nm').forEach((nm) => {
      nm.style.fontSize = '';
      let size = parseFloat(getComputedStyle(nm).fontSize);
      const room = nm.parentElement.clientWidth - 4;
      const longest = Math.max(...nm.textContent.split(' ').map((w) => w.length));
      const probe = document.createElement('span');
      probe.style.cssText = 'position:absolute;visibility:hidden;white-space:nowrap;font:inherit';
      probe.textContent = nm.textContent.split(' ').find((w) => w.length === longest);
      nm.appendChild(probe);
      while (probe.offsetWidth > room && size > 7) {
        size -= 0.5;
        nm.style.fontSize = size + 'px';
      }
      probe.remove();
    });
  }
  window.addEventListener('resize', fitToneNames);

  function buildControls() {
    document.querySelectorAll('.sw[data-param]').forEach((b) => {
      b.addEventListener('click', () => setParam(b.dataset.param, state[b.dataset.param] ? 0 : 1));
    });
    $('#fill').addEventListener('click', () => send({ type: 'fill' }));

    document.querySelectorAll('.stepper').forEach((el) => {
      const p = el.dataset.param;
      const min = +el.dataset.min, max = +el.dataset.max;
      el.innerHTML = '<output></output><button aria-label="down">&minus;</button><button aria-label="up">+</button>';
      const [down, up] = el.querySelectorAll('button');
      down.addEventListener('click', () => setParam(p, Math.max(min, state[p] - 1)));
      up.addEventListener('click', () => setParam(p, Math.min(max, state[p] + 1)));
    });

    document.querySelectorAll('.fader input').forEach((inp) => {
      inp.addEventListener('input', () => setParam(inp.dataset.param, +inp.value));
    });
  }

  function refresh() {
    document.querySelectorAll('.tone').forEach((b, i) => b.setAttribute('aria-checked', i === state.tone));
    document.querySelectorAll('.rhy').forEach((b, i) => b.setAttribute('aria-checked', i === state.rhythm));
    document.querySelectorAll('.sw[data-param]').forEach((b) => b.setAttribute('aria-pressed', !!state[b.dataset.param]));
    document.querySelectorAll('.stepper').forEach((el) => {
      const v = state[el.dataset.param];
      el.querySelector('output').textContent = el.hasAttribute('data-note') ? noteName(v) : v;
    });
    document.querySelectorAll('.fader input').forEach((inp) => { inp.value = state[inp.dataset.param]; });
    $('#tempoVal').textContent = Math.round(state.tempo);
    buildLeds();
    paintKeyboardZones();
  }

  // ---------------------------------------------------------------- step LEDs
  let ledKey = '';
  let ledEls = [];
  function buildLeds() {
    const r = rhythms[state.rhythm];
    if (!r) return;
    const key = r.length + ':' + r.stepsPerBeat;
    if (key === ledKey) return;
    ledKey = key;
    const box = $('#leds');
    box.innerHTML = '';
    ledEls = [];
    for (let i = 0; i < r.length; i++) {
      const led = document.createElement('i');
      if (i % r.stepsPerBeat === 0) led.className = 'beat';
      box.appendChild(led);
      ledEls.push(led);
    }
  }
  let litLed = null;
  function showStep(running, step) {
    if (litLed) litLed.classList.remove('on');
    litLed = running && ledEls[step] ? ledEls[step] : null;
    if (litLed) litLed.classList.add('on');
  }

  // ---------------------------------------------------------------- keyboard
  const LOW = 24, HIGH = 84; // C0..C5
  const keyEls = new Map();
  const isBlack = (n) => [1, 3, 6, 8, 10].includes(n % 12);

  function buildKeyboard() {
    const kb = $('#keyboard');
    const whites = [];
    for (let n = LOW; n <= HIGH; n++) if (!isBlack(n)) whites.push(n);
    const w = 100 / whites.length;
    let wi = 0;
    for (let n = LOW; n <= HIGH; n++) {
      const el = document.createElement('div');
      el.dataset.note = n;
      if (isBlack(n)) {
        el.className = 'key black';
        el.style.left = `${wi * w - w * 0.3}%`;
        el.style.width = `${w * 0.6}%`;
      } else {
        el.className = 'key white';
        el.style.left = `${wi * w}%`;
        el.style.width = `${w}%`;
        if (n % 12 === 0) el.innerHTML = `<span class="lbl">${noteName(n)}</span>`;
        wi++;
      }
      kb.appendChild(el);
      keyEls.set(n, el);
    }

    // Pointer play with glissando; one note per pointer (multi-touch works).
    const active = new Map();
    const noteAt = (x, y) => {
      const el = document.elementFromPoint(x, y);
      return el && el.dataset && el.dataset.note ? +el.dataset.note : null;
    };
    kb.addEventListener('pointerdown', (e) => {
      const n = noteAt(e.clientX, e.clientY);
      if (n === null) return;
      kb.setPointerCapture(e.pointerId);
      active.set(e.pointerId, n);
      noteOn(n, 0.8, 1);
    });
    kb.addEventListener('pointermove', (e) => {
      if (!active.has(e.pointerId)) return;
      const n = noteAt(e.clientX, e.clientY);
      const old = active.get(e.pointerId);
      if (n === null || n === old) return;
      noteOff(old, 1);
      active.set(e.pointerId, n);
      noteOn(n, 0.8, 1);
    });
    const end = (e) => {
      if (!active.has(e.pointerId)) return;
      noteOff(active.get(e.pointerId), 1);
      active.delete(e.pointerId);
    };
    kb.addEventListener('pointerup', end);
    kb.addEventListener('pointercancel', end);
  }

  function paintKeyboardZones() {
    for (const [n, el] of keyEls) el.classList.toggle('bass', n < state.split);
  }

  const held = new Map(); // note -> count of sources holding it
  function noteOn(note, velocity, channel) {
    held.set(note, (held.get(note) || 0) + 1);
    const el = keyEls.get(note);
    if (el) el.classList.add('down');
    send({ type: 'noteOn', note, velocity, channel });
  }
  function noteOff(note, channel) {
    const c = (held.get(note) || 1) - 1;
    if (c > 0) { held.set(note, c); return; }
    held.delete(note);
    const el = keyEls.get(note);
    if (el) el.classList.remove('down');
    send({ type: 'noteOff', note, channel });
  }

  // Computer keyboard: Z-row = bass section, Q-row = melody (number row = sharps).
  const PC_KEYS = {};
  'zsxdcvgbhnjm'.split('').forEach((k, i) => { PC_KEYS[k] = 36 + i; });
  'q2w3er5t6y7ui'.split('').forEach((k, i) => { PC_KEYS[k] = 60 + i; });
  const pcDown = new Set();
  window.addEventListener('keydown', (e) => {
    if (e.repeat || e.metaKey || e.ctrlKey || e.altKey) return;
    if (e.target instanceof HTMLInputElement) return;
    const n = PC_KEYS[e.key.toLowerCase()];
    if (n === undefined || pcDown.has(e.key.toLowerCase())) return;
    pcDown.add(e.key.toLowerCase());
    noteOn(n, 0.8, 1);
    e.preventDefault();
  });
  window.addEventListener('keyup', (e) => {
    const k = e.key.toLowerCase();
    if (!pcDown.has(k)) return;
    pcDown.delete(k);
    noteOff(PC_KEYS[k], 1);
  });
  window.addEventListener('blur', () => {
    for (const k of pcDown) noteOff(PC_KEYS[k], 1);
    pcDown.clear();
  });

  // Web MIDI (Chrome / Edge / Firefox with permission).
  function setupMidi() {
    if (!navigator.requestMIDIAccess) return;
    navigator.requestMIDIAccess().then((access) => {
      const bind = (input) => {
        input.onmidimessage = (m) => {
          const [st, d1, d2] = m.data;
          const type = st & 0xf0, ch = (st & 0x0f) + 1;
          if (type === 0x90 && d2 > 0) noteOn(d1, d2 / 127, ch);
          else if (type === 0x80 || (type === 0x90 && d2 === 0)) noteOff(d1, ch);
          else if (type === 0xb0 && (d1 === 123 || d1 === 120)) send({ type: 'allOff' });
        };
      };
      access.inputs.forEach(bind);
      access.onstatechange = (e) => { if (e.port.type === 'input' && e.port.state === 'connected') bind(e.port); };
    }).catch(() => { /* no MIDI permission: mouse and keyboard still work */ });
  }

  // ---------------------------------------------------------------- audio
  async function powerOn(wasmBytes) {
    ctx = new AudioContext({ latencyHint: 'interactive' });
    await ctx.audioWorklet.addModule('worklet.js');
    node = new AudioWorkletNode(ctx, 'teng40', {
      numberOfInputs: 0,
      numberOfOutputs: 1,
      outputChannelCount: [2],
      processorOptions: { wasm: wasmBytes },
    });
    node.port.onmessage = (e) => {
      if (e.data.type === 'status') showStep(e.data.running, e.data.step);
    };
    node.connect(ctx.destination);
    for (const k of Object.keys(PARAM)) send({ type: 'param', id: PARAM[k], value: state[k] });
    await ctx.resume();
    $('#power').hidden = true;
    setupMidi();
  }

  async function main() {
    const bytes = await (await fetch('teng40.wasm')).arrayBuffer();
    // Read the preset tables from the engine so the UI never duplicates them.
    const { instance } = await WebAssembly.instantiate(bytes, {});
    const x = instance.exports;
    x._initialize();
    const str = (p) => {
      const m = new Uint8Array(x.memory.buffer);
      let s = '';
      while (m[p]) s += String.fromCharCode(m[p++]);
      return s;
    };
    const tones = [];
    for (let i = 0; i < x.teng_num_tones(); i++) tones.push(str(x.teng_tone_name(i)));
    const rhy = [];
    for (let i = 0; i < x.teng_num_rhythms(); i++)
      rhy.push({ name: str(x.teng_rhythm_name(i)), length: x.teng_rhythm_length(i), stepsPerBeat: x.teng_rhythm_steps_per_beat(i) });

    buildPresets(tones, rhy);
    buildControls();
    buildKeyboard();
    refresh();
    fitToneNames();
    // On narrow screens the keyboard scrolls sideways; start it just below the split.
    const wrap = $('.kb-wrap');
    const startKey = keyEls.get(36);
    if (wrap.scrollWidth > wrap.clientWidth && startKey) wrap.scrollLeft = startKey.offsetLeft;

    $('#powerBtn').addEventListener('click', () => {
      $('#powerBtn').disabled = true;
      powerOn(bytes.slice(0)).catch((err) => {
        $('#power p').textContent = `Couldn't start audio: ${err.message}`;
        $('#powerBtn').disabled = false;
      });
    });
  }

  main().catch((err) => {
    $('#power p').textContent = `Couldn't load the engine: ${err.message}`;
  });
})();
