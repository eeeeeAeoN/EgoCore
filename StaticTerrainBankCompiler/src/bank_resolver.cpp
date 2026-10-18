#include "egostb/bank_resolver.hpp"
#include "egostb/lzo1x.hpp"
#include <fstream>
#include <iostream>
#include <cstring>
#include <algorithm>

namespace egostb {

BankResolver& BankResolver::instance() {
    static BankResolver s_instance;
    return s_instance;
}


static std::string readBankString(std::ifstream& file) {
    uint32_t len = 0;
    file.read(reinterpret_cast<char*>(&len), 4);
    if (!file || len == 0) return "";
    std::string s(len, '\0');
    file.read(&s[0], len);
    auto nulPos = s.find('\0');
    if (nulPos != std::string::npos) s.erase(nulPos);
    return s;
}

struct BigSubBankInfo {
    std::string name;
    uint32_t version = 0;
    uint32_t entryCount = 0;
    uint32_t offset = 0;
    uint32_t size = 0;
    uint32_t align = 0;
};

static bool parseBigBankEntries(const std::string& path,
                                const std::vector<std::string>& targetSubBanks,
                                std::unordered_map<std::string, uint32_t>& outMap,
                                std::unordered_map<uint32_t, MeshBounds>* outBoundsById = nullptr,
                                std::unordered_map<std::string, MeshBounds>* outBoundsByName = nullptr,
                                std::unordered_map<uint32_t, TextureBankEntry>* outTexById = nullptr,
                                std::unordered_map<std::string, TextureBankEntry>* outTexByName = nullptr) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    char magic[4] = {0};
    file.read(magic, 4);
    if (std::memcmp(magic, "BIGB", 4) != 0) return false;

    uint32_t version = 0, footOff = 0, footSz = 0;
    file.read(reinterpret_cast<char*>(&version), 4);
    file.read(reinterpret_cast<char*>(&footOff), 4);
    file.read(reinterpret_cast<char*>(&footSz), 4);

    file.seekg(footOff, std::ios::beg);
    uint32_t bankCount = 0;
    file.read(reinterpret_cast<char*>(&bankCount), 4);

    std::vector<BigSubBankInfo> subBanks;
    subBanks.reserve(bankCount);
    for (uint32_t i = 0; i < bankCount; ++i) {
        BigSubBankInfo sb;
        std::getline(file, sb.name, '\0');
        file.read(reinterpret_cast<char*>(&sb.version), 4);
        file.read(reinterpret_cast<char*>(&sb.entryCount), 4);
        file.read(reinterpret_cast<char*>(&sb.offset), 4);
        file.read(reinterpret_cast<char*>(&sb.size), 4);
        file.read(reinterpret_cast<char*>(&sb.align), 4);
        subBanks.push_back(sb);
    }

    int targetIndex = -1;
    for (int i = 0; i < static_cast<int>(subBanks.size()); ++i) {
        for (const auto& target : targetSubBanks) {
            if (subBanks[i].name == target) {
                targetIndex = i;
                break;
            }
        }
        if (targetIndex != -1) break;
    }

    if (targetIndex == -1) return false;

    const auto& info = subBanks[targetIndex];
    file.seekg(info.offset, std::ios::beg);

    uint32_t statsCount = 0;
    file.read(reinterpret_cast<char*>(&statsCount), 4);
    if (statsCount < 1000) {
        file.seekg(statsCount * 8, std::ios::cur);
    } else {
        file.seekg(-4, std::ios::cur);
    }

    for (uint32_t i = 0; i < info.entryCount; ++i) {
        uint32_t magicE = 0;
        file.read(reinterpret_cast<char*>(&magicE), 4);
        uint32_t id = 0, type = 0, size = 0, offset = 0, crc = 0;
        file.read(reinterpret_cast<char*>(&id), 4);
        file.read(reinterpret_cast<char*>(&type), 4);
        file.read(reinterpret_cast<char*>(&size), 4);
        file.read(reinterpret_cast<char*>(&offset), 4);
        file.read(reinterpret_cast<char*>(&crc), 4);

        std::string name = readBankString(file);

        uint32_t timestamp = 0, depCount = 0;
        file.read(reinterpret_cast<char*>(&timestamp), 4);
        file.read(reinterpret_cast<char*>(&depCount), 4);
        for (uint32_t d = 0; d < depCount; ++d) {
            readBankString(file);
        }

        uint32_t infoSize = 0;
        file.read(reinterpret_cast<char*>(&infoSize), 4);
        if (outBoundsById && infoSize >= 44) {
            int32_t phys = 0;
            MeshBounds mb;
            file.read(reinterpret_cast<char*>(&phys), 4);
            file.read(reinterpret_cast<char*>(mb.sphereCenter), 12);
            file.read(reinterpret_cast<char*>(&mb.sphereRadius), 4);
            file.read(reinterpret_cast<char*>(mb.boxMin), 12);
            file.read(reinterpret_cast<char*>(mb.boxMax), 12);
            (*outBoundsById)[id] = mb;
            if (outBoundsByName && !name.empty()) {
                (*outBoundsByName)[name] = mb;
            }
            if (infoSize > 44) {
                file.seekg(infoSize - 44, std::ios::cur);
            }
        } else if (outTexById && infoSize >= 34) {
            std::vector<uint8_t> infoData(infoSize);
            file.read(reinterpret_cast<char*>(infoData.data()), infoSize);
            uint16_t w = 0, h = 0;
            uint32_t fmt = 0;
            std::memcpy(&w, infoData.data(), 2);
            std::memcpy(&h, infoData.data() + 2, 2);
            std::memcpy(&fmt, infoData.data() + 12, 4);

            TextureBankEntry te;
            te.id = id;
            te.offset = offset;
            te.size = size;
            te.width = w;
            te.height = h;
            te.format = fmt;

            (*outTexById)[id] = te;
            if (outTexByName && !name.empty()) {
                (*outTexByName)[name] = te;
            }
        } else if (infoSize > 0) {
            file.seekg(infoSize, std::ios::cur);
        }

        if (!name.empty() && (magicE == 42 || (size > 0 && id > 0))) {
            if (outMap.find(name) == outMap.end()) {
                outMap[name] = id;
            }
        }
    }

    return !outMap.empty();
}

bool BankResolver::loadTextureBank(const std::string& path) {
    m_textureBankPath = path;
    if (m_textureBankFile.is_open()) m_textureBankFile.close();
    m_textureBankFile.open(m_textureBankPath, std::ios::binary);
    std::vector<std::string> subBanks = {"GBANK_MAIN_PC", "GBANK_MAIN"};
    bool ok = parseBigBankEntries(path, subBanks, m_textureIds, nullptr, nullptr, &m_textureBankEntries, &m_textureBankEntriesByName);
    if (ok) {
        std::cout << "Loaded " << m_textureIds.size() << " textures (" << m_textureBankEntries.size() << " entries) from: " << path << "\n";
    }
    return ok;
}


bool BankResolver::loadMeshBank(const std::string& path) {
    std::vector<std::string> subBanks = {"MBANK_ALLMESHES"};
    bool ok = parseBigBankEntries(path, subBanks, m_meshIds, &m_meshBounds, &m_meshBoundsByName);
    if (ok) {
        std::cout << "Loaded " << m_meshIds.size() << " meshes (" << m_meshBounds.size() << " bounds) from: " << path << "\n";
    }
    return ok;
}

static bool parseEnumHeader(const std::string& path, std::unordered_map<std::string, uint32_t>& outMap) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string line;
    size_t count = 0;
    while (std::getline(file, line)) {
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        // Extract key
        size_t kStart = line.find_first_not_of(" \t\r\n", 0);
        if (kStart == std::string::npos || kStart >= eqPos) continue;
        size_t kEnd = line.find_last_not_of(" \t\r\n", eqPos - 1);
        if (kEnd == std::string::npos || kEnd < kStart) continue;
        std::string key = line.substr(kStart, kEnd - kStart + 1);

        // Extract value
        size_t vStart = line.find_first_not_of(" \t\r\n", eqPos + 1);
        if (vStart == std::string::npos) continue;
        size_t commaPos = line.find(',', vStart);
        size_t vEnd = (commaPos != std::string::npos) ? commaPos - 1 : line.find_last_not_of(" \t\r\n;");
        if (vEnd < vStart) continue;
        std::string valStr = line.substr(vStart, vEnd - vStart + 1);

        try {
            uint32_t id = static_cast<uint32_t>(std::stoul(valStr));
            if (outMap.find(key) == outMap.end()) {
                outMap[key] = id;
                count++;
            }
        } catch (...) {}
    }
    return count > 0;
}

bool BankResolver::loadRetailHeaders(const std::string& meshHeaderPath, const std::string& texHeaderPath) {
    bool okMesh = parseEnumHeader(meshHeaderPath, m_meshIds);
    if (okMesh) {
        std::cout << "Loaded " << m_meshIds.size() << " meshes from retail header: " << meshHeaderPath << "\n";
    }
    bool okTex = parseEnumHeader(texHeaderPath, m_textureIds);
    if (okTex) {
        std::cout << "Loaded " << m_textureIds.size() << " textures from retail header: " << texHeaderPath << "\n";
    }
    return okMesh || okTex;
}

uint32_t BankResolver::getTextureId(const std::string& name) const {
    auto it = m_textureIds.find(name);
    return it != m_textureIds.end() ? it->second : 0;
}

uint32_t BankResolver::getMeshId(const std::string& name) const {
    auto it = m_meshIds.find(name);
    return it != m_meshIds.end() ? it->second : 0;
}

const MeshBounds* BankResolver::getMeshBounds(uint32_t id) const {
    auto it = m_meshBounds.find(id);
    return it != m_meshBounds.end() ? &it->second : nullptr;
}

const MeshBounds* BankResolver::getMeshBounds(const std::string& name) const {
    auto it = m_meshBoundsByName.find(name);
    return it != m_meshBoundsByName.end() ? &it->second : nullptr;
}

bool BankResolver::hasTexture(const std::string& name) const {
    return m_textureIds.find(name) != m_textureIds.end();
}

bool BankResolver::hasMesh(const std::string& name) const {
    return m_meshIds.find(name) != m_meshIds.end();
}

void BankResolver::setTextureId(const std::string& name, uint32_t id) {
    m_textureIds[name] = id;
}

void BankResolver::setMeshId(const std::string& name, uint32_t id) {
    m_meshIds[name] = id;
}

const DecodedTexture* BankResolver::getDecodedTexture(uint32_t id) const {
    if (id == 0) return nullptr;

    {
        std::shared_lock<std::shared_mutex> readLock(m_decodeMutex);
        auto it = m_decodedTextures.find(id);
        if (it != m_decodedTextures.end()) {
            return &it->second;
        }
    }

    std::unique_lock<std::shared_mutex> writeLock(m_decodeMutex);

    auto it = m_decodedTextures.find(id);
    if (it != m_decodedTextures.end()) {
        return &it->second;
    }

    auto eit = m_textureBankEntries.find(id);
    if (eit == m_textureBankEntries.end()) {
        return nullptr;
    }
    const auto& entry = eit->second;
    if (entry.size == 0) {
        return nullptr;
    }

    if (!m_textureBankFile.is_open()) {
        if (!m_textureBankPath.empty()) {
            m_textureBankFile.open(m_textureBankPath, std::ios::binary);
        }
        if (!m_textureBankFile.is_open()) return nullptr;
    }

    m_textureBankFile.seekg(entry.offset, std::ios::beg);
    std::vector<uint8_t> payload(entry.size);
    m_textureBankFile.read(reinterpret_cast<char*>(payload.data()), entry.size);

    uint16_t w = entry.width > 0 ? entry.width : 512;
    uint16_t h = entry.height > 0 ? entry.height : 512;
    size_t expectedDxtLen = (static_cast<size_t>(w) * h) / 2;
    std::vector<uint8_t> dxtData(expectedDxtLen, 0);

    bool dxtReady = false;

    // Check for LZO compressed header:
    // - If uint16 at offset 0 == 0xFFFF: 6-byte header [0xFFFF][uint32 cLen], data at +6
    // - Otherwise: 2-byte header [uint16 cLen], data at +2
    if (payload.size() >= 2) {
        uint16_t len16 = 0;
        std::memcpy(&len16, payload.data(), 2);
        uint32_t cLen = 0;
        size_t headerSize = 0;
        if (len16 == 0xFFFF && payload.size() >= 6) {
            std::memcpy(&cLen, payload.data() + 2, 4);
            headerSize = 6;
        } else if (len16 > 0 && len16 != 0xFFFF) {
            cLen = len16;
            headerSize = 2;
        }
        if (headerSize > 0 && cLen > 0 && cLen <= payload.size() - headerSize) {
            size_t outLen = expectedDxtLen;
            auto st = albion::lzo1x::decompress(payload.data() + headerSize, cLen, dxtData.data(), &outLen);
            if (st == albion::lzo1x::Status::Ok) {
                dxtReady = true;
            }
        }
    }

    // Fallback: check if payload is raw uncompressed DXT1
    if (!dxtReady && payload.size() >= expectedDxtLen) {
        std::memcpy(dxtData.data(), payload.data(), expectedDxtLen);
        dxtReady = true;
    }

    if (!dxtReady) {
        return nullptr;
    }

    // Decode DXT1 blocks to RGB888
    DecodedTexture dec;
    dec.width = w;
    dec.height = h;
    dec.rgb.resize(static_cast<size_t>(w) * h * 3, 0);

    auto toRGB565_local = [](uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
        r = static_cast<uint8_t>(((c >> 11) & 0x1F) * 255 / 31);
        g = static_cast<uint8_t>(((c >> 5) & 0x3F) * 255 / 63);
        b = static_cast<uint8_t>((c & 0x1F) * 255 / 31);
    };

    int blocksX = (w + 3) / 4;
    int blocksY = (h + 3) / 4;
    size_t blockOffset = 0;

    for (int by = 0; by < blocksY; ++by) {
        for (int bx = 0; bx < blocksX; ++bx) {
            if (blockOffset + 8 > dxtData.size()) break;
            uint16_t c0, c1;
            uint32_t indices;
            std::memcpy(&c0, dxtData.data() + blockOffset, 2);
            std::memcpy(&c1, dxtData.data() + blockOffset + 2, 2);
            std::memcpy(&indices, dxtData.data() + blockOffset + 4, 4);
            blockOffset += 8;

            uint8_t palR[4], palG[4], palB[4];
            toRGB565_local(c0, palR[0], palG[0], palB[0]);
            toRGB565_local(c1, palR[1], palG[1], palB[1]);

            if (c0 > c1) {
                palR[2] = static_cast<uint8_t>((2 * palR[0] + palR[1]) / 3);
                palG[2] = static_cast<uint8_t>((2 * palG[0] + palG[1]) / 3);
                palB[2] = static_cast<uint8_t>((2 * palB[0] + palB[1]) / 3);

                palR[3] = static_cast<uint8_t>((palR[0] + 2 * palR[1]) / 3);
                palG[3] = static_cast<uint8_t>((palG[0] + 2 * palG[1]) / 3);
                palB[3] = static_cast<uint8_t>((palB[0] + 2 * palB[1]) / 3);
            } else {
                palR[2] = static_cast<uint8_t>((palR[0] + palR[1]) / 2);
                palG[2] = static_cast<uint8_t>((palG[0] + palG[1]) / 2);
                palB[2] = static_cast<uint8_t>((palB[0] + palB[1]) / 2);

                palR[3] = 0;
                palG[3] = 0;
                palB[3] = 0;
            }

            for (int py = 0; py < 4; ++py) {
                int y = by * 4 + py;
                if (y >= h) continue;
                for (int px = 0; px < 4; ++px) {
                    int x = bx * 4 + px;
                    if (x >= w) continue;
                    int p = py * 4 + px;
                    int idx = (indices >> (2 * p)) & 3;
                    size_t outPixelIdx = (static_cast<size_t>(y) * w + x) * 3;
                    dec.rgb[outPixelIdx + 0] = palR[idx];
                    dec.rgb[outPixelIdx + 1] = palG[idx];
                    dec.rgb[outPixelIdx + 2] = palB[idx];
                }
            }
        }
    }

    m_decodedTextures[id] = std::move(dec);
    return &m_decodedTextures[id];
}

const DecodedTexture* BankResolver::getDecodedTexture(const std::string& name) const {
    if (name.empty()) return nullptr;
    auto nit = m_textureBankEntriesByName.find(name);
    if (nit != m_textureBankEntriesByName.end()) {
        return getDecodedTexture(nit->second.id);
    }
    uint32_t id = getTextureId(name);
    if (id != 0) {
        return getDecodedTexture(id);
    }
    return nullptr;
}

void BankResolver::clear() {
    std::unique_lock<std::shared_mutex> lock(m_decodeMutex);
    m_textureIds.clear();
    m_meshIds.clear();
    m_meshBounds.clear();
    m_meshBoundsByName.clear();
    m_textureBankPath.clear();
    m_textureBankEntries.clear();
    m_textureBankEntriesByName.clear();
    m_decodedTextures.clear();
    if (m_textureBankFile.is_open()) m_textureBankFile.close();
}


} // namespace egostb
