// AudioWorklet that runs the Teng-40 engine (teng40.wasm) on the audio thread.
// Messages from the page: param / noteOn / noteOff / allOff / fill.
// Messages to the page: ready (preset tables) and status (rhythm position).

class TengProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.x = null;
    this.queue = [];
    this.lastStatus = '';
    this.blocks = 0;
    this.port.onmessage = (e) => (this.x ? this.handle(e.data) : this.queue.push(e.data));
    WebAssembly.instantiate(options.processorOptions.wasm, {}).then(({ instance }) => {
      const x = instance.exports;
      x._initialize();
      x.teng_init(sampleRate);
      this.x = x;
      const str = (p) => {
        const m = new Uint8Array(x.memory.buffer);
        let s = '';
        while (m[p]) s += String.fromCharCode(m[p++]);
        return s;
      };
      const tones = [];
      for (let i = 0; i < x.teng_num_tones(); i++) tones.push(str(x.teng_tone_name(i)));
      const rhythms = [];
      for (let i = 0; i < x.teng_num_rhythms(); i++)
        rhythms.push({ name: str(x.teng_rhythm_name(i)), length: x.teng_rhythm_length(i), stepsPerBeat: x.teng_rhythm_steps_per_beat(i) });
      for (const m of this.queue) this.handle(m);
      this.queue = [];
      this.port.postMessage({ type: 'ready', tones, rhythms });
    });
  }

  handle(m) {
    const x = this.x;
    switch (m.type) {
      case 'param': x.teng_set_param(m.id, m.value); break;
      case 'noteOn': x.teng_note_on(m.note, m.velocity, m.channel || 1); break;
      case 'noteOff': x.teng_note_off(m.note, m.channel || 1); break;
      case 'allOff': x.teng_all_notes_off(); break;
      case 'fill': x.teng_fill(); break;
    }
  }

  process(_inputs, outputs) {
    const out = outputs[0];
    if (!this.x || out.length === 0) return true;
    const n = out[0].length;
    const ptr = this.x.teng_process(n);
    const mono = new Float32Array(this.x.memory.buffer, ptr, n);
    for (const ch of out) ch.set(mono);

    if ((++this.blocks & 3) === 0) {
      const running = this.x.teng_running();
      const step = this.x.teng_step();
      const key = running + ':' + step;
      if (key !== this.lastStatus) {
        this.lastStatus = key;
        this.port.postMessage({ type: 'status', running: !!running, step });
      }
    }
    return true;
  }
}

registerProcessor('teng40', TengProcessor);
