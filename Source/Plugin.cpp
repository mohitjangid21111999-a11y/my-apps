#include "Presets.h"
#include <juce_audio_utils/juce_audio_utils.h>
using namespace juce;
class Vox : public AudioProcessor {
public:
  UndoManager undo;
  AudioProcessorValueTreeState state;
  mo::Dynamics engine;
  std::atomic<float> input{0}, reduction{0}, left{0}, right{0};
  std::atomic<int> theme{1};
  int current = 0;
  static AudioProcessorValueTreeState::ParameterLayout layout() {
    AudioProcessorValueTreeState::ParameterLayout l;
    l.add(std::make_unique<AudioParameterFloat>(
        ParameterID{"gate", 1}, "Gate", NormalisableRange<float>(-90, 0, .1f),
        -90));
    l.add(std::make_unique<AudioParameterFloat>(
        ParameterID{"comp", 1}, "Compression",
        NormalisableRange<float>(-36, 0, .1f), 0));
    l.add(std::make_unique<AudioParameterFloat>(
        ParameterID{"gain", 1}, "Gain / Ceiling",
        NormalisableRange<float>(-36, 0, .1f), -1));
    l.add(std::make_unique<AudioParameterBool>(ParameterID{"bypass", 1},
                                               "Bypass", false));
    return l;
  }
  Vox()
      : AudioProcessor(
            BusesProperties()
                .withInput("Input", AudioChannelSet::stereo(), true)
                .withOutput("Output", AudioChannelSet::stereo(), true)),
        state(*this, &undo, "MOVOX", layout()) {}
  const String getName() const override { return "MO VOX"; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override { return 0; }
  void prepareToPlay(double sr, int) override {
    engine.prepare(sr);
    setLatencySamples(engine.latency());
  }
  void releaseResources() override {}
  bool isBusesLayoutSupported(const BusesLayout &l) const override {
    return (l.getMainInputChannelSet() == AudioChannelSet::mono() ||
            l.getMainInputChannelSet() == AudioChannelSet::stereo()) &&
           l.getMainInputChannelSet() == l.getMainOutputChannelSet();
  }
  AudioProcessorParameter *getBypassParameter() const override {
    return state.getParameter("bypass");
  }
  void processBlock(AudioBuffer<float> &b, MidiBuffer &) override {
    ScopedNoDenormals guard;
    for (int c = getTotalNumInputChannels(); c < b.getNumChannels(); ++c)
      b.clear(c, 0, b.getNumSamples());
    mo::Parameters p{*state.getRawParameterValue("gate"),
                     *state.getRawParameterValue("comp"),
                     *state.getRawParameterValue("gain"),
                     *state.getRawParameterValue("bypass") > .5f};
    auto m = engine.process(b.getArrayOfWritePointers(),
                            std::min(2, getTotalNumInputChannels()),
                            b.getNumSamples(), p);
    auto publish = [](std::atomic<float> &dst, float v) {
      float old = dst.load();
      while (old < v && !dst.compare_exchange_weak(old, v)) {
      };
    };
    publish(input, m.input);
    publish(reduction, m.reduction);
    publish(left, m.left);
    publish(right, m.right);
  }
  bool hasEditor() const override { return true; }
  AudioProcessorEditor *createEditor() override;
  int getNumPrograms() override { return 38; }
  int getCurrentProgram() override { return current; }
  void setCurrentProgram(int i) override {
    current = std::clamp(i, 0, 37);
    auto p = mo::presets[current].p;
    undo.beginNewTransaction("Load preset");
    set("gate", p.gate);
    set("comp", p.comp);
    set("gain", p.gain);
  }
  void set(const char *id, float value) {
    auto *p = state.getParameter(id);
    p->beginChangeGesture();
    p->setValueNotifyingHost(p->convertTo0to1(value));
    p->endChangeGesture();
  }
  const String getProgramName(int i) override {
    return mo::presets[std::clamp(i, 0, 37)].name;
  }
  void changeProgramName(int, const String &) override {}
  void getStateInformation(MemoryBlock &dest) override {
    auto s = state.copyState();
    s.setProperty("theme", theme.load(), nullptr);
    s.setProperty("program", current, nullptr);
    if (auto xml = s.createXml())
      copyXmlToBinary(*xml, dest);
  }
  void setStateInformation(const void *data, int size) override {
    if (auto x = getXmlFromBinary(data, size))
      if (x->hasTagName("MOVOX")) {
        auto s = ValueTree::fromXml(*x);
        state.replaceState(s);
        theme = std::clamp(int(s.getProperty("theme", 1)), 0, 2);
        current = std::clamp(int(s.getProperty("program", 0)), 0, 37);
      }
  }
};
class ConsoleLook : public LookAndFeel_V4 {
public:
  void drawLinearSlider(Graphics &g, int x, int y, int width, int height,
                        float position, float, float, Slider::SliderStyle,
                        Slider &slider) override {
    float centre = x + width * .5f;
    g.setColour(Colour(0xff070e16));
    g.fillRoundedRectangle(centre - 4, float(y), 8, float(height), 3);
    g.setFont(FontOptions(9.f));
    for (int i = 0; i <= 6; ++i) {
      float yy = y + height * i / 6.f;
      g.setColour(Colour(0xff7c8c99));
      g.drawLine(centre + 7, yy, centre + 12, yy);
      float value = float(slider.getMaximum() +
                          (slider.getMinimum() - slider.getMaximum()) * i / 6.);
      String text = slider.getName() == "GATE" && i == 6 ? String("Off")
                                                         : String(value, 0);
      g.drawText(text, int(centre + 14), int(yy - 6), 26, 12,
                 Justification::left);
    }
    ColourGradient metal(Colour(0xfff1e7cb), centre - 16, position - 9,
                         Colour(0xff887858), centre + 16, position + 9, false);
    metal.addColour(.45, Colour(0xffb8ab8b));
    g.setGradientFill(metal);
    g.fillRoundedRectangle(centre - 16, position - 9, 32, 18, 3);
    g.setColour(Colour(0xff252a30));
    g.drawLine(centre - 12, position, centre + 12, position, 2);
  }
};
class VoxEditor : public AudioProcessorEditor, private Timer {
  Vox &v;
  ConsoleLook look;
  Slider gate, comp, gain;
  std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> ga, co, gn;
  TextButton bypass{"BYPASS"}, prev{"<"}, next{">"}, ab{"A/B"}, undo{"Undo"},
      redo{"Redo"}, settings{"Settings"};
  ComboBox presets;
  std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> by;
  ValueTree alternate;
  float in = -90, gr = 0, l = -90, r = -90, holdL = -90, holdR = -90;
  int ticks = 0;
  std::unique_ptr<FileChooser> chooser;
  File userDir() const {
    return File::getSpecialLocation(File::userApplicationDataDirectory)
        .getChildFile("MO AUDIO/MO VOX/Presets");
  }
  void refresh() {
    presets.clear();
    String cat;
    for (int i = 0; i < 38; ++i) {
      auto &p = mo::presets[i];
      if (cat != p.category) {
        cat = p.category;
        presets.addSectionHeading(cat);
      }
      presets.addItem(p.name, i + 1);
    }
    presets.setSelectedId(v.current + 1, dontSendNotification);
  }
  void configure(Slider &s, const String &name) {
    addAndMakeVisible(s);
    s.setName(name);
    s.setSliderStyle(Slider::LinearVertical);
    s.setTextBoxStyle(Slider::TextBoxBelow, false, 90, 26);
    s.setDoubleClickReturnValue(true, name == "GATE"   ? -90
                                      : name == "COMP" ? 0
                                                       : -1);
    s.setTextValueSuffix(" dB");
    s.setVelocityBasedMode(false);
    s.setColour(Slider::thumbColourId, Colour(0xffd8bd85));
    s.setColour(Slider::trackColourId, Colour(0xff314959));
  }
  void saveUser() {
    chooser = std::make_unique<FileChooser>(
        "Save or replace user preset", userDir().getChildFile("My Vocal.xml"),
        "*.xml");
    chooser->launchAsync(FileBrowserComponent::saveMode |
                             FileBrowserComponent::canSelectFiles |
                             FileBrowserComponent::warnAboutOverwriting,
                         [this](const FileChooser &f) {
                           auto file = f.getResult();
                           if (file != File{}) {
                             file.getParentDirectory().createDirectory();
                             if (auto xml = v.state.copyState().createXml())
                               xml->writeTo(file);
                           }
                         });
  }
  void loadUser() {
    chooser = std::make_unique<FileChooser>("Import / load user preset",
                                            userDir(), "*.xml");
    chooser->launchAsync(FileBrowserComponent::openMode |
                             FileBrowserComponent::canSelectFiles,
                         [this](const FileChooser &f) {
                           if (auto xml = XmlDocument::parse(f.getResult())) {
                             if (xml->hasTagName("MOVOX")) {
                               auto tree = ValueTree::fromXml(*xml);
                               for (auto id : {"gate", "comp", "gain"}) {
                                 for (auto child : tree)
                                   if (child["id"].toString() == id) {
                                     float value = child["value"];
                                     if (std::isfinite(value))
                                       v.set(id, value);
                                   }
                               }
                             }
                           }
                         });
  }
  void menu() {
    PopupMenu m;
    m.addItem(1, "MO Classic");
    m.addItem(2, "MO Dark");
    m.addItem(3, "MO Light");
    m.addSeparator();
    m.addItem(4, "Save / export user preset...");
    m.addItem(5, "Load / import user preset...");
    m.addItem(6, "Restore factory default");
    m.addItem(7, "Delete user preset...");
    m.addItem(8, "Rename user preset...");
    m.showMenuAsync(
        PopupMenu::Options().withTargetComponent(&settings),
        [safe = Component::SafePointer<VoxEditor>(this)](int id) {
          if (!safe)
            return;
          auto &e = *safe;
          if (id >= 1 && id <= 3)
            e.v.theme = id - 1;
          if (id == 4)
            e.saveUser();
          if (id == 5)
            e.loadUser();
          if (id == 6)
            e.v.setCurrentProgram(0);
          if (id == 7) {
            e.chooser = std::make_unique<FileChooser>("Delete user preset",
                                                      e.userDir(), "*.xml");
            e.chooser->launchAsync(FileBrowserComponent::openMode |
                                       FileBrowserComponent::canSelectFiles,
                                   [safe](const FileChooser &f) {
                                     if (safe &&
                                         f.getResult().getParentDirectory() ==
                                             safe->userDir())
                                       f.getResult().deleteFile();
                                   });
          }
          if (id == 8) {
            e.chooser = std::make_unique<FileChooser>(
                "Choose user preset to rename", e.userDir(), "*.xml");
            e.chooser->launchAsync(
                FileBrowserComponent::openMode |
                    FileBrowserComponent::canSelectFiles,
                [safe](const FileChooser &f) {
                  if (!safe)
                    return;
                  auto original = f.getResult();
                  if (original.getParentDirectory() != safe->userDir())
                    return;
                  safe->chooser = std::make_unique<FileChooser>(
                      "New user preset name", original, "*.xml");
                  safe->chooser->launchAsync(
                      FileBrowserComponent::saveMode |
                          FileBrowserComponent::canSelectFiles,
                      [safe, original](const FileChooser &dest) {
                        if (safe) {
                          auto target = dest.getResult();
                          if (target.getParentDirectory() == safe->userDir() &&
                              !target.exists())
                            original.moveFileTo(target);
                        }
                      });
                });
          }
          e.repaint();
        });
  }

public:
  VoxEditor(Vox &p) : AudioProcessorEditor(p), v(p) {
    setLookAndFeel(&look);
    setResizable(true, true);
    setResizeLimits(620, 470, 1000, 760);
    setSize(680, 520);
    configure(gate, "GATE");
    configure(comp, "COMP");
    configure(gain, "GAIN");
    ga = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
        v.state, "gate", gate);
    co = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
        v.state, "comp", comp);
    gn = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
        v.state, "gain", gain);
    gate.textFromValueFunction = [](double x) {
      return x <= -89.95 ? String("Off") : String(x, 1) + " dB";
    };
    gate.valueFromTextFunction = [](const String& text) { auto t=text.trim(); return t.equalsIgnoreCase("Off") || t.containsIgnoreCase("inf") ? -90.0 : t.getDoubleValue(); };
    gate.setTextValueSuffix("");
    gate.updateText();
    for (auto *b : {&bypass, &prev, &next, &ab, &undo, &redo, &settings})
      addAndMakeVisible(b);
    addAndMakeVisible(presets);
    refresh();
    bypass.setClickingTogglesState(true);
    by = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(
        v.state, "bypass", bypass);
    presets.onChange = [this] {
      v.setCurrentProgram(presets.getSelectedId() - 1);
    };
    prev.onClick = [this] {
      v.setCurrentProgram((v.current + 37) % 38);
      refresh();
    };
    next.onClick = [this] {
      v.setCurrentProgram((v.current + 1) % 38);
      refresh();
    };
    alternate = v.state.copyState();
    ab.onClick = [this] {
      auto old = v.state.copyState();
      v.state.replaceState(alternate);
      alternate = old;
    };
    undo.onClick = [this] { v.undo.undo(); };
    redo.onClick = [this] { v.undo.redo(); };
    settings.onClick = [this] { menu(); };
    startTimerHz(30);
  }
  ~VoxEditor() override {
    stopTimer();
    setLookAndFeel(nullptr);
  }
  void resized() override {
    float sx = getWidth() / 680.f, sy = getHeight() / 520.f;
    auto place = [&](Component &c, int x, int y, int w, int h) {
      c.setBounds(int(x * sx), int(y * sy), int(w * sx), int(h * sy));
    };
    place(presets, 120, 85, 330, 28);
    place(prev, 82, 85, 30, 28);
    place(next, 458, 85, 30, 28);
    place(bypass, 515, 85, 86, 28);
    place(gate, 112, 158, 92, 292);
    place(comp, 294, 158, 92, 292);
    place(gain, 476, 158, 92, 292);
    place(ab, 80, 475, 62, 25);
    place(undo, 150, 475, 62, 25);
    place(redo, 220, 475, 62, 25);
    place(settings, 508, 475, 92, 25);
  }
  void mouseDown(const MouseEvent &) override {
    holdL = holdR = -90;
    ticks = 0;
  }
  void timerCallback() override {
    auto decay = [](float old, float next) {
      return std::max(next, old - 1.6f);
    };
    in = decay(in, mo::db(v.input.exchange(0)));
    l = decay(l, mo::db(v.left.exchange(0)));
    r = decay(r, mo::db(v.right.exchange(0)));
    gr = std::max(v.reduction.exchange(0), gr - .6f);
    holdL = std::max(holdL, l);
    holdR = std::max(holdR, r);
    ++ticks;
    auto p = mo::presets[v.current].p;
    bool dirty = std::abs(gate.getValue() - p.gate) > .15 ||
                 std::abs(comp.getValue() - p.comp) > .15 ||
                 std::abs(gain.getValue() - p.gain) > .15;
    presets.setText(String(mo::presets[v.current].name) + (dirty ? " *" : ""),
                    dontSendNotification);
    repaint();
  }
  void paint(Graphics &g) override {
    g.addTransform(
        AffineTransform::scale(getWidth() / 680.f, getHeight() / 520.f));
    int t = v.theme;
    g.fillAll(Colour(t == 2 ? 0xffd1d6da : t == 0 ? 0xff424e5a : 0xff1b1e23));
    auto ink = Colour(t == 2 ? 0xff263442 : 0xffe9e2d4);
    g.setColour(ink);
    g.setFont(FontOptions(39.f, Font::bold));
    g.drawText("MO VOX", 32, 18, 300, 48, Justification::left);
    g.setFont(FontOptions(13.f));
    g.drawText("MO AUDIO  /  VOCAL DYNAMICS", 360, 32, 285, 24,
               Justification::right);
    for (int x : {80, 262, 444}) {
      g.setColour(Colour(0xff101e2c));
      g.fillRoundedRectangle(float(x), 130, 158, 330, 7);
    }
    g.setColour(Colour(0xffd8bd85));
    g.setFont(FontOptions(17.f, Font::bold));
    g.drawText("GATE", 110, 132, 96, 24, Justification::centred);
    g.drawText("COMP", 292, 132, 96, 24, Justification::centred);
    g.drawText("GAIN", 474, 132, 96, 24, Justification::centred);
    auto meter = [&](int x, float value, Colour colour, bool reduction) {
      g.setColour(Colour(0xff07111c));
      g.fillRect(x, 170, 15, 240);
      float h = std::clamp(reduction ? value / 60.f : (value + 60.f) / 60.f,
                           0.f, 1.f) *
                240;
      g.setColour(colour);
      g.fillRect(float(x), reduction ? 170.f : 410 - h, 15.f, h);
    };
    meter(88, in, Colour(0xff41a7e0), false);
    meter(270, gr, Colour(0xffed963e), true);
    meter(580, l,
          Colour(l > -1   ? 0xffef6357
                 : l > -6 ? 0xffe7bd5c
                          : 0xff56cba0),
          false);
    meter(600, r,
          Colour(r > -1   ? 0xffef6357
                 : r > -6 ? 0xffe7bd5c
                          : 0xff56cba0),
          false);
    g.setColour(ink);
    g.setFont(FontOptions(10.f));
    for (int db : {0, -12, -24, -36, -48, -60}) {
      int y = 170 - db * 4;
      g.drawText(String(db), 42, y - 6, 40, 12, Justification::right);
      g.drawText(String(-db), 239, y - 6, 28, 12, Justification::right);
      g.drawText(String(db), 618, y - 6, 35, 12, Justification::left);
    }
    g.setColour(Colour(0xffed963e));
    g.setFont(FontOptions(13.f));
    g.drawText(String(gr, 1) + " dB GR", 268, 452, 146, 17,
               Justification::centred);
    g.setColour(ink);
    g.setFont(FontOptions(10.f));
    g.drawText("IN", 82, 415, 28, 16, Justification::centred);
    g.drawText("L  R", 580, 415, 36, 16, Justification::centred);
    for (auto pair : {std::pair<int, float>{580, holdL}, {600, holdR}}) {
      g.setColour(Colour(0xfff5cf75));
      g.fillRect(pair.first, 170 + int(std::clamp(-pair.second, 0.f, 60.f) * 4),
                 15, 2);
    }
    g.setColour(ink);
    g.drawText("5 ms lookahead | stereo linked", 300, 479, 200, 20,
               Justification::centred);
  }
};
AudioProcessorEditor *Vox::createEditor() { return new VoxEditor(*this); }
AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new Vox; }
// Offline processor integration checks; no audio device or display is needed.
extern "C" int moVoxProcessorChecks() {
  Vox v;
  v.prepareToPlay(48000, 256);
  if (v.getLatencySamples() != 240)
    return 1;
  MidiBuffer midi;
  AudioBuffer<float> audio(2, 256);
  for (int i = 0; i < 38; ++i) {
    v.setCurrentProgram(i);
    auto p = mo::presets[i].p;
    if (std::abs(*v.state.getRawParameterValue("comp") - p.comp) > .01f)
      return 2;
  }
  v.setCurrentProgram(22);
  v.theme = 2;
  MemoryBlock saved;
  v.getStateInformation(saved);
  Vox restored;
  restored.setStateInformation(saved.getData(), int(saved.getSize()));
  if (restored.theme != 2 ||
      std::abs(*restored.state.getRawParameterValue("comp") + 13) > .01f)
    return 3;
  for (int b = 0; b < 100; ++b) {
    for (int c = 0; c < 2; ++c)
      for (int i = 0; i < 256; ++i)
        audio.setSample(c, i, .4f * std::sin(float((b * 256 + i) * .04)));
    v.processBlock(audio, midi);
  }
  if (v.input <= 0 || v.left <= 0 || v.reduction <= 0)
    return 4;
  return 0;
}

extern "C" int moVoxHostChecks(const char *path) {
  ScopedJuceInitialiser_GUI init;
  AudioPluginFormatManager formats;
  formats.addDefaultFormats();
  OwnedArray<PluginDescription> types;
  for (int i = 0; i < formats.getNumFormats(); ++i)
    formats.getFormat(i)->findAllTypesForFile(types, path);
  if (types.isEmpty())
    return 10;
  String error;
  auto plugin = formats.createPluginInstance(*types[0], 48000, 256, error);
  if (!plugin)
    return 11;
  plugin->prepareToPlay(48000, 256);
  AudioBuffer<float> b(2, 256);
  MidiBuffer midi;
  float max = 0;
  for (int block = 0; block < 100; ++block) {
    for (int c = 0; c < 2; ++c)
      for (int i = 0; i < 256; ++i)
        b.setSample(c, i, .25f * std::sin(float((block * 256 + i) * .05)));
    plugin->processBlock(b, midi);
    max = std::max(max, b.getMagnitude(0, 256));
  }
  MemoryBlock saved;
  plugin->getStateInformation(saved);
  plugin->setStateInformation(saved.getData(), int(saved.getSize()));
  plugin->releaseResources();
  return max > 0 && std::isfinite(max) ? 0 : 12;
}
extern "C" int moVoxRender(const char *path) {
  ScopedJuceInitialiser_GUI init;
  Vox processor;
  std::unique_ptr<AudioProcessorEditor> editor(processor.createEditor());
  auto image = editor->createComponentSnapshot(editor->getLocalBounds());
  File file(path);
  file.deleteFile();
  FileOutputStream output(file);
  PNGImageFormat png;
  return png.writeImageToStream(image, output) ? 0 : 1;
}
