#include "fable/defs/compiler/def_reader.hpp"

namespace fable::defs::compiler {

bool parse_entry(const BinaryEntry &entry, ParsedEntry &out, std::string &error) {
    out = ParsedEntry{};
    out.def_type = entry.def_name;
    out.is_template = entry.is_template;

    bool found = false;
    bool parsed = false;
    auto parse = [&](auto value) {
        found = true;
        Reader reader(entry.body.data(), entry.body.size());
        Value root;
        if (!read_value(value, reader, root)) {
            error = "failed to read " + entry.def_name + " body";
            return;
        }
        // A few def classes (notably OPINION_SOURCE) have trailer controls
        // that the generic value reflection does not expose. Keep the parsed
        // prefix rather than rejecting the whole entry.
        out.fields = std::move(root.fields);
        parsed = true;
    };

    if (!with_class_for_type(entry.def_name, parse)) {
        with_class(entry.def_name, parse);
    }
    if (!found) {
        error = "no model class for def type '" + entry.def_name + "'";
        return false;
    }
    return parsed;
}

} // namespace fable::defs::compiler
