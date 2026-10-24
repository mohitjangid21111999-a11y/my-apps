#include "../Source/Presets.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
void check(bool ok, const char *msg) {
  if (!ok)
    throw std::runtime_error(msg);
}
struct Result {
  float rms, peak, gr;
};
Result run(mo::Parameters p, float amplitude, double sr = 48000,
           int channels = 2) {
  mo::Dynamics d;
  d.prepare(sr);
  float peak = 0, gr = 0;
  double sum = 0;
  int n = 0;
  for (int b = 0; b < 400; ++b) {
    float a[256], c[256];
    for (int i = 0; i < 256; ++i)
      a[i] = c[i] =
          amplitude *
          std::sin(float((b * 256 + i) * 2 * 3.141592653589793 * 440 / sr));
    float *ptr[] = {a, c};
    auto m = d.process(ptr, channels, 256, p);
    if (b > 200) {
      for (int i = 0; i < 256; ++i) {
        check(std::isfinite(a[i]), "nonfinite output");
        peak = std::max(peak, std::abs(a[i]));
        sum += a[i] * a[i];
        ++n;
        if (channels == 2)
          check(a[i] == c[i], "stereo link mismatch");
      }
      gr = std::max(gr, m.reduction);
    }
  }
  return {float(std::sqrt(sum / n)), peak, gr};
}
int main() {
  try {
    check(std::size(mo::presets) == 38, "preset count");
    auto dry = run({-90, 0, 0}, .1f);
    check(std::abs(dry.rms - .07071) < .001, "input does not reach output");
    auto noisy = run({-90, 0, -1}, .001f);
    auto gated = run({-40, 0, -1}, .001f);
    check(gated.rms < noisy.rms * .15f, "gate attenuation");
    auto compressed = run({-90, -24, -1}, .5f);
    check(compressed.gr > 10, "compressor gain reduction");
    auto ceiling = run({-90, 0, -12}, 4);
    check(ceiling.peak <= mo::amp(-12) + 1.e-5, "limiter ceiling");
    check(ceiling.peak > mo::amp(-12) * .98f,
          "limiter test never reached ceiling");
    check(run({-90, 0, -18}, .1f).rms < dry.rms * .14, "output gain");
    for (auto &p : mo::presets) {
      check(p.name[0] != 0, "empty preset");
      auto r = run(p.p, .2f);
      check(r.peak <= mo::amp(p.p.gain) + 1.e-5, "preset ceiling");
    }
    for (double sr : {22050., 44100., 48000., 88200., 96000., 192000., 384000.})
      for (int c : {1, 2})
        run({-45, -18, -2}, .5, sr, c);
    mo::Dynamics d;
    d.prepare(48000);
    float bad[] = {std::numeric_limits<float>::quiet_NaN(),
                   std::numeric_limits<float>::infinity(), 1.e30f};
    float *ptr[] = {bad};
    d.process(ptr, 1, 3, {});
    for (auto f : bad)
      check(std::isfinite(f), "invalid input protection");
    std::cout
        << "PASS: signal routing, gate, compression, gain, limiter, 38 "
           "presets, mono/stereo, 7 sample rates, invalid input protection\n";
    std::cout << "Gate attenuation: " << mo::db(gated.rms / noisy.rms)
              << " dB; compression GR: " << compressed.gr
              << " dB; ceiling peak: " << mo::db(ceiling.peak) << " dBFS\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
