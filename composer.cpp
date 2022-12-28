#include <forward_list>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class regex_iter_match {
  // A range-for compatible regular expression iterator.
 public:
  typedef regex_iterator<string::const_iterator> iterator;

  regex_iter_match(const string& s, const string& expr)
      : _expr(expr), _begin(s.begin(), s.end(), _expr), _end() {}
  regex_iter_match(const ssub_match& m, const regex& expr)
      : _expr(expr), _begin(m.first, m.second, _expr), _end() {}

  iterator begin() const { return _begin; }
  iterator end() const { return _end; }

 private:
  regex _expr;
  iterator _begin;
  iterator _end;
};

class Stem {
 public:
  const char species;
  const char size;

  Stem(const char species, const char size) : species(species), size(size) {
    if (species < 'a' || 'z' < species)
      throw invalid_argument("Species not in range a-z: " + string(1, species));
    if (size != 'S' && size != 'L')
      throw invalid_argument("Size not one of S, L: " + string(1, size));
  }

  bool operator<(const Stem& other) const {
    // Small stems before large, species in alphabetical order.
    return (size == other.size) ? species < other.species : size > other.size;
  }
  bool operator==(const Stem& other) const {
    // Equality if all members are equal
    return species == other.species && size == other.size;
  }
  static Stem from_string(const string_view spec) {
    if (spec.size() != 2)
      throw invalid_argument("Species should be initiated from 2-char string.");
    return Stem{spec[0], spec[1]};
  }

 private:
  friend ostream& operator<<(ostream& os, const Stem& stem) {
    // Output streaming for Stem objects
    return (os << stem.species << stem.size);
  }
};

namespace std {
template <>
struct hash<Stem> {
  size_t operator()(const Stem& stem) const {
    return hash<char>()(stem.species) ^ (hash<char>()(stem.size) << 1);
  }
};
}  // namespace std

class StemCount {
 public:
  const Stem stem;
  const int count;
  StemCount(const Stem stem, const int count) : stem(stem), count(count) {}

 private:
  friend ostream& operator<<(ostream& out, StemCount const& req) {
    // Output streaming for StemCount objects
    return out << req.stem.species << req.count;
  }
};

class Design {
 public:
  const char name;
  const char size;
  const forward_list<StemCount> required;
  const int total;

  Design(const char name,
         const char size,
         const forward_list<StemCount> required,
         const int total)
      : name(name), size(size), required(required), total(total) {}

  static Design from_string(const string spec) {
    regex pat_design{R"(([A-Z])([SL])((?:\d+[a-z])+)(\d+))"};
    regex pat_spec{R"((\d+)([a-z]))"};
    smatch match;
    if (!regex_match(spec, match, pat_design))
      throw invalid_argument("Not a valid pattern: " + spec);
    auto name = match[1].first[0];
    auto stem_size = match[2].first[0];
    auto bouquet_size = stoi(match[4]);

    // Determine raw maximums per stem species
    unordered_map<char, int> requirements;
    for (const auto& spec_match : regex_iter_match(match[3], pat_spec))
      requirements[spec_match[2].first[0]] = stoi(spec_match[1]);

    // Determine bounded maximums for stem requirements
    int stem_count = requirements.size();
    auto any_stem_max = bouquet_size - stem_count + 1;
    forward_list<StemCount> required;
    for (const auto& [species, stem_max] : requirements) {
      const auto stem = Stem{species, stem_size};
      const auto max_required = min(stem_max, any_stem_max);
      if (max_required < 1)
        throw invalid_argument("Stem count must be a positive int");
      required.push_front(StemCount{stem, max_required});
    }
    return Design{name, stem_size, required, bouquet_size};
  }

 private:
  friend ostream& operator<<(ostream& out, Design const& design) {
    // Output streaming for Design objects
    out << "Design " << design.name << "[" << design.size << "] takes: ";
    for (const auto& req : design.required)
      out << req;
    return out << " with total " << design.total;
  }
};

class Bouquet {
 public:
  Bouquet(const Design& design, vector<StemCount> arrangement)
      : design(design), arrangement(arrangement) {}

 private:
  const Design& design;
  const vector<StemCount> arrangement;

  friend ostream& operator<<(ostream& out, Bouquet const& bouquet) {
    // Output streaming and formatting for Bouquet objects
    out << bouquet.design.name << bouquet.design.size;
    for (const auto& spec : bouquet.arrangement)
      out << spec.count << spec.stem.species;
    return out;
  }
};

bool extract(unordered_map<Stem, int>& supply,
             const Design& design,
             vector<StemCount>& arrangement) {
  // Takes flowers from the supply into the given arrangement, reports success.
  auto remaining = design.total;
  for (const auto& req : design.required) {
    auto& entry = supply[req.stem];
    if (!entry)
      return false;
    auto take = min({req.count, entry, remaining});
    arrangement.push_back({req.stem, take});
    entry -= take;
    remaining -= take;
  }
  return remaining == 0;
}

void restore(unordered_map<Stem, int>& supply,
            const vector<StemCount>& arrangement) {
  for (const auto& spec : arrangement)
    supply[spec.stem] += spec.count;
}

bool readline(string& line) {
  // Reads a line, signaling the end of a paragraph in addition to EOF.
  getline(cin, line);
  return cin && line.size();
}

int main() {
  ios::sync_with_stdio(false);
  unordered_map<Stem, int> flower_counts;
  unordered_map<Stem, forward_list<Design>> designs_by_stem;

  for (string line; readline(line);) {
    auto design = Design::from_string(line);
    for (const auto& req : design.required)
      designs_by_stem[req.stem].push_front(design);
  }

  for (string line; readline(line);) {
    const auto stem = Stem::from_string(line);
    flower_counts[stem] += 1;
    for (const auto& design : designs_by_stem[stem]) {
      vector<StemCount> arrangement;
      if (extract(flower_counts, design, arrangement)) {
        auto bouquet = Bouquet(design, arrangement);
        cout << bouquet << "\n";
      } else {
        restore(flower_counts, arrangement);
      }
    }
  }
}
