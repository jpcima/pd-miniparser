/*
    Copyright Jean-Pierre Cimalando 2017

    Distributed under the Boost Software License, Version 1.0.
    http://www.boost.org/LICENSE_1_0.txt
*/

#include "pd-miniparser.h"
#include <array>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cmath>

template <class T>
static bool can_convert_real(double in) {
  static_assert(std::is_integral<T>(), "T is not an integral type");
  if (!std::isfinite(in))
    return false;
  double t = std::trunc(in);
  if (in != t)
    return false;
  if (t > std::numeric_limits<T>::max() || t < std::numeric_limits<T>::min())
    return false;
  return true;
}

///
static pd_atom pd_atom_of_token(const std::string &tok, pd_symtab &symtab) {
  pd_atom atom;
  if (!tok.empty()) {
    double flt {};
    unsigned fltn {};
    if (sscanf(tok.c_str(), "%lf%n", &flt, &fltn) == 1 && fltn == tok.size()) {
      atom.type = pd_atomtype::Float;
      atom.flt = flt;
    } else {
      atom.type = pd_atomtype::Symbol;
      atom.str = &*symtab.insert(tok).first;
    }
  }
  return atom;
}

static int pd_getchar(std::istream &in) {
  int c = in.get();
  if (in.bad()) throw pd_miniparser_exception("Input error reading pd patch");
  return c;
}

static int pd_ensure_getchar(std::istream &in) {
  int c = in.get();
  if (in.bad()) throw pd_miniparser_exception("Input error reading pd patch");
  if (in.fail()) throw pd_miniparser_exception("Premature end reading pd patch");
  return c;
}

static bool pd_is_ws(int c) {
  return c == ' ' || c == '\r' || c == '\n';
}

static unsigned pd_skip_ws(std::istream &in) {
  unsigned n = 0;
  for (; pd_is_ws(in.peek()); ++n)
    in.get();
  return n;
}

///
pd_patch pd_read_patch(std::istream &in) {
  pd_patch pat;
  pd_record rec;
  std::string tok;
  bool newtok = true;
  pd_skip_ws(in);
  for (int ch; (ch = pd_getchar(in)) != EOF;) {
    if (ch == ';') {
      if (!tok.empty()) {
        rec.atoms.push_back(pd_atom_of_token(tok, pat.symtab));
        tok.clear();
      }
      pat.records.push_back(std::move(rec));
      rec = pd_record();
      pd_skip_ws(in);
      newtok = true;
    } else {
      if (newtok && !tok.empty()) {
        rec.atoms.push_back(pd_atom_of_token(tok, pat.symtab));
        tok.clear();
      }
      if (ch == '\\') {
        ch = pd_ensure_getchar(in);
        if ((ch >= 32 && ch <= 47) ||
            (ch >= 58 && ch <= 64) ||
            (ch >= 91 && ch <= 96) ||
            (ch >= 123 && ch <= 126)) {
        } else {
          throw pd_miniparser_exception(
              "Unrecognized escape sequence reading pd patch");
        }
      }
      tok.push_back(ch);
      newtok = pd_skip_ws(in);
    }
  }
  if (!tok.empty())
    throw pd_miniparser_exception("Premature end reading pd patch");
  return pat;
}

///
bool pd_parse_obj(
    const pd_record &rec, int *px, int *py,
    size_t *pargc, const pd_atom **pargv) {
  const std::array<pd_atomtype, 4> argtypes = {
    pd_atomtype::Symbol, pd_atomtype::Symbol,
    pd_atomtype::Float, pd_atomtype::Float,
  };

  bool valid = rec.atoms.size() >= argtypes.size();
  for (size_t i = 0, n = argtypes.size(); i < n && valid; ++i)
    valid = rec.atoms[i].type == argtypes[i];
  valid = valid &&
          *rec.atoms[0].str == "#X" && *rec.atoms[1].str == "obj" &&
          can_convert_real<int>(rec.atoms[2].flt) &&
          can_convert_real<int>(rec.atoms[3].flt);
  if (!valid)
    return false;

  if (px) *px = rec.atoms[2].flt;
  if (py) *py = rec.atoms[3].flt;
  if (pargc) *pargc = rec.atoms.size() - 4;
  if (pargv) *pargv = rec.atoms.data() + 4;
  return true;
}

bool pd_parse_cmd(
    const pd_record &rec, int *px, int *py,
    const std::string **pcmd, size_t *pargc, const pd_atom **pargv) {
  int x;
  int y;
  size_t argc;
  const pd_atom *argv;
  if (!pd_parse_obj(rec, &x, &y, &argc, &argv) ||
      argc == 0 || argv[0].type != pd_atomtype::Symbol)
    return false;

  if (px) *px = x;
  if (py) *py = y;
  if (pcmd) *pcmd = argv[0].str;
  if (pargc) *pargc = argc - 1;
  if (pargv) *pargv = argv + 1;
  return true;
}

///
static unsigned pd_parse_channels(size_t argc, const pd_atom *argv) {
  if (argc == 0)
    return 2;

  unsigned maxchannel = 0;

  bool valid = true;
  for (size_t i = 0; i < argc && valid; ++i)
    valid = argv[i].type == pd_atomtype::Float &&
            can_convert_real<unsigned>(argv[i].flt) &&
            unsigned(argv[i].flt) > 0;
  if (!valid)
    return 0;

  for (size_t i = 0; i < argc; ++i) {
    unsigned channel = argv[i].flt;
    maxchannel = std::max(maxchannel, channel);
  }
  return maxchannel;
}

unsigned pd_patch_adc_channels(const pd_patch &pat) {
  unsigned channels = 0;
  for (const pd_record &rec : pat.records) {
    const std::string *cmd;
    size_t argc;
    const pd_atom *argv;
    if (pd_parse_cmd(rec, nullptr, nullptr, &cmd, &argc, &argv) &&
        *cmd == "adc~")
      channels = std::max(pd_parse_channels(argc, argv), channels);
  }
  return channels;
}

unsigned pd_patch_dac_channels(const pd_patch &pat) {
  unsigned channels = 0;
  for (const pd_record &rec : pat.records) {
    const std::string *cmd;
    size_t argc;
    const pd_atom *argv;
    if (pd_parse_cmd(rec, nullptr, nullptr, &cmd, &argc, &argv) &&
        *cmd == "dac~")
      channels = std::max(pd_parse_channels(argc, argv), channels);
  }
  return channels;
}

bool pd_patch_midi_in(const pd_patch &pat) {
  std::array<const char *, 10> cmds {
    "notein", "ctlin", "pgmin", "bendin", "touchin", "polytouchin",
    "midiin", "sysexin", "midirealtimein", "midiclkin" };
  for (const pd_record &rec : pat.records) {
    const std::string *cmd;
    if (pd_parse_cmd(rec, nullptr, nullptr, &cmd, nullptr, nullptr))
      if (std::find(cmds.begin(), cmds.end(), *cmd) != cmds.end())
        return true;
  }
  return false;
}

bool pd_patch_midi_out(const pd_patch &pat) {
  static std::array<const char *, 7> cmds = {
    "noteout", "ctlout", "pgmout", "bendout", "touchout", "polytouchout",
    "midiout" };
  for (const pd_record &rec : pat.records) {
    const std::string *cmd;
    if (pd_parse_cmd(rec, nullptr, nullptr, &cmd, nullptr, nullptr))
      if (std::find(cmds.begin(), cmds.end(), *cmd) != cmds.end())
        return true;
  }
  return false;
}

bool pd_patch_root_canvas(
    const pd_patch &pat, int pos[2], unsigned size[2], unsigned *fs) {
  const std::array<pd_atomtype, 7> argtypes = {
    pd_atomtype::Symbol, pd_atomtype::Symbol,
    pd_atomtype::Float, pd_atomtype::Float,
    pd_atomtype::Float, pd_atomtype::Float, pd_atomtype::Float,
  };

  if (pat.records.empty())
    return false;

  const pd_record &rec = pat.records.front();
  bool valid = rec.atoms.size() == argtypes.size();
  for (size_t i = 0, n = argtypes.size(); i < n && valid; ++i)
    valid = rec.atoms[i].type == argtypes[i];
  valid = valid &&
          *rec.atoms[0].str == "#N" && *rec.atoms[1].str == "canvas" &&
          can_convert_real<int>(rec.atoms[2].flt) &&
          can_convert_real<int>(rec.atoms[3].flt) &&
          can_convert_real<unsigned>(rec.atoms[4].flt) &&
          can_convert_real<unsigned>(rec.atoms[5].flt) &&
          can_convert_real<unsigned>(rec.atoms[6].flt);
  if (!valid)
    return false;

  if (pos) {
    pos[0] = rec.atoms[2].flt;
    pos[1] = rec.atoms[3].flt;
  }
  if (size) {
    size[0] = rec.atoms[4].flt;
    size[1] = rec.atoms[5].flt;
  }
  if (fs)
    *fs = rec.atoms[6].flt;
  return true;
}

///
std::ostream &operator<<(std::ostream &out, const pd_patch &pat) {
  for (size_t i = 0, n = pat.records.size(); i < n; ++i) {
    const pd_record &rec = pat.records[i];
    out << std::setw(6) << i << ":   " << rec << '\n';
  }
  return out;
}

std::ostream &operator<<(std::ostream &out, const pd_record &rec) {
  for (size_t i = 0, n = rec.atoms.size(); i < n; ++i) {
    const pd_atom &atom = rec.atoms[i];
    out << (i ? " " : "") << atom;
  }
  return out;
}

std::ostream &operator<<(std::ostream &out, const pd_atom &atom) {
  switch (atom.type) {
    case pd_atomtype::Null:
      out << "#n()"; break;
    case pd_atomtype::Float:
      out << "#f(" << atom.flt << ")"; break;
    case pd_atomtype::Symbol:
      out << "#s(" << *atom.str << ")"; break;
  }
  return out;
}
