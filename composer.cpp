#include <forward_list>
#include <iostream>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class regex_iter_match {
  // A range-for compatible regular expression iterator.
 public:
  typedef std::regex_iterator<std::string::const_iterator> iterator;

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
  const char species;
  const char size;

  Stem(const char species, const char size) : species(species), size(size) {
    if (species < 'a' || 'z' < species) {
      auto err_msg = std::string("Species not in range a-z: ") + species;
      throw std::invalid_argument(err_msg);
    }
    if (size != 'S' && size != 'L')
      throw std::invalid_argument(std::string("Size not one of S, L: ") + size);
  }

  bool operator<(const Stem& other) const {
    // Small stems before large, species in alphabetical order.
    return (size == other.size) ? species < other.species : size > other.size;
  }
  bool operator==(const Stem& other) const {
    // Equality if all members are equal
    return species == other.species && size == other.size;
  }
  static Stem from_string(const std::string_view spec) {
    if (spec.size() != 2)
      throw std::invalid_argument(
          "Species should be initiated from 2-char string.");
    return Stem{spec[0], spec[1]};
  }

 private:
  friend std::ostream& operator<<(std::ostream& os, const Stem& stem) {
    // Output streaming for Stem objects
    return (os << stem.species << stem.size);
  }
};

namespace std {
template <>
struct std::hash<Stem> {
  size_t operator()(const Stem& stem) const {
    return stem.size << 8 | stem.species;
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
    return out << req.count << req.stem.species;
  }
};

class Design {
 public:
  const char name;
  const char size;
  const std::forward_list<StemCount> required;
  const int total;

  Design(const char name,
         const char size,
         const std::forward_list<StemCount> required,
         const int total)
      : name(name), size(size), required(required), total(total) {}

  static Design from_string(const std::string spec) {
    std::regex pat_design{R"(([A-Z])([SL])((?:\d+[a-z])+)(\d+))"};
    std::regex pat_spec{R"((\d+)([a-z]))"};
    std::smatch match;
    if (!std::regex_match(spec, match, pat_design))
      throw std::invalid_argument(std::string("Not a valid pattern: ") + spec);
    auto name = match[1].first[0];
    auto stem_size = match[2].first[0];
    auto bouquet_size = stoi(match[4]);

    // Determine raw maximums per stem species
    std::vector<StemCount> raw_stem_specifications;
    for (const auto& spec_match : regex_iter_match(match[3], pat_spec)) {
      Stem stem{spec_match[2].first[0], stem_size};
      raw_stem_specifications.push_back({stem, stoi(spec_match[1])});
    }

    // Determine bounded maximums for stem requirements
    int stem_count = raw_stem_specifications.size();
    auto any_stem_max = bouquet_size - stem_count + 1;
    std::forward_list<StemCount> required;
    for (const auto& spec : raw_stem_specifications) {
      const auto max_required = std::min(spec.count, any_stem_max);
      if (max_required < 1)
        throw std::invalid_argument("Stem count must be a positive int");
      required.push_front(StemCount{std::move(spec.stem), max_required});
    }
    return Design{name, stem_size, required, bouquet_size};
  }

 private:
  friend std::ostream& operator<<(std::ostream& out, const Design& design) {
    // Output streaming for Design objects
    out << "Design " << design.name << "[" << design.size << "] takes: ";
    for (const auto& req : design.required)
      out << req;
    return out << " with total " << design.total;
  }
};

class Bouquet {
 public:
  Bouquet(const Design& design, const std::forward_list<StemCount> arrangement)
      : design(design), arrangement(arrangement) {}

 private:
  const Design& design;
  const std::forward_list<StemCount> arrangement;

  friend std::ostream& operator<<(std::ostream& out, const Bouquet& bouquet) {
    // Output streaming and formatting for Bouquet objects
    out << bouquet.design.name << bouquet.design.size;
    for (const auto& spec : bouquet.arrangement)
      out << spec;
    return out;
  }
};

class Composer {
 public:
  void add_design(const Design design) {
    for (const auto& req : design.required)
      designs[req.stem].push_front(design);
  }

  const Stem add_stem(const Stem stem) {
    supply[stem] += 1;
    return stem;
  }

  std::optional<Bouquet> bouquet_for_stem(const Stem& stem) {
    // Returns a Bouquet if one could be created given a newly inserted stem.
    for (const auto& design : designs[stem]) {
      std::forward_list<StemCount> arrangement;
      if (_extract(design, arrangement)) {
        return Bouquet{design, arrangement};
      } else {
        _restore(arrangement);
      }
    }
    return std::nullopt;
  }

 private:
  std::unordered_map<Stem, int> supply;
  std::unordered_map<Stem, std::forward_list<Design>> designs;

  bool _extract(const Design& design,
                std::forward_list<StemCount>& arrangement) {
    // Moves flowers from the supply into the given arrangement.
    auto remaining = design.total;
    for (const auto& req : design.required) {
      if (auto& entry = supply[req.stem]) {
        auto take = std::min({req.count, entry, remaining});
        arrangement.push_front({req.stem, take});
        entry -= take;
        remaining -= take;
      } else {
        return false;
      }
    }
    return remaining == 0;
  }

  void _restore(const std::forward_list<StemCount>& arrangement) {
    // Adds the stems in the arrangement back to the supply.
    for (const auto& spec : arrangement)
      supply[spec.stem] += spec.count;
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
    composer.add_design(Design::from_string(line));

  for (std::string line; readline(line);) {
    auto stem = composer.add_stem(Stem::from_string(line));
    if (auto bouquet = composer.bouquet_for_stem(stem))
      std::cout << *bouquet << std::endl;
  }
}
