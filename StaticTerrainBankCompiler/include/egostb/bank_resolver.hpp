#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>
#include <vector>
#include <filesystem>
#include <mutex>
#include <shared_mutex>
#include <fstream>

namespace egostb {

struct MeshBounds {
    float sphereCenter[3] = {0.0f, 0.0f, 0.0f};
    float sphereRadius = 0.0f;
    float boxMin[3] = {0.0f, 0.0f, 0.0f};
    float boxMax[3] = {0.0f, 0.0f, 0.0f};
};

struct TextureBankEntry {
    uint32_t id = 0;
    uint32_t offset = 0;
    uint32_t size = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    uint32_t format = 0;
};

struct DecodedTexture {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> rgb; // 3 bytes per pixel: R, G, B
};

class BankResolver {
public:
    static BankResolver& instance();

    bool loadTextureBank(const std::string& path);
    bool loadMeshBank(const std::string& path);
    bool loadRetailHeaders(const std::string& meshHeaderPath, const std::string& texHeaderPath);

    uint32_t getTextureId(const std::string& name) const;
    uint32_t getMeshId(const std::string& name) const;

    const MeshBounds* getMeshBounds(uint32_t id) const;
    const MeshBounds* getMeshBounds(const std::string& name) const;

    bool hasTexture(const std::string& name) const;
    bool hasMesh(const std::string& name) const;

    void setTextureId(const std::string& name, uint32_t id);
    void setMeshId(const std::string& name, uint32_t id);

    bool isTexturesLoaded() const { return !m_textureIds.empty(); }
    bool isMeshesLoaded() const { return !m_meshIds.empty(); }

    size_t textureCount() const { return m_textureIds.size(); }
    size_t meshCount() const { return m_meshIds.size(); }

    const DecodedTexture* getDecodedTexture(uint32_t id) const;
    const DecodedTexture* getDecodedTexture(const std::string& name) const;

    const std::string& getTextureBankPath() const { return m_textureBankPath; }

    void clear();

private:
    std::unordered_map<std::string, uint32_t> m_textureIds;
    std::unordered_map<std::string, uint32_t> m_meshIds;
    std::unordered_map<uint32_t, MeshBounds> m_meshBounds;
    std::unordered_map<std::string, MeshBounds> m_meshBoundsByName;

    std::string m_textureBankPath;
    std::unordered_map<uint32_t, TextureBankEntry> m_textureBankEntries;
    std::unordered_map<std::string, TextureBankEntry> m_textureBankEntriesByName;
    mutable std::shared_mutex m_decodeMutex;
    mutable std::ifstream m_textureBankFile;
    mutable std::unordered_map<uint32_t, DecodedTexture> m_decodedTextures;
};

} // namespace egostb
