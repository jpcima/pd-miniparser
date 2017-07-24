/*
    Copyright Jean-Pierre Cimalando 2017

    Distributed under the Boost Software License, Version 1.0.
    http://www.boost.org/LICENSE_1_0.txt
*/

#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <iosfwd>

enum class pd_atomtype {
  Null = 0,
  Float = 1,
  Symbol = 2,
};

struct pd_atom {
  pd_atomtype type = pd_atomtype::Null;
  union {
    double flt;
    const std::string *str;
  };
};

typedef std::unordered_set<std::string> pd_symtab;

struct pd_record {
  std::vector<pd_atom> atoms;
};

struct pd_patch {
  std::vector<pd_record> records;
  pd_symtab symtab;
};

struct pd_miniparser_exception : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

///
pd_patch pd_read_patch(std::istream &in);

///
bool pd_parse_obj(
    const pd_record &rec, int *px, int *py,
    size_t *pargc, const pd_atom **pargv);
bool pd_parse_cmd(
    const pd_record &rec, int *px, int *py,
    const std::string **pcmd, size_t *pargc, const pd_atom **pargv);

///
unsigned pd_patch_adc_channels(const pd_patch &pat);
unsigned pd_patch_dac_channels(const pd_patch &pat);
bool pd_patch_midi_in(const pd_patch &pat);
bool pd_patch_midi_out(const pd_patch &pat);
bool pd_patch_root_canvas(
    const pd_patch &pat, int pos[2], unsigned size[2], unsigned *fs);

///
std::ostream &operator<<(std::ostream &out, const pd_patch &pat);
std::ostream &operator<<(std::ostream &out, const pd_record &rec);
std::ostream &operator<<(std::ostream &out, const pd_atom &atom);
