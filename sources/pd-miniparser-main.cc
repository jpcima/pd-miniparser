/*
    Copyright Jean-Pierre Cimalando 2017

    Distributed under the Boost Software License, Version 1.0.
    http://www.boost.org/LICENSE_1_0.txt
*/

#include "pd-miniparser.h"
#include <getopt.h>
#include <fstream>
#include <iostream>

int main(int argc, char *argv[]) {
  for (int c; (c = getopt(argc, argv, "")) != -1;) {
    switch (c) {
      default:
        return 1;
    }
  }

  for (unsigned ind = optind; ind < (unsigned)argc; ++ind) {
    std::string filename = argv[ind];

    std::ifstream in(filename, std::ios::binary);
    std::ostream &out = std::cout;

    if (ind > optind)
      out << '\n';
    out << '<' << filename << '>' << '\n';

    pd_patch pat = pd_read_patch(in);

    unsigned adc_channels = pd_patch_adc_channels(pat);
    unsigned dac_channels = pd_patch_dac_channels(pat);
    out << "   -- adc channels: " << adc_channels << '\n';
    out << "   -- dac channels: " << dac_channels << '\n';

    bool has_midi_in = pd_patch_midi_in(pat);
    bool has_midi_out = pd_patch_midi_out(pat);
    out << "   -- midi in: " << (has_midi_in ? "yes" : "no") << '\n';
    out << "   -- midi out: " << (has_midi_out ? "yes" : "no") << '\n';

    int pos[2] {};
    unsigned size[2] {};
    unsigned fs {};
    if (pd_patch_root_canvas(pat, pos, size, &fs)) {
      out << "   -- root canvas position: " << pos[0] << ' ' << pos[1] << '\n';
      out << "   -- root canvas size: " << size[0] << ' ' << size[1] << '\n';
      out << "   -- root canvas font: " << fs << '\n';
    }

    out << pat;
  }

  return 0;
}
