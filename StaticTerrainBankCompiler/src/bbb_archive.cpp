#include "egostb/bbb_archive.hpp"
#include <fstream>
#include <stdexcept>
#include <cstring>

namespace egostb {

namespace {

uint32_t readU32(std::istream& in) {
    uint32_t v = 0;
    in.read(reinterpret_cast<char*>(&v), sizeof(v));
    if (!in) throw std::runtime_error("Unexpected EOF while reading uint32");
    return v;
}

std::string readString(std::istream& in, uint32_t length) {
    std::string s(length, '\0');
    if (length > 0) {
        in.read(s.data(), length);
        if (!in) throw std::runtime_error("Unexpected EOF while reading string");
    }
    return s;
}

void writeU32(std::vector<uint8_t>& buf, uint32_t v) {
    const size_t offset = buf.size();
    buf.resize(offset + 4);
    std::memcpy(buf.data() + offset, &v, 4);
}

void padTo(std::vector<uint8_t>& buf, size_t align) {
    if (align == 0) return;
    size_t rem = buf.size() % align;
    if (rem != 0) {
        buf.resize(buf.size() + (align - rem), 0);
    }
}

} // namespace

BBBArchive BBBArchive::read(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path.string());
    }

    uint32_t magic = readU32(file);
    if (magic != 0x42424242u) {
        throw std::runtime_error("Invalid BBB magic");
    }

    BBBArchive archive;
    archive.version1 = readU32(file);
    archive.version2 = readU32(file);
    archive.version3 = readU32(file);
    archive.alignment = readU32(file);
    (void)readU32(file); // count1
    uint32_t entryCount = readU32(file);
    uint32_t tableOffset = readU32(file);

    file.seekg(tableOffset, std::ios::beg);
    // Dev header: 12 bytes
    (void)readU32(file);
    (void)readU32(file);
    (void)readU32(file);

    archive.entries.resize(entryCount);
    for (uint32_t i = 0; i < entryCount; ++i) {
        uint32_t entryMagic = readU32(file);
        if (entryMagic != 42) {
            throw std::runtime_error("Invalid entry magic at entry " + std::to_string(i));
        }

        BBBEntry& e = archive.entries[i];
        e.id = readU32(file);
        e.type = readU32(file);
        uint32_t size = readU32(file);
        e.offset = readU32(file);
        e.crc = readU32(file);

        uint32_t nameLen = readU32(file);
        e.name = readString(file, nameLen);

        e.devId = readU32(file);
        e.devType = readU32(file);
        uint32_t devNameLen = readU32(file);
        e.devName = readString(file, devNameLen);

        uint32_t extraSize = readU32(file);
        e.extra.resize(extraSize);
        if (extraSize > 0) {
            file.read(reinterpret_cast<char*>(e.extra.data()), extraSize);
        }

        // Read payload
        if (size > 0) {
            std::streampos saved = file.tellg();
            file.seekg(e.offset, std::ios::beg);
            e.payload.resize(size);
            file.read(reinterpret_cast<char*>(e.payload.data()), size);
            file.seekg(saved, std::ios::beg);
        }
    }

    return archive;
}

std::vector<uint8_t> BBBArchive::serialize() const {
    std::vector<uint8_t> out;
    // Header placeholder (32 bytes)
    out.resize(32, 0);
    padTo(out, alignment);

    std::vector<uint32_t> offsets(entries.size(), alignment);

    // Write payloads
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        if (!e.payload.empty()) {
            padTo(out, alignment);
            offsets[i] = static_cast<uint32_t>(out.size());
            out.insert(out.end(), e.payload.begin(), e.payload.end());
        } else {
            offsets[i] = alignment; // Default offset for 0-byte payload
        }
    }

    // Pad before table
    padTo(out, alignment);
    uint32_t finalTableOffset = static_cast<uint32_t>(out.size());

    // Write 12-byte dev header
    writeU32(out, 1);
    writeU32(out, 0);
    writeU32(out, static_cast<uint32_t>(entries.size()));

    // Write entry table
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        writeU32(out, 42);
        writeU32(out, e.id);
        writeU32(out, e.type);
        writeU32(out, static_cast<uint32_t>(e.payload.size()));
        writeU32(out, offsets[i]);
        writeU32(out, e.crc);
        writeU32(out, static_cast<uint32_t>(e.name.size()));
        out.insert(out.end(), e.name.begin(), e.name.end());

        writeU32(out, e.devId);
        writeU32(out, e.devType);
        writeU32(out, static_cast<uint32_t>(e.devName.size()));
        out.insert(out.end(), e.devName.begin(), e.devName.end());

        writeU32(out, static_cast<uint32_t>(e.extra.size()));
        out.insert(out.end(), e.extra.begin(), e.extra.end());
    }

    // Patch header
    const char magic[4] = {'B', 'B', 'B', 'B'};
    std::memcpy(out.data(), magic, 4);
    std::memcpy(out.data() + 4, &version1, 4);
    std::memcpy(out.data() + 8, &version2, 4);
    std::memcpy(out.data() + 12, &version3, 4);
    std::memcpy(out.data() + 16, &alignment, 4);
    uint32_t count = static_cast<uint32_t>(entries.size());
    std::memcpy(out.data() + 20, &count, 4);
    std::memcpy(out.data() + 24, &count, 4);
    std::memcpy(out.data() + 28, &finalTableOffset, 4);

    return out;
}

void BBBArchive::write(const std::filesystem::path& path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot write file: " + path.string());
    }

    std::vector<uint32_t> offsets(entries.size(), alignment);
    size_t currentPos = 32;
    if (alignment > 0) {
        size_t rem = currentPos % alignment;
        if (rem != 0) currentPos += (alignment - rem);
    }

    for (size_t i = 0; i < entries.size(); ++i) {
        if (!entries[i].payload.empty()) {
            if (alignment > 0) {
                size_t rem = currentPos % alignment;
                if (rem != 0) currentPos += (alignment - rem);
            }
            offsets[i] = static_cast<uint32_t>(currentPos);
            currentPos += entries[i].payload.size();
        } else {
            offsets[i] = alignment;
        }
    }

    if (alignment > 0) {
        size_t rem = currentPos % alignment;
        if (rem != 0) currentPos += (alignment - rem);
    }
    uint32_t finalTableOffset = static_cast<uint32_t>(currentPos);

    // Write 32-byte header
    const char magic[4] = {'B', 'B', 'B', 'B'};
    file.write(magic, 4);
    file.write(reinterpret_cast<const char*>(&version1), 4);
    file.write(reinterpret_cast<const char*>(&version2), 4);
    file.write(reinterpret_cast<const char*>(&version3), 4);
    file.write(reinterpret_cast<const char*>(&alignment), 4);
    uint32_t count = static_cast<uint32_t>(entries.size());
    file.write(reinterpret_cast<const char*>(&count), 4);
    file.write(reinterpret_cast<const char*>(&count), 4);
    file.write(reinterpret_cast<const char*>(&finalTableOffset), 4);

    auto writePad = [&](size_t targetPos) {
        size_t cur = static_cast<size_t>(file.tellp());
        if (targetPos > cur) {
            static const char zeros[4096] = {0};
            size_t diff = targetPos - cur;
            while (diff > 0) {
                size_t chunk = std::min(diff, sizeof(zeros));
                file.write(zeros, chunk);
                diff -= chunk;
            }
        }
    };

    // Write payloads
    for (size_t i = 0; i < entries.size(); ++i) {
        if (!entries[i].payload.empty()) {
            writePad(offsets[i]);
            file.write(reinterpret_cast<const char*>(entries[i].payload.data()), entries[i].payload.size());
        }
    }

    writePad(finalTableOffset);

    // Write 12-byte dev header
    uint32_t dev1 = 1, dev2 = 0, dev3 = count;
    file.write(reinterpret_cast<const char*>(&dev1), 4);
    file.write(reinterpret_cast<const char*>(&dev2), 4);
    file.write(reinterpret_cast<const char*>(&dev3), 4);

    // Write entry table
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        uint32_t entryMagic = 42;
        file.write(reinterpret_cast<const char*>(&entryMagic), 4);
        file.write(reinterpret_cast<const char*>(&e.id), 4);
        file.write(reinterpret_cast<const char*>(&e.type), 4);
        uint32_t pSize = static_cast<uint32_t>(e.payload.size());
        file.write(reinterpret_cast<const char*>(&pSize), 4);
        uint32_t off = offsets[i];
        file.write(reinterpret_cast<const char*>(&off), 4);
        file.write(reinterpret_cast<const char*>(&e.crc), 4);

        uint32_t nLen = static_cast<uint32_t>(e.name.size());
        file.write(reinterpret_cast<const char*>(&nLen), 4);
        if (nLen > 0) file.write(e.name.data(), nLen);

        file.write(reinterpret_cast<const char*>(&e.devId), 4);
        file.write(reinterpret_cast<const char*>(&e.devType), 4);
        uint32_t dnLen = static_cast<uint32_t>(e.devName.size());
        file.write(reinterpret_cast<const char*>(&dnLen), 4);
        if (dnLen > 0) file.write(e.devName.data(), dnLen);

        uint32_t exLen = static_cast<uint32_t>(e.extra.size());
        file.write(reinterpret_cast<const char*>(&exLen), 4);
        if (exLen > 0) file.write(reinterpret_cast<const char*>(e.extra.data()), exLen);
    }
}


} // namespace egostb
