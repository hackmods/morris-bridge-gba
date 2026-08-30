export class AudioDirector {
  private ctx: AudioContext | null = null;

  private ensure(): AudioContext | null {
    if (typeof AudioContext === 'undefined') {
      return null;
    }
    if (!this.ctx) {
      this.ctx = new AudioContext();
    }
    return this.ctx;
  }

  resume(): void {
    void this.ensure()?.resume();
  }

  place(): void {
    this.tone(420, 0.07, 'triangle', 0.05);
  }

  snap(): void {
    this.noise(0.18, 0.12);
  }

  success(): void {
    this.tone(523, 0.12, 'sine', 0.06);
    this.tone(784, 0.16, 'sine', 0.05, 0.1);
  }

  fail(): void {
    this.tone(180, 0.22, 'sawtooth', 0.05);
  }

  ui(): void {
    this.tone(640, 0.05, 'sine', 0.03);
  }

  private tone(
    freq: number,
    duration: number,
    type: OscillatorType,
    gain: number,
    delay = 0,
  ): void {
    const ctx = this.ensure();
    if (!ctx) {
      return;
    }
    const osc = ctx.createOscillator();
    const amp = ctx.createGain();
    osc.type = type;
    osc.frequency.value = freq;
    amp.gain.value = 0;
    osc.connect(amp);
    amp.connect(ctx.destination);
    const t = ctx.currentTime + delay;
    amp.gain.linearRampToValueAtTime(gain, t + 0.01);
    amp.gain.exponentialRampToValueAtTime(0.0001, t + duration);
    osc.start(t);
    osc.stop(t + duration + 0.02);
  }

  private noise(duration: number, gain: number): void {
    const ctx = this.ensure();
    if (!ctx) {
      return;
    }
    const buffer = ctx.createBuffer(1, ctx.sampleRate * duration, ctx.sampleRate);
    const data = buffer.getChannelData(0);
    for (let i = 0; i < data.length; i += 1) {
      data[i] = (Math.random() * 2 - 1) * (1 - i / data.length);
    }
    const src = ctx.createBufferSource();
    const amp = ctx.createGain();
    src.buffer = buffer;
    amp.gain.value = gain;
    src.connect(amp);
    amp.connect(ctx.destination);
    src.start();
  }
}
