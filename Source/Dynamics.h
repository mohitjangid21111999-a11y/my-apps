#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace mo {
inline float db(float x) { return 20.f * std::log10(std::max(x, 1.e-9f)); }
inline float amp(float x) { return std::pow(10.f, x / 20.f); }
struct Parameters {
  float gate = -90, comp = 0, gain = -1;
  bool bypass = false;
};
struct Meters {
  float input = 0, reduction = 0, left = 0, right = 0;
};
class Dynamics {
  double rate = 48000;
  float envelope = 0, gateGain = 0, compGain = 0, limiter = 1, wet = 1;
  int hold = 0, pos = 0, delay = 240;
  Parameters smooth;
  std::array<std::array<float, 2048>, 2> ring{}, wetRing{};
  std::array<float, 4096> peaks{};
  std::array<unsigned long long, 4096> ages{};
  unsigned long long clock = 0;
  int head = 0, tail = 0;
  float coefficient(float ms) const {
    return std::exp(-1.f / (float(rate) * ms * .001f));
  }
  float follow(float old, float target, float ms) const {
    return target + (old - target) * coefficient(ms);
  }

public:
  void prepare(double sr) {
    rate = std::clamp(sr, 8000., 384000.);
    delay = std::min(2047, int(rate * .005));
    reset();
  }
  int latency() const { return delay; }
  void reset() {
    envelope = 0;
    gateGain = compGain = 0;
    limiter = wet = 1;
    hold = pos = 0;
    ring = {};
    wetRing = {};
    head = tail = 0;
    clock = 0;
    smooth = {};
  }
  Meters process(float *const *data, int channels, int count, Parameters p) {
    Meters m;
    p.gate = std::clamp(p.gate, -90.f, 0.f);
    p.comp = std::clamp(p.comp, -36.f, 0.f);
    p.gain = std::clamp(p.gain, -36.f, 0.f);
    for (int i = 0; i < count; ++i) {
      smooth.gate = follow(smooth.gate, p.gate, 15);
      smooth.comp = follow(smooth.comp, p.comp, 15);
      smooth.gain = follow(smooth.gain, p.gain, 15);
      std::array<float, 2> x{};
      float peak = 0;
      for (int c = 0; c < channels; ++c) {
        float v = data[c][i];
        x[c] = std::isfinite(v) ? std::clamp(v, -32.f, 32.f) : 0;
        peak = std::max(peak, std::abs(x[c]));
      }
      m.input = std::max(m.input, peak);
      envelope = follow(envelope, peak, peak > envelope ? 0.5f : 55.f);
      float level = db(envelope);
      float gateTarget = 0;
      if (smooth.gate > -89.f) {
        float below = smooth.gate - level;
        float knee = 6;
        if (below > -knee / 2)
          gateTarget = -std::min(80.f, below < knee / 2
                                           ? (below + knee / 2) *
                                                 (below + knee / 2) / (2 * knee)
                                           : below);
      }
      if (gateTarget > gateGain)
        hold = int(rate * .04);
      else if (hold > 0) {
        --hold;
        gateTarget = gateGain;
      }
      gateGain =
          follow(gateGain, gateTarget, gateTarget > gateGain ? 2.f : 120.f);
      float amount = -smooth.comp, ratio = 1.f + amount / 7.f,
            over = level - smooth.comp, knee = 8.f;
      float reduction = over <= -knee / 2 ? 0
                        : over < knee / 2
                            ? (1 - 1 / ratio) * (over + knee / 2) *
                                  (over + knee / 2) / (2 * knee)
                            : (1 - 1 / ratio) * over;
      compGain = follow(
          compGain, -reduction,
          -reduction < compGain ? 2.f : 100.f + std::min(200.f, reduction * 8));
      float makeup = amount * .48f;
      float g = amp(gateGain + compGain + makeup + smooth.gain);
      wet = follow(wet, p.bypass ? 0.f : 1.f, 5);
      float ceiling = amp(smooth.gain);
      float processedPeak = peak * g;
      while (head != tail && ages[head] + (unsigned)delay < clock)
        head = (head + 1) % 4096;
      while (head != tail && peaks[(tail + 4095) % 4096] <= processedPeak)
        tail = (tail + 4095) % 4096;
      peaks[tail] = processedPeak;
      ages[tail] = clock++;
      tail = (tail + 1) % 4096;
      float aheadPeak = peaks[head];
      float target = aheadPeak > ceiling ? ceiling / aheadPeak : 1.f;
      limiter = target < limiter ? target : follow(limiter, target, 90);
      m.reduction = std::max(m.reduction, -gateGain - compGain);
      for (int c = 0; c < channels; ++c) {
        float dry = ring[c][pos];
        ring[c][pos] = x[c];
        // A bounded monotonic queue holds every peak in the lookahead window.
        float delayedWet = wetRing[c][pos];
        wetRing[c][pos] = x[c] * g;
        float effected = std::clamp(delayedWet * limiter, -ceiling, ceiling);
        float out = dry + (effected - dry) * wet;
        data[c][i] = std::abs(out) < 1.e-20f ? 0 : out;
        if (c == 0)
          m.left = std::max(m.left, std::abs(out));
        else
          m.right = std::max(m.right, std::abs(out));
      }
      if (++pos >= delay)
        pos = 0;
    }
    if (channels == 1)
      m.right = m.left;
    return m;
  }
};
} // namespace mo
