#include <iostream>
extern "C" int moVoxProcessorChecks();
extern "C" int moVoxRender(const char *);
extern "C" int moVoxHostChecks(const char *);
int main(int argc, char **argv) {
  if (argc > 2)
    return moVoxRender(argv[2]);
  if (argc > 1) {
    int r = moVoxHostChecks(argv[1]);
    std::cout << "VST3 host load/process/state check: " << r << "\n";
    return r;
  }
  int result = moVoxProcessorChecks();
  std::cout << (result ? "FAIL" : "PASS")
            << ": native processor, preset parameters, session recall, theme "
               "recall, audio-fed meter atomics ("
            << result << ")\n";
  return result;
}
