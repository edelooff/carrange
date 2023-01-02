#include <compare>
#include <iostream>
#include <map>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class regex_iter_match {
  // A range-for compatible regular expression iterator.
 public:
  using iterator = std::regex_iterator<std::string::const_iterator>;

  regex_iter_match(const std::string& s, const std::string& expr)
      : _expr(expr), _begin(s.begin(), s.end(), _expr), _end() {}
  regex_iter_match(const std::ssub_match& m, const std::regex& expr)
      : _expr(expr), _begin(m.first, m.second, _expr), _end() {}

  iterator begin() const { return _begin; }
  iterator end() const { return _end; }

 private:
  std::regex _expr;
  iterator _begin;
  iterator _end;
};

class Stem {
 public:
  Stem(const std::string spec) {
    if (spec.size() != 2)
      throw std::invalid_argument("Stem constructor takes 2-character string.");

    species = spec[0];
    size = spec[1];
    // Invariant checks
    if (species < 'a' || 'z' < species) {
      auto err_msg = std::string("Species not in range a-z: ") + species;
      throw std::invalid_argument(err_msg);
    }
    if (size != 'S' && size != 'L')
      throw std::invalid_argument(std::string("Size not one of S, L: ") + size);
  }

  bool operator==(const Stem&) const = default;
  auto operator<=>(const Stem&) const = default;
  char get_species() const { return species; }

  friend std::hash<Stem>;

 private:
  char species;
  char size;

  friend std::ostream& operator<<(std::ostream& out, const Stem& stem) {
    // Output streaming for Stem objects
    return out << stem.species << stem.size;
  }
};

namespace std {
template <>
struct std::hash<Stem> {
  size_t operator()(const Stem& stem) const {
    return (stem.size << 8) | stem.species;
  }
};
}  // namespace std

class StemCount {
 public:
  const Stem stem;
  const int count;
  StemCount(const Stem stem, const int count) : stem(stem), count(count) {}

 private:
  friend std::ostream& operator<<(std::ostream& out, const StemCount& req) {
    // Output streaming for StemCount objects
    return out << req.count << req.stem.get_species();
  }
};

class Design {
 public:
  Design(const std::string spec) {
    std::smatch match;
    if (!std::regex_match(spec, match, _re_design))
      throw std::invalid_argument(std::string("Not a valid pattern: ") + spec);
    const auto stem_size = match[2].str();
    _total = stoi(match[4]);
    _code = match[1].str() + stem_size;

    // Determine raw maximums per stem species
    std::map<Stem, int> raw_stem_counts;
    for (const auto& stem_match : regex_iter_match(match[3], _re_stems)) {
      const int stem_count = stoi(stem_match[1]);
      const char stem_species = stem_match[2].first[0];
      raw_stem_counts.emplace(stem_species + stem_size, stem_count);
    }

    // Store bounded maximums per stem in design
    const int any_stem_max = _total - raw_stem_counts.size() + 1;
    for (const auto& [stem, count] : raw_stem_counts) {
      const auto stem_max = std::min(count, any_stem_max);
      if (stem_max < 1)
        throw std::invalid_argument("Stem count must be a positive int");
      _stem_counts.emplace_back(stem, stem_max);
    }
  }

  const std::string& code() const { return _code; }
  const std::vector<StemCount>& stem_counts() const { return _stem_counts; }
  int total() const { return _total; }

 private:
  std::string _code;
  std::vector<StemCount> _stem_counts;
  int _total;

  static const std::regex _re_design;
  static const std::regex _re_stems;

  friend std::ostream& operator<<(std::ostream& out, const Design& design) {
    // Output streaming for Design objects
    out << "Design " << design._code << " with stem options ";
    for (const auto& req : design._stem_counts)
      out << req;
    return out << " and total " << design._total;
  }
};

const std::regex Design::_re_design{R"(([A-Z])([SL])((?:\d+[a-z])+)(\d+))"};
const std::regex Design::_re_stems{R"((\d+)([a-z]))"};

class Bouquet {
 public:
  Bouquet(const std::string& code, std::vector<StemCount> arrangement)
      : code(code), arrangement(arrangement) {}

 private:
  const std::string& code;
  std::vector<StemCount> arrangement;

  friend std::ostream& operator<<(std::ostream& out, const Bouquet& bouquet) {
    // Output streaming and formatting for Bouquet objects
    out << bouquet.code;
    for (const auto& spec : bouquet.arrangement)
      out << spec;
    return out;
  }
};

class Composer {
 public:
  void add_design(const Design design) {
    workspace.reserve(design.stem_counts().size());
    for (const auto& req : design.stem_counts())
      designs[req.stem].push_back(design);
  }

  void add_stem(const Stem& stem) { supply[stem] += 1; }

  std::optional<Bouquet> bouquet_for_stem(const Stem& stem) {
    // Returns an optional Bouquet, created from a Design containing the Stem.
    for (const auto& design : designs[stem]) {
      if (_select_stems(design)) {
        _take_arrangement_from_supply();
        return Bouquet{design.code(), workspace};
      }
    }
    return std::nullopt;
  }

 private:
  std::vector<StemCount> workspace;
  std::unordered_map<Stem, int> supply;
  std::unordered_map<Stem, std::vector<Design>> designs;

  bool _select_stems(const Design& design) {
    // Selects stems of design into workspace and returns completion of bouquet.
    workspace.clear();
    auto remaining = design.total();
    auto remaining_options = design.stem_counts().size();
    for (const auto& option : design.stem_counts()) {
      if (const auto& available = supply[option.stem]) {
        const int maximum_take = remaining - (--remaining_options);
        const auto take = std::min({available, option.count, maximum_take});
        workspace.emplace_back(option.stem, take);
        remaining -= take;
      } else {
        return false;
      }
    }
    return remaining == 0;
  }

  void _take_arrangement_from_supply() {
    // Removes the stems in the workspace from the supply.
    for (const auto& spec : workspace)
      supply[spec.stem] -= spec.count;
  }
};

bool readline(std::string& line, std::istream& source = std::cin) {
  // Reads a line, signaling the end of a paragraph in addition to EOF.
  std::getline(source, line);
  return source && line.size();
}

int main() {
  std::ios::sync_with_stdio(false);
  Composer composer;

  for (std::string line; readline(line);)
    composer.add_design(line);

  for (std::string line; readline(line);) {
    const Stem stem{line};
    composer.add_stem(stem);
    if (auto bouquet = composer.bouquet_for_stem(stem))
      std::cout << *bouquet << std::endl;
  }
}
