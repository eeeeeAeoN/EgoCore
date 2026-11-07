#include "fable/defs/compiler/build_pipeline.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <fable/defs/transfer.hpp>

#include "fable/defs/compiler/build_index.hpp"
#include "fable/defs/compiler/definitions.hpp"
#include "fable/defs/compiler/lower.hpp"
#include "fable/defs/compiler/model_registry.hpp"
#include "fable/defs/compiler/type_registry.hpp"
#include "fable/defs/compiler/wire.hpp"

namespace fable::defs::compiler {

namespace {

struct SubDefRecord {
    std::uint32_t name_crc = 0;
    std::uint32_t def_index = 0;
    std::uint32_t owner_index = 0;
};

struct BuiltEntry {
    std::uint32_t def_name_off = 0;
    std::uint32_t file_name_off = 0;
    std::uint32_t counter = 0;
    bool is_real = false;
    bool is_template = false;
    std::uint8_t unknown_0 = 0;
    bool has_subdefs = false;
    std::vector<SubDefRecord> subdefs;
    std::vector<std::uint8_t> body;
};

struct BodyOutcome {
    bool found = false;
    std::vector<std::uint8_t> body;
    std::size_t unsupported = 0;
    std::string error;
};

template <class F>
bool dispatch_class(const std::string &type, F &&f) {
    if (with_class_for_type(type, f)) {
        return true;
    }
    return with_class(type, f);
}

template <class F>
bool dispatch_type(const std::string &type, F &&f) {
    return with_class_for_type(type, f);
}

BodyOutcome make_default_body(const std::string &type, NamesBuilder &names) {
    BodyOutcome out;
    out.found = dispatch_type(type, [&](auto value) {
        apply_schema_defaults(value, &names);
        ByteWriter writer;
        WireStats stats;
        serialize_def(value, writer, stats);
        out.body = writer.take();
        out.unsupported = stats.unsupported;
    });
    if (!out.found) {
        out.error = "no model class for NULLDEF type '" + type + "'";
    }
    return out;
}

struct SubBlock {
    std::uint32_t name_crc = 0;
    std::string tag;
    std::vector<Statement> body;
};

std::vector<SubBlock> collect_sub_blocks(const Chain &chain) {
    std::map<std::uint32_t, SubBlock> blocks;
    for (const Definition *definition : chain) {
        for (const Statement &statement : definition->body) {
            if (statement.kind != Statement::Kind::TaggedBlock || !statement.tagged_block) {
                continue;
            }
            const TaggedBlock &tagged = *statement.tagged_block;
            const std::uint32_t name_crc = crc32(tagged.tag);
            auto [it, inserted] = blocks.try_emplace(name_crc);
            if (inserted) {
                it->second.name_crc = name_crc;
                it->second.tag = tagged.tag;
            }
            it->second.body.insert(it->second.body.end(), tagged.body.begin(),
                                   tagged.body.end());
        }
    }
    std::vector<SubBlock> out;
    out.reserve(blocks.size());
    for (auto &[crc, block] : blocks) {
        (void)crc;
        out.push_back(std::move(block));
    }
    return out;
}

BodyOutcome lower_subdef(const SubBlock &block, const DefinitionIndex &index,
                         const SymbolTable &symbols, NamesBuilder &names) {
    Definition definition;
    definition.def_type = block.tag;
    definition.name = block.tag;
    definition.body = block.body;
    BodyOutcome out;
    out.found = dispatch_class(block.tag, [&](auto value) {
        LowerStats lower_stats;
        lower_definition(definition, value, symbols, lower_stats, nullptr, &index, &names);
        if (!lower_stats.errors.empty()) {
            out.error = lower_stats.errors.front().message;
        }
        ByteWriter writer;
        WireStats wire_stats;
        serialize_def(value, writer, wire_stats);
        out.body = writer.take();
        out.unsupported = wire_stats.unsupported;
    });
    if (!out.found) {
        out.error = "no model class for sub-def tag '" + block.tag + "'";
    }
    return out;
}

BodyOutcome lower_named(const Definition &definition, const DefinitionIndex &index,
                        const SymbolTable &symbols, NamesBuilder &names) {
    BodyOutcome out;
    const auto chain_result = specialization_chain(index, definition.name);
    if (const auto *error = std::get_if<ChainError>(&chain_result)) {
        out.error = error->message;
        return out;
    }
    const Chain &chain = std::get<Chain>(chain_result);
    Definition flattened = flatten_chain(chain);

    out.found = dispatch_class(definition.def_type, [&](auto value) {
        LowerStats lower_stats;
        lower_definition(flattened, value, symbols, lower_stats, nullptr, &index, &names);
        if (!lower_stats.errors.empty()) {
            out.error = lower_stats.errors.front().message;
        }
        ByteWriter writer;
        WireStats wire_stats;
        serialize_def(value, writer, wire_stats);
        out.body = writer.take();
        out.unsupported = wire_stats.unsupported;
    });
    if (!out.found) {
        out.error = "no model class for def type '" + definition.def_type + "'";
    }
    return out;
}

std::uint32_t next_class_index(std::map<std::string, std::uint32_t> &counters,
                               const std::string &class_name) {
    auto &slot = counters[class_name];
    return slot++;
}

// ── file scoping ───────────────────────────────────────────────────────────

bool path_ends_with(std::string_view path, std::string_view suffix) {
    if (path.size() < suffix.size()) {
        return false;
    }
    if (path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0) {
        return false;
    }
    return path.size() == suffix.size() || path[path.size() - suffix.size() - 1] == '/';
}

std::vector<const ParsedFile *> scope_files(const std::vector<ParsedFile> &files,
                                             BinaryKind kind) {
    std::vector<const ParsedFile *> out;
    if (kind == BinaryKind::Game) {
        out.reserve(files.size());
        for (const auto &file : files) {
            // Frontend and script definitions belong to their own binaries;
            // including them in game.bin both bloats it and changes which
            // duplicate name wins in the shared index space.
            const bool in_frontend =
                file.path.find("/FrontEndDefs/") != std::string::npos ||
                file.path.rfind("FrontEndDefs/", 0) == 0;
            const bool in_script =
                file.path.find("/ScriptDefs/") != std::string::npos ||
                file.path.rfind("ScriptDefs/", 0) == 0;
            if (in_frontend || in_script) {
                continue;
            }
            out.push_back(&file);
        }
        return out;
    }

    if (kind == BinaryKind::Frontend) {
        for (std::size_t i = 0; i < FRONTEND_DEF_FILE_COUNT; ++i) {
            const std::string_view rel = FRONTEND_DEF_FILES[i];
            for (const auto &file : files) {
                if (path_ends_with(file.path, rel)) {
                    out.push_back(&file);
                    break;
                }
            }
        }
        for (const auto &file : files) {
            const bool in_frontend =
                file.path.find("/FrontEndDefs/") != std::string::npos ||
                file.path.rfind("FrontEndDefs/", 0) == 0;
            if (!in_frontend) {
                continue;
            }
            if (std::find(out.begin(), out.end(), &file) == out.end()) {
                out.push_back(&file);
            }
        }
        return out;
    }

    for (const auto &file : files) {
        const bool in_script = file.path.find("/ScriptDefs/") != std::string::npos ||
                               file.path.rfind("ScriptDefs/", 0) == 0;
        if (in_script) {
            out.push_back(&file);
        }
    }
    return out;
}

// ── entry and chunk serialization ──────────────────────────────────────────

void append_entry(ByteWriter &out, const BuiltEntry &entry) {
    out.u8(entry.is_real ? 1 : 0);
    out.u8(entry.is_template ? 1 : 0);
    out.u8(entry.unknown_0);
    if (entry.has_subdefs) {
        out.u16(static_cast<std::uint16_t>(entry.subdefs.size()));
        for (const SubDefRecord &record : entry.subdefs) {
            out.u32(record.name_crc);
            out.u32(record.def_index);
            out.u32(record.owner_index);
        }
    }
    out.bytes(entry.body.data(), entry.body.size());
}

std::size_t entry_size(const BuiltEntry &entry) {
    return 3 + (entry.has_subdefs ? 2 + entry.subdefs.size() * 12 : 0) +
           entry.body.size();
}

bool compress_chunk(const std::vector<const BuiltEntry *> &entries,
                    std::vector<std::uint8_t> &compressed, std::string &error) {
    const std::size_t table_size = entries.size() * 2;
    std::size_t payload_size = 0;
    for (const BuiltEntry *entry : entries) {
        payload_size += entry_size(*entry);
    }
    if (table_size + payload_size > 0xFFFF) {
        error = "chunk exceeds the 16-bit entry offset table";
        return false;
    }

    std::vector<std::uint8_t> raw(table_size, 0);
    std::uint32_t offset = static_cast<std::uint32_t>(table_size);
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const std::size_t size = entry_size(*entries[i]);
        if (offset > 0xFFFF) {
            error = "entry offset exceeds 16 bits";
            return false;
        }
        raw[i * 2] = static_cast<std::uint8_t>(offset);
        raw[i * 2 + 1] = static_cast<std::uint8_t>(offset >> 8);
        offset += static_cast<std::uint32_t>(size);
    }

    ByteWriter payload;
    for (const BuiltEntry *entry : entries) {
        append_entry(payload, *entry);
    }
    raw.insert(raw.end(), payload.data().begin(), payload.data().end());

    // A valid zlib stream made of stored DEFLATE blocks. The retail files use
    // level-1 deflate; stored blocks are larger but decode identically, and
    // keep the C++ port free of a zlib dependency while the container layer is
    // being brought up. Replace with miniz/zlib once byte parity is the goal.
    compressed.clear();
    compressed.push_back(0x78); // CMF: deflate, 32K window
    compressed.push_back(0x01); // FLG: no preset dictionary, level 1
    std::size_t position = 0;
    do {
        const std::size_t block = std::min<std::size_t>(65535, raw.size() - position);
        const bool final = position + block == raw.size();
        compressed.push_back(final ? 1 : 0);
        compressed.push_back(static_cast<std::uint8_t>(block));
        compressed.push_back(static_cast<std::uint8_t>(block >> 8));
        const std::uint16_t nlen = static_cast<std::uint16_t>(~block);
        compressed.push_back(static_cast<std::uint8_t>(nlen));
        compressed.push_back(static_cast<std::uint8_t>(nlen >> 8));
        compressed.insert(compressed.end(), raw.begin() + static_cast<std::ptrdiff_t>(position),
                          raw.begin() + static_cast<std::ptrdiff_t>(position + block));
        position += block;
    } while (position < raw.size());

    std::uint32_t a = 1;
    std::uint32_t b = 0;
    for (std::uint8_t byte : raw) {
        a = (a + byte) % 65521;
        b = (b + a) % 65521;
    }
    const std::uint32_t adler = (b << 16) | a;
    compressed.push_back(static_cast<std::uint8_t>(adler >> 24));
    compressed.push_back(static_cast<std::uint8_t>(adler >> 16));
    compressed.push_back(static_cast<std::uint8_t>(adler >> 8));
    compressed.push_back(static_cast<std::uint8_t>(adler));
    return true;
}

bool write_binary(const std::filesystem::path &path, const std::vector<BuiltEntry> &entries,
                  const BinaryHeaderInfo &header, std::string &error) {
    std::vector<std::vector<std::uint8_t>> chunks;
    std::vector<std::uint32_t> chunk_counts;

    std::size_t start = 0;
    std::size_t size = 0;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const std::size_t next_size = entry_size(entries[i]);
        if (size > 0 && size + next_size > 16384) {
            std::vector<const BuiltEntry *> group;
            for (std::size_t j = start; j < i; ++j) {
                group.push_back(&entries[j]);
            }
            std::vector<std::uint8_t> compressed;
            if (!compress_chunk(group, compressed, error)) {
                return false;
            }
            chunks.push_back(std::move(compressed));
            chunk_counts.push_back(static_cast<std::uint32_t>(group.size()));
            start = i;
            size = 0;
        }
        size += next_size;
    }
    if (start < entries.size()) {
        std::vector<const BuiltEntry *> group;
        for (std::size_t j = start; j < entries.size(); ++j) {
            group.push_back(&entries[j]);
        }
        std::vector<std::uint8_t> compressed;
        if (!compress_chunk(group, compressed, error)) {
            return false;
        }
        chunks.push_back(std::move(compressed));
        chunk_counts.push_back(static_cast<std::uint32_t>(group.size()));
    }

    ByteWriter out;
    out.u8(header.use_names_bin ? 1 : 0);
    out.u32(header.file_indicator);
    out.u32(header.platform_indicator);
    out.u32(static_cast<std::uint32_t>(entries.size()));

    for (const BuiltEntry &entry : entries) {
        out.u32(entry.def_name_off);
        out.u32(entry.file_name_off);
        out.u32(entry.counter);
    }

    const std::uint32_t chunk_count = static_cast<std::uint32_t>(chunks.size() + 1);
    out.u32(chunk_count);
    out.u32(0);

    std::uint32_t relative = 0;
    std::uint32_t cumulative = 0;
    for (std::size_t i = 0; i < chunks.size(); ++i) {
        cumulative += chunk_counts[i];
        out.u32(relative);
        out.u32(cumulative);
        relative += static_cast<std::uint32_t>(chunks[i].size());
    }
    out.u32(relative);
    out.u32(relative);

    for (const auto &chunk : chunks) {
        out.bytes(chunk.data(), chunk.size());
    }

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        error = "cannot open " + path.string();
        return false;
    }
    const auto &data = out.data();
    file.write(reinterpret_cast<const char *>(data.data()),
               static_cast<std::streamsize>(data.size()));
    if (!file) {
        error = "cannot write " + path.string();
        return false;
    }
    return true;
}

// ── per-binary build ───────────────────────────────────────────────────────

struct BinSpec {
    BinaryKind kind;
    const char *label;
    const char *file_name;
    BinaryHeaderInfo header;
};

const BinSpec &spec_for(BinaryKind kind) {
    static const BinSpec game{BinaryKind::Game, "game", "game.bin", GAME_BINARY_HEADER};
    static const BinSpec frontend{BinaryKind::Frontend, "frontend", "frontend.bin",
                                   FRONTEND_BINARY_HEADER};
    static const BinSpec script{BinaryKind::Script, "script", "script.bin",
                                SCRIPT_BINARY_HEADER};
    switch (kind) {
    case BinaryKind::Frontend:
        return frontend;
    case BinaryKind::Script:
        return script;
    case BinaryKind::Game:
        break;
    }
    return game;
}

BinBuildStats build_one(const std::vector<ParsedFile> &files, const SymbolTable &symbols,
                        const std::filesystem::path &out_dir, BinaryKind kind,
                        NamesBuilder &names) {
    const BinSpec &spec = spec_for(kind);
    BinBuildStats stats;
    stats.label = spec.label;
    stats.file_name = spec.file_name;

    std::vector<const ParsedFile *> scoped = scope_files(files, kind);
    std::vector<const SourceAst *> asts;
    asts.reserve(scoped.size());

    DefinitionIndex definitions;
    for (const ParsedFile *file : scoped) {
        asts.push_back(&file->ast);
        definitions.add(file->ast);
    }

    BuildIndex index = BuildIndex::build(kind, asts);
    definitions.apply_indices(index.indices());

    std::vector<BuiltEntry> entries;
    entries.reserve(null_entry_count(kind) + index.named_count());
    std::vector<std::vector<SubBlock>> sub_blocks;
    sub_blocks.reserve(index.named_count());
    std::map<std::string, std::uint32_t> class_index;

    // NULLDEF pass. Bodies are built before any NULLDEF name is interned,
    // matching the Rust builder: lower_def interns schema strings (notably the
    // UI font) before the emission pass interns ARMOUR / NULLDEF_ARMOUR.
    const char *const *null_list = null_entries(kind);
    std::vector<BodyOutcome> null_bodies;
    null_bodies.reserve(null_entry_count(kind));
    for (std::size_t i = 0; i < null_entry_count(kind); ++i) {
        null_bodies.push_back(make_default_body(null_list[i], names));
    }

    for (std::size_t i = 0; i < null_entry_count(kind); ++i) {
        const std::string type = null_list[i];
        BodyOutcome &body = null_bodies[i];
        if (!body.found) {
            stats.errors.push_back(body.error);
        }
        stats.unsupported += body.unsupported;
        ++stats.null_defs;

        BuiltEntry entry;
        entry.def_name_off = names.intern(type);
        entry.file_name_off = names.intern("NULLDEF_" + type);
        entry.counter = next_class_index(class_index, type);
        entry.is_real = false;
        entry.is_template = false;
        entry.unknown_0 = 0;
        entry.has_subdefs = has_subdef_table(type);
        entry.body = std::move(body.body);
        entries.push_back(std::move(entry));
    }

    // Named pass.
    for (const std::string &name : index.named_order()) {
        const Definition *definition = definitions.find(name);
        if (definition == nullptr) {
            stats.errors.push_back("definition '" + name + "' not found");
            continue;
        }
        // Rust interns the type and definition name before lowering the body;
        // body interning follows. Preserve that order for names.bin parity.
        const std::uint32_t def_name_off = names.intern(definition->def_type);
        const std::uint32_t file_name_off = names.intern(name);

        BodyOutcome body = lower_named(*definition, definitions, symbols, names);
        if (!body.found) {
            stats.errors.push_back(body.error);
        } else if (!body.error.empty()) {
            stats.errors.push_back(name + ": " + body.error);
        } else {
            ++stats.lowered;
        }
        stats.unsupported += body.unsupported;
        ++stats.named;

        std::vector<SubBlock> blocks;
        if (has_subdef_table(definition->def_type)) {
            const auto chain_result = specialization_chain(definitions, name);
            if (const auto *chain = std::get_if<Chain>(&chain_result)) {
                blocks = collect_sub_blocks(*chain);
            }
        }
        sub_blocks.push_back(std::move(blocks));

        BuiltEntry entry;
        entry.def_name_off = def_name_off;
        entry.file_name_off = file_name_off;
        entry.counter = next_class_index(class_index, definition->def_type);
        entry.is_real = true;
        entry.is_template = false;
        entry.unknown_0 = 1;
        entry.has_subdefs = has_subdef_table(definition->def_type);
        entry.body = std::move(body.body);
        entries.push_back(std::move(entry));
    }

    // Anonymous sub-def region (game.bin only; other binaries have no blocks).
    std::vector<BuiltEntry> sub_entries;
    std::map<std::pair<std::string, std::vector<std::uint8_t>>, std::uint32_t> sub_dedup;
    for (std::size_t oi = 0; oi < index.named_order().size(); ++oi) {
        if (oi >= sub_blocks.size()) {
            break;
        }
        const std::uint32_t owner_index =
            static_cast<std::uint32_t>(null_entry_count(kind) + oi);
        BuiltEntry &owner = entries[null_entry_count(kind) + oi];
        if (!owner.has_subdefs) {
            continue;
        }
        std::vector<SubDefRecord> table;
        for (const SubBlock &block : sub_blocks[oi]) {
            BodyOutcome body = lower_subdef(block, definitions, symbols, names);
            if (!body.found || !body.error.empty()) {
                stats.errors.push_back(index.named_order()[oi] + " <" + block.tag +
                                       ">: " + body.error);
                continue;
            }
            stats.unsupported += body.unsupported;
            const auto key = std::make_pair(block.tag, body.body);
            auto it = sub_dedup.find(key);
            std::uint32_t local_index = 0;
            if (it == sub_dedup.end()) {
                local_index = static_cast<std::uint32_t>(sub_entries.size());
                sub_dedup.emplace(key, local_index);
                BuiltEntry sub;
                sub.def_name_off = names.intern(block.tag);
                sub.file_name_off = 0xFFFFFFFFu;
                sub.counter = next_class_index(class_index, block.tag);
                sub.is_real = true;
                sub.is_template = false;
                sub.unknown_0 = 1;
                sub.has_subdefs = has_subdef_table(block.tag);
                sub.body = body.body;
                sub_entries.push_back(std::move(sub));
            } else {
                local_index = it->second;
            }
            table.push_back(SubDefRecord{block.name_crc, local_index, owner_index});
        }
        owner.subdefs = std::move(table);
    }

    const std::uint32_t sub_base = static_cast<std::uint32_t>(entries.size());
    for (std::size_t i = null_entry_count(kind); i < entries.size(); ++i) {
        for (SubDefRecord &record : entries[i].subdefs) {
            record.def_index += sub_base;
        }
    }
    entries.insert(entries.end(), std::make_move_iterator(sub_entries.begin()),
                   std::make_move_iterator(sub_entries.end()));

    stats.entries = entries.size();
    std::string write_error;
    if (!write_binary(out_dir / spec.file_name, entries, spec.header, write_error)) {
        stats.errors.push_back(write_error);
    }
    return stats;
}


} // namespace

BuildResult build_binaries(const std::vector<ParsedFile> &files,
                           const SymbolTable &symbols,
                           const std::filesystem::path &out_dir,
                           const std::vector<BinaryKind> &kinds) {
    BuildResult result;
    std::error_code ec;
    std::filesystem::create_directories(out_dir, ec);
    if (ec) {
        result.error = "cannot create output directory: " + ec.message();
        return result;
    }

    std::vector<BinaryKind> requested = kinds;
    if (requested.empty()) {
        requested = {BinaryKind::Game, BinaryKind::Frontend, BinaryKind::Script};
    }

    NamesBuilder names;
    for (BinaryKind kind : requested) {
        result.bins.push_back(build_one(files, symbols, out_dir, kind, names));
    }

    const auto names_bytes = names.to_bytes(NAMES_HEADER_BYTES);
    std::ofstream names_file(out_dir / "names.bin", std::ios::binary);
    if (!names_file) {
        result.error = "cannot open names.bin";
        return result;
    }
    names_file.write(reinterpret_cast<const char *>(names_bytes.data()),
                     static_cast<std::streamsize>(names_bytes.size()));
    if (!names_file) {
        result.error = "cannot write names.bin";
        return result;
    }
    result.names = names.size();
    result.ok = true;
    for (const BinBuildStats &bin : result.bins) {
        if (!bin.errors.empty()) {
            result.ok = false;
        }
    }
    return result;
}

} // namespace fable::defs::compiler
