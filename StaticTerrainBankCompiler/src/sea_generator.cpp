#include "egostb/sea_generator.hpp"
#include "egostb/stripifier.hpp"
#include "egostb/range_compressor.hpp"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <map>
#include <future>
#include <thread>
#include <cstring>
#include <iostream>

namespace egostb {

namespace {

// CRC32 table with polynomial 0xEDB88320 and initial value 0
uint32_t calcSeaCRC(const void* data, size_t size) {
    static uint32_t crcTable[256];
    static bool initialized = false;
    if (!initialized) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int j = 0; j < 8; ++j) {
                if (c & 1) {
                    c = 0xEDB88320u ^ (c >> 1);
                } else {
                    c = c >> 1;
                }
            }
            crcTable[i] = c;
        }
        initialized = true;
    }

    uint32_t crc = 0;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) {
        crc = crcTable[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc;
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

struct CVertexDef {
    int32_t x = 0;
    int32_t y = 0;
    float z = 0.0f;
};

struct CSeaQuadElement {
    int32_t startX = 0;
    int32_t startY = 0;
    int32_t endX = 0;
    int32_t endY = 0;
    int32_t landscapeLipDirection = 0; // 0=NULL, 1=LEFT, 2=RIGHT, 3=TOP, 4=BOTTOM
    std::vector<int32_t> vertexListLeft;
    std::vector<int32_t> vertexListRight;
    std::vector<int32_t> vertexListTop;
    std::vector<int32_t> vertexListBottom;
    int32_t middleVertex = 0;

    CSeaQuadElement() = default;
    CSeaQuadElement(int32_t sx, int32_t sy, int32_t ex, int32_t ey, int32_t lip = 0)
        : startX(sx), startY(sy), endX(ex), endY(ey), landscapeLipDirection(lip) {}
};

float calcSeaHeight(int xp, int yp, int worldCenterX, int worldCenterY, const SeaDefConfig& cfg) {
    float dx = static_cast<float>(xp - worldCenterX);
    float dy = static_cast<float>(yp - worldCenterY);
    float fxp = dx * dx + dy * dy;
    if (fxp < cfg.seaFlatSectionStart * cfg.seaFlatSectionStart) {
        return cfg.seaHeight;
    }
    float global_radius = static_cast<float>(static_cast<double>(cfg.seaRadius) * 1.414213538169861);
    float r_sq = global_radius * global_radius;
    float h_sq = r_sq - fxp;
    if (h_sq >= 0.0f) {
        if (h_sq > r_sq) h_sq = r_sq;
    } else {
        h_sq = 0.0f;
    }
    float heighta = std::sqrt(h_sq) - global_radius;
    if (fxp <= cfg.seaFlatSectionEnd * cfg.seaFlatSectionEnd) {
        float dist = std::sqrt(fxp);
        return (dist - cfg.seaFlatSectionStart) * heighta / (cfg.seaFlatSectionEnd - cfg.seaFlatSectionStart) + cfg.seaHeight;
    }
    return cfg.seaHeight + heighta;
}

float findElementWorldZDistance(int worldCenterX, int worldCenterY, const CSeaQuadElement& elem) {
    float cx = static_cast<float>(worldCenterX);
    float cy = static_cast<float>(worldCenterY);
    float px = (elem.startX <= cx) ? ((cx <= elem.endX) ? cx : static_cast<float>(elem.endX)) : static_cast<float>(elem.startX);
    float py = (elem.startY <= cy) ? ((cy <= elem.endY) ? cy : static_cast<float>(elem.endY)) : static_cast<float>(elem.startY);
    float dx = px - cx;
    float dy = py - cy;
    return std::sqrt(dx * dx + dy * dy);
}

bool doCurveSplitTest(float min_2d_dist, float avg_z, float real_z, int worldRadius) {
    float dista = min_2d_dist * min_2d_dist + real_z * real_z;
    float dist = std::sqrt(dista) - static_cast<float>(worldRadius);
    if (dist < 1.0f) dist = 1.0f;
    float diff = std::abs(avg_z - real_z);
    return (diff * 800.0f / dist) > 2.0f;
}

bool doAreaTest(int sx, int sy, int ex, int ey, int worldCenterX, int worldCenterY, const SeaDefConfig& cfg) {
    int midY = (sy + ey) / 2;
    float h_left = calcSeaHeight(sx, midY, worldCenterX, worldCenterY, cfg);
    float h_right = calcSeaHeight(ex, midY, worldCenterX, worldCenterY, cfg);
    int midX = (sx + ex) / 2;
    float h_top = calcSeaHeight(midX, sy, worldCenterX, worldCenterY, cfg);
    float h_bottom = calcSeaHeight(midX, ey, worldCenterX, worldCenterY, cfg);
    int area = std::abs(sx - ex) * std::abs(sy - ey);
    return ((h_left - h_right) < 1.0f && (h_top - h_bottom) < 1.0f && area > 65536);
}

bool mapHasDataAtPoint(int xp, int yp, const std::vector<SeaMapInfo>& allWorldMaps) {
    for (const auto& m : allWorldMaps) {
        if (xp >= m.tlx && xp < m.brx && yp >= m.tly && yp < m.bry) {
            return true;
        }
    }
    return false;
}

bool peekHasWaterAtPoint(int xp, int yp, const std::vector<SeaMapInfo>& allWorldMaps) {
    for (const auto& m : allWorldMaps) {
        if (xp >= m.tlx && xp <= m.brx && yp >= m.tly && yp <= m.bry) {
            int lx = xp - m.tlx;
            int ly = yp - m.tly;
            if (lx >= 0 && lx <= m.width && ly >= 0 && ly <= m.height) {
                size_t idx = static_cast<size_t>(ly) * (m.width + 1) + lx;
                if (idx < m.waterGrid.size() && m.waterGrid[idx] != 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

class VertexManager {
public:
    std::vector<CVertexDef> vertDefs;
    std::unordered_map<uint32_t, int32_t> vertexMap;
    int worldCenterX = 0;
    int worldCenterY = 0;
    SeaDefConfig cfg;

    VertexManager(int cx, int cy, const SeaDefConfig& c)
        : worldCenterX(cx), worldCenterY(cy), cfg(c) {}

    int32_t getVertexDef(int xp, int yp) {
        uint32_t key = ((static_cast<uint32_t>(xp + 0x8000) & 0xFFFFu)) |
                       ((static_cast<uint32_t>(yp + 0x8000) & 0xFFFFu) << 16);
        auto it = vertexMap.find(key);
        if (it != vertexMap.end()) {
            return it->second;
        }
        int32_t idx = static_cast<int32_t>(vertDefs.size());
        CVertexDef def;
        def.x = xp;
        def.y = yp;
        def.z = calcSeaHeight(xp, yp, worldCenterX, worldCenterY, cfg);
        vertDefs.push_back(def);
        vertexMap[key] = idx;
        return idx;
    }
};

void mergeElementEdgeVert(std::vector<int32_t>& edge, const std::vector<int32_t>& mergeEdge, const std::vector<CVertexDef>& vertDefs) {
    if (mergeEdge.empty() || edge.size() < 2) return;
    int v5 = 0;
    for (size_t i = 0; i < mergeEdge.size(); ++i) {
        int v7 = mergeEdge[i];
        int yPos = vertDefs[v7].y;
        while (v5 > 0 && vertDefs[edge[v5]].y > yPos) {
            --v5;
        }
        while (v5 < static_cast<int>(edge.size()) - 2 && vertDefs[edge[v5 + 1]].y < yPos) {
            ++v5;
        }
        if (yPos > vertDefs[edge[v5]].y && yPos < vertDefs[edge[v5 + 1]].y) {
            edge.insert(edge.begin() + v5 + 1, v7);
        }
    }
}

void mergeElementEdgeHor(std::vector<int32_t>& edge, const std::vector<int32_t>& mergeEdge, const std::vector<CVertexDef>& vertDefs) {
    if (mergeEdge.empty() || edge.size() < 2) return;
    int v5 = 0;
    for (size_t i = 0; i < mergeEdge.size(); ++i) {
        int v7 = mergeEdge[i];
        int xPos = vertDefs[v7].x;
        while (v5 > 0 && vertDefs[edge[v5]].x > xPos) {
            --v5;
        }
        while (v5 < static_cast<int>(edge.size()) - 2 && vertDefs[edge[v5 + 1]].x < xPos) {
            ++v5;
        }
        if (xPos > vertDefs[edge[v5]].x && xPos < vertDefs[edge[v5 + 1]].x) {
            edge.insert(edge.begin() + v5 + 1, v7);
        }
    }
}

void fillElementEdgeVert(std::vector<int32_t>& edge, int startY, int endY, VertexManager& vtxMgr) {
    if (edge.size() < 2 || startY > endY) return;
    int v6 = 0;
    for (int y = startY; y <= endY; ++y) {
        while (v6 > 0 && vtxMgr.vertDefs[edge[v6]].y > y) {
            --v6;
        }
        while (v6 < static_cast<int>(edge.size()) - 2 && vtxMgr.vertDefs[edge[v6 + 1]].y < y) {
            ++v6;
        }
        if (y > vtxMgr.vertDefs[edge[v6]].y && y < vtxMgr.vertDefs[edge[v6 + 1]].y) {
            int32_t newVtx = vtxMgr.getVertexDef(vtxMgr.vertDefs[edge[v6]].x, y);
            edge.insert(edge.begin() + v6 + 1, newVtx);
        }
    }
}

void fillElementEdgeHor(std::vector<int32_t>& edge, int startX, int endX, VertexManager& vtxMgr) {
    if (edge.size() < 2 || startX > endX) return;
    int v6 = 0;
    for (int x = startX; x <= endX; ++x) {
        while (v6 > 0 && vtxMgr.vertDefs[edge[v6]].x > x) {
            --v6;
        }
        while (v6 < static_cast<int>(edge.size()) - 2 && vtxMgr.vertDefs[edge[v6 + 1]].x < x) {
            ++v6;
        }
        if (x > vtxMgr.vertDefs[edge[v6]].x && x < vtxMgr.vertDefs[edge[v6 + 1]].x) {
            int32_t newVtx = vtxMgr.getVertexDef(x, vtxMgr.vertDefs[edge[v6]].y);
            edge.insert(edge.begin() + v6 + 1, newVtx);
        }
    }
}

} // namespace

SeaDefConfig SeaGenerator::parseDefs(const std::filesystem::path& defsDir) {
    SeaDefConfig cfg;
    std::filesystem::path engineDefPath = defsDir / "engine.def";
    if (std::filesystem::exists(engineDefPath)) {
        std::ifstream file(engineDefPath);
        std::string line;
        while (std::getline(file, line)) {
            std::string t = trim(line);
            if (t.rfind("SeaHeight", 0) == 0) {
                std::istringstream iss(t.substr(9));
                float val = 0.0f;
                if (iss >> val) cfg.seaHeight = val;
            }
        }
    }

    std::filesystem::path envDefPath = defsDir / "environment.def";
    if (std::filesystem::exists(envDefPath)) {
        std::ifstream file(envDefPath);
        std::string line;
        while (std::getline(file, line)) {
            std::string t = trim(line);
            if (t.rfind("SeaRadius", 0) == 0) {
                std::istringstream iss(t.substr(9));
                float val = 0.0f;
                if (iss >> val) cfg.seaRadius = val;
            } else if (t.rfind("SeaFlatSectionStart", 0) == 0) {
                std::istringstream iss(t.substr(19));
                float val = 0.0f;
                if (iss >> val) cfg.seaFlatSectionStart = val;
            } else if (t.rfind("SeaFlatSectionEnd", 0) == 0) {
                std::istringstream iss(t.substr(17));
                float val = 0.0f;
                if (iss >> val) cfg.seaFlatSectionEnd = val;
            }
        }
    }

    return cfg;
}

std::vector<std::string> SeaGenerator::parseHasSeaRegionDefs(const std::filesystem::path& defsDir) {
    std::vector<std::string> hasSeaDefs;
    std::filesystem::path regionDefPath = defsDir / "region.def";
    if (!std::filesystem::exists(regionDefPath)) {
        regionDefPath = defsDir / "regions.def";
    }
    if (!std::filesystem::exists(regionDefPath)) {
        return hasSeaDefs;
    }

    std::ifstream file(regionDefPath);
    std::string line;
    std::string curDef;
    bool hasSea = false;

    while (std::getline(file, line)) {
        std::string t = trim(line);
        if (t.rfind("#definition", 0) == 0) {
            std::istringstream iss(t);
            std::string tag, type, name;
            iss >> tag >> type >> name;
            if (type == "REGION") {
                curDef = name;
                hasSea = false;
            }
        } else if (t.rfind("HasSea", 0) == 0) {
            if (t.find("TRUE") != std::string::npos) {
                hasSea = true;
            }
        } else if (t.rfind("#end_definition", 0) == 0) {
            if (!curDef.empty() && hasSea) {
                hasSeaDefs.push_back(curDef);
            }
            curDef.clear();
            hasSea = false;
        }
    }

    return hasSeaDefs;
}

BBBEntry SeaGenerator::generateSeaEntry(
    uint32_t entryId,
    const SeaRegionJob& job,
    const SeaDefConfig& defConfig,
    const std::vector<SeaMapInfo>& allWorldMaps
) {
    BBBEntry entry;
    entry.id = entryId;
    entry.type = 0;
    entry.devType = 1;
    entry.name = "__ENGINE_SEA_STATIC_MAP_BANK_FILE__" + job.regionName;
    entry.devName = entry.name;

    // 1. Compute cutout boxes & extents
    std::vector<int32_t> rawCutoutBoxes;
    int minTLX = 0x7FFFFFFF, minTLY = 0x7FFFFFFF;
    int maxBRX = -0x7FFFFFFF, maxBRY = -0x7FFFFFFF;

    for (const auto& m : job.cutoutMaps) {
        rawCutoutBoxes.push_back(m.tlx);
        rawCutoutBoxes.push_back(m.tly);
        rawCutoutBoxes.push_back(m.brx);
        rawCutoutBoxes.push_back(m.bry);

        if (m.tlx < minTLX) minTLX = m.tlx;
        if (m.tly < minTLY) minTLY = m.tly;
        if (m.brx > maxBRX) maxBRX = m.brx;
        if (m.bry > maxBRY) maxBRY = m.bry;
    }

    int worldCenterX = (minTLX + maxBRX) / 2;
    // Note: in ego_r.exe (CWaterSeaGenerator::CalcMapExtents at 0x131b520), result->BRY is assigned max(BRX)
    // rather than max(BRY), so WorldCenterY is calculated as (minTLY + maxBRX) / 2.
    int worldCenterY = (minTLY + maxBRX) / 2;

    int worldRadius = 0;
    for (size_t b = 0; b < job.cutoutMaps.size(); ++b) {
        const auto& m = job.cutoutMaps[b];
        int corners[4][2] = {
            {m.tlx, m.tly},
            {m.brx, m.tly},
            {m.tlx, m.bry},
            {m.brx, m.bry}
        };
        for (int c = 0; c < 4; ++c) {
            float dx = static_cast<float>(corners[c][0] - worldCenterX);
            float dy = static_cast<float>(corners[c][1] - worldCenterY);
            int d = static_cast<int>(std::sqrt(dx * dx + dy * dy));
            if (d > worldRadius) {
                worldRadius = d;
            }
        }
    }

    uint32_t cutoutBoxesCRC = calcSeaCRC(rawCutoutBoxes.data(), rawCutoutBoxes.size() * sizeof(int32_t));

    // 2. Initial 4 quads
    int quadSize = static_cast<int>(static_cast<double>(defConfig.seaRadius) / 1.414213538169861);
    std::vector<CSeaQuadElement> elements;
    elements.reserve(4096);

    for (int row = 0; row < 2; ++row) {
        int y0 = worldCenterY + (-quadSize + row * quadSize);
        int y1 = worldCenterY + (row * quadSize);
        for (int col = 0; col < 2; ++col) {
            int x0 = worldCenterX + (-quadSize + col * quadSize);
            int x1 = worldCenterX + (col * quadSize);
            elements.emplace_back(x0, y0, x1, y1, 0);
        }
    }

    // 3. Subdivide to curve and size
    std::function<void(size_t)> splitToCurve = [&](size_t idx) {
        while (true) {
            CSeaQuadElement* elem = &elements[idx];
            float topZ = calcSeaHeight((elem->startX + elem->endX) / 2, elem->startY, worldCenterX, worldCenterY, defConfig);
            float botZ = calcSeaHeight((elem->startX + elem->endX) / 2, elem->endY, worldCenterX, worldCenterY, defConfig);
            float edist = findElementWorldZDistance(worldCenterX, worldCenterY, *elem);

            bool splitLeft = false, splitRight = false, splitTop = false, splitBottom = false;
            if (elem->endX - elem->startX >= 4) {
                int midY = (elem->startY + elem->endY) / 2;
                float realZ = calcSeaHeight(elem->startX, midY, worldCenterX, worldCenterY, defConfig);
                float avgZ = (calcSeaHeight(elem->startX, elem->startY, worldCenterX, worldCenterY, defConfig) +
                              calcSeaHeight(elem->startX, elem->endY, worldCenterX, worldCenterY, defConfig)) * 0.5f;
                splitLeft = doCurveSplitTest(edist, avgZ, realZ, worldRadius);

                float realZ2 = calcSeaHeight(elem->endX, midY, worldCenterX, worldCenterY, defConfig);
                float avgZ2 = (calcSeaHeight(elem->endX, elem->startY, worldCenterX, worldCenterY, defConfig) +
                               calcSeaHeight(elem->endX, elem->endY, worldCenterX, worldCenterY, defConfig)) * 0.5f;
                splitRight = doCurveSplitTest(edist, avgZ2, realZ2, worldRadius);
            }
            if (elem->endY - elem->startY >= 4) {
                float avgTop = (calcSeaHeight(elem->startX, elem->startY, worldCenterX, worldCenterY, defConfig) +
                                calcSeaHeight(elem->endX, elem->startY, worldCenterX, worldCenterY, defConfig)) * 0.5f;
                splitTop = doCurveSplitTest(edist, avgTop, topZ, worldRadius);

                float avgBot = (calcSeaHeight(elem->startX, elem->endY, worldCenterX, worldCenterY, defConfig) +
                                calcSeaHeight(elem->endX, elem->endY, worldCenterX, worldCenterY, defConfig)) * 0.5f;
                splitBottom = doCurveSplitTest(edist, avgBot, botZ, worldRadius);
            }

            bool horiz = splitLeft || splitRight;
            bool vert = splitTop || splitBottom;
            if ((!horiz || !vert) && !doAreaTest(elem->startX, elem->startY, elem->endX, elem->endY, worldCenterX, worldCenterY, defConfig)) {
                break;
            }

            int midX = (elem->startX + elem->endX) / 2;
            int midY = (elem->startY + elem->endY) / 2;

            CSeaQuadElement c1(midX, elem->startY, elem->endX, midY, 0);
            CSeaQuadElement c2(elem->startX, midY, midX, elem->endY, 0);
            CSeaQuadElement c3(midX, midY, elem->endX, elem->endY, 0);

            size_t idx0 = elements.size();
            elements.push_back(c1);
            elements.push_back(c2);
            elements.push_back(c3);

            elem = &elements[idx];
            elem->endX = midX;
            elem->endY = midY;

            splitToCurve(idx);
            splitToCurve(idx0);
            splitToCurve(idx0 + 1);
            idx = idx0 + 2;
        }
    };

    size_t numInitial = elements.size();
    for (size_t i = 0; i < numInitial; ++i) {
        splitToCurve(i);
    }

    // 4. Split elements intersecting cutout boxes (matching 0x131de70)
    std::function<void(int, int, int, int, int)> splitWithArea = [&](int element_id, int startx, int starty, int endx, int endy) {
        if (startx >= elements[element_id].endX || starty >= elements[element_id].endY ||
            endx <= elements[element_id].startX || endy <= elements[element_id].startY) {
            return;
        }

        if (startx > elements[element_id].startX && startx < elements[element_id].endX) {
            elements.emplace_back(startx, elements[element_id].startY,
                                  elements[element_id].endX, elements[element_id].endY, 0);
            elements[element_id].endX = startx;
            splitWithArea(element_id, startx, starty, endx, endy);
            splitWithArea(static_cast<int>(elements.size()) - 1, startx, starty, endx, endy);
            return;
        }

        if (endx > elements[element_id].startX && endx < elements[element_id].endX) {
            elements.emplace_back(endx, elements[element_id].startY,
                                  elements[element_id].endX, elements[element_id].endY, 0);
            elements[element_id].endX = endx;
            splitWithArea(element_id, startx, starty, endx, endy);
            splitWithArea(static_cast<int>(elements.size()) - 1, startx, starty, endx, endy);
            return;
        }

        if (starty > elements[element_id].startY && starty < elements[element_id].endY) {
            elements.emplace_back(elements[element_id].startX, starty,
                                  elements[element_id].endX, elements[element_id].endY, 0);
            elements[element_id].endY = starty;
            splitWithArea(element_id, startx, starty, endx, endy);
            splitWithArea(static_cast<int>(elements.size()) - 1, startx, starty, endx, endy);
            return;
        }

        if (endy > elements[element_id].startY && endy < elements[element_id].endY) {
            elements.emplace_back(elements[element_id].startX, endy,
                                  elements[element_id].endX, elements[element_id].endY, 0);
            elements[element_id].endY = endy;
            splitWithArea(element_id, startx, starty, endx, endy);
            splitWithArea(static_cast<int>(elements.size()) - 1, startx, starty, endx, endy);
        }
    };

    for (const auto& box : job.cutoutMaps) {
        int n = static_cast<int>(elements.size());
        for (int k = 0; k < n; ++k) {
            splitWithArea(k, box.tlx, box.tly, box.brx, box.bry);
        }
    }

    // 5. Erase elements completely inside cutout boxes
    for (const auto& box : job.cutoutMaps) {
        std::vector<CSeaQuadElement> kept;
        kept.reserve(elements.size());
        for (const auto& e : elements) {
            if (e.startX >= box.tlx && e.endX <= box.brx && e.startY >= box.tly && e.endY <= box.bry) {
                continue;
            }
            kept.push_back(e);
        }
        elements = std::move(kept);
    }

    // 6. Add landscape linking lips
    for (const auto& box : job.cutoutMaps) {
        // Bottom edge (y = tly, check y - 1)
        int minX = 0x7FFFFFFF, maxX = -0x7FFFFFFF;
        for (int x = box.tlx; x <= box.brx; ++x) {
            if (!mapHasDataAtPoint(x, box.tly - 1, allWorldMaps) && peekHasWaterAtPoint(x, box.tly, allWorldMaps)) {
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
            }
        }
        if (minX != 0x7FFFFFFF && maxX > minX + 1) {
            elements.emplace_back(minX, box.tly, maxX, box.tly + 2, 4); // LLIP_DIR_BOTTOM = 4
        }

        // Top edge (y = bry, check y + 1)
        minX = 0x7FFFFFFF; maxX = -0x7FFFFFFF;
        for (int x = box.tlx; x <= box.brx; ++x) {
            if (!mapHasDataAtPoint(x, box.bry + 1, allWorldMaps) && peekHasWaterAtPoint(x, box.bry, allWorldMaps)) {
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
            }
        }
        if (minX != 0x7FFFFFFF && maxX > minX + 1) {
            elements.emplace_back(minX, box.bry - 2, maxX, box.bry, 3); // LLIP_DIR_TOP = 3
        }

        // Right edge (x = tlx, check x - 1)
        int minY = 0x7FFFFFFF, maxY = -0x7FFFFFFF;
        for (int y = box.tly; y <= box.bry; ++y) {
            if (!mapHasDataAtPoint(box.tlx - 1, y, allWorldMaps) && peekHasWaterAtPoint(box.tlx, y, allWorldMaps)) {
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
        if (minY != 0x7FFFFFFF && maxY > minY + 1) {
            elements.emplace_back(box.tlx, minY, box.tlx + 2, maxY, 2); // LLIP_DIR_RIGHT = 2
        }

        // Left edge (x = brx, check x + 1)
        minY = 0x7FFFFFFF; maxY = -0x7FFFFFFF;
        for (int y = box.tly; y <= box.bry; ++y) {
            if (!mapHasDataAtPoint(box.brx + 1, y, allWorldMaps) && peekHasWaterAtPoint(box.brx, y, allWorldMaps)) {
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
        if (minY != 0x7FFFFFFF && maxY > minY + 1) {
            elements.emplace_back(box.brx - 2, minY, box.brx, maxY, 1); // LLIP_DIR_LEFT = 1
        }
    }

    // 7. Build mesh definitions
    VertexManager vtxMgr(worldCenterX, worldCenterY, defConfig);

    for (auto& elem : elements) {
        int pos00x = elem.startX;
        int pos00y = elem.startY;
        int endX = elem.endX;
        int pos10y = elem.startY;
        int se_startx = elem.endY;
        int pos11y = elem.endY;
        int pos11x = elem.endX;
        int mp = elem.startX;

        if (elem.landscapeLipDirection == 1) { // LLIP_DIR_LEFT
            if (!mapHasDataAtPoint(pos00x, elem.startY - 1, allWorldMaps)) pos00y += 2;
            if (!mapHasDataAtPoint(pos00x, se_startx + 1, allWorldMaps)) se_startx -= 2;
        }
        if (elem.landscapeLipDirection == 2) { // LLIP_DIR_RIGHT
            if (!mapHasDataAtPoint(endX, pos10y - 1, allWorldMaps)) pos10y += 2;
            if (!mapHasDataAtPoint(endX, pos11y + 1, allWorldMaps)) pos11y -= 2;
        }
        if (elem.landscapeLipDirection == 3) { // LLIP_DIR_TOP
            if (!mapHasDataAtPoint(pos00x - 1, pos00y, allWorldMaps)) pos00x += 2;
            if (!mapHasDataAtPoint(endX + 1, pos00y, allWorldMaps)) endX -= 2;
        }
        if (elem.landscapeLipDirection == 4) { // LLIP_DIR_BOTTOM
            if (!mapHasDataAtPoint(mp - 1, se_startx, allWorldMaps)) mp += 2;
            if (!mapHasDataAtPoint(pos11x + 1, se_startx, allWorldMaps)) pos11x -= 2;
        }

        int32_t vTL = vtxMgr.getVertexDef(pos00x, pos00y);
        int32_t vTR = vtxMgr.getVertexDef(endX, pos10y);
        int32_t vBR = vtxMgr.getVertexDef(pos11x, pos11y);
        int32_t vBL = vtxMgr.getVertexDef(mp, se_startx);

        if (elem.landscapeLipDirection == 1) { // LLIP_DIR_LEFT
            vtxMgr.vertDefs[vTL].z -= 1.0f;
            vtxMgr.vertDefs[vBL].z -= 1.0f;
        }
        if (elem.landscapeLipDirection == 2) { // LLIP_DIR_RIGHT
            vtxMgr.vertDefs[vTR].z -= 1.0f;
            vtxMgr.vertDefs[vBR].z -= 1.0f;
        }
        if (elem.landscapeLipDirection == 3) { // LLIP_DIR_TOP
            vtxMgr.vertDefs[vTL].z -= 1.0f;
            vtxMgr.vertDefs[vTR].z -= 1.0f;
        }
        if (elem.landscapeLipDirection == 4) { // LLIP_DIR_BOTTOM
            vtxMgr.vertDefs[vBL].z -= 1.0f;
            vtxMgr.vertDefs[vBR].z -= 1.0f;
        }

        elem.vertexListLeft = { vTL, vBL };
        elem.vertexListRight = { vTR, vBR };
        elem.vertexListTop = { vTL, vTR };
        elem.vertexListBottom = { vBL, vBR };

        int midX = (elem.startX + elem.endX) / 2;
        int midY = (elem.startY + elem.endY) / 2;
        int32_t midVtx = vtxMgr.getVertexDef(midX, midY);
        elem.middleVertex = midVtx;
        if (elem.landscapeLipDirection != 0) {
            vtxMgr.vertDefs[midVtx].z -= 0.5f;
        }
    }

    // Merge edges between adjacent elements
    for (size_t i = 0; i < elements.size(); ++i) {
        for (size_t j = 0; j < elements.size(); ++j) {
            if (i == j) continue;
            auto& elemA = elements[i];
            const auto& elemB = elements[j];

            if (elemA.startX == elemB.endX && elemA.startY <= elemB.endY && elemB.startY <= elemA.endY) {
                if ((elemA.landscapeLipDirection == 2 || elemA.landscapeLipDirection == 0) &&
                    elemB.landscapeLipDirection <= 1) {
                    mergeElementEdgeVert(elemA.vertexListLeft, elemB.vertexListRight, vtxMgr.vertDefs);
                }
            }
            if (elemA.endX == elemB.startX && elemA.startY <= elemB.endY && elemB.startY <= elemA.endY) {
                if (elemA.landscapeLipDirection <= 1 &&
                    (elemB.landscapeLipDirection == 2 || elemB.landscapeLipDirection == 0)) {
                    mergeElementEdgeVert(elemA.vertexListRight, elemB.vertexListLeft, vtxMgr.vertDefs);
                }
            }
            if (elemA.startY == elemB.endY && elemA.startX <= elemB.endX && elemB.startX <= elemA.endX) {
                if ((elemA.landscapeLipDirection == 4 || elemA.landscapeLipDirection == 0) &&
                    elemB.landscapeLipDirection <= 3 && elemB.landscapeLipDirection != 1 && elemB.landscapeLipDirection != 2) {
                    mergeElementEdgeHor(elemA.vertexListTop, elemB.vertexListBottom, vtxMgr.vertDefs);
                }
            }
            if (elemA.endY == elemB.startY && elemA.startX <= elemB.endX && elemB.startX <= elemA.endX) {
                if (elemA.landscapeLipDirection <= 3 && elemA.landscapeLipDirection != 1 && elemA.landscapeLipDirection != 2 &&
                    (elemB.landscapeLipDirection == 4 || elemB.landscapeLipDirection == 0)) {
                    mergeElementEdgeHor(elemA.vertexListBottom, elemB.vertexListTop, vtxMgr.vertDefs);
                }
            }
        }
    }

    // Fill edges on world map boundaries
    for (auto& elem : elements) {
        for (const auto& m : allWorldMaps) {
            int mTLX = m.tlx;
            int mTLY = m.tly;
            int mBRX = m.brx;
            int mBRY = m.bry;

            if (elem.startX == mBRX && elem.startY <= mBRY && mTLY <= elem.endY) {
                fillElementEdgeVert(elem.vertexListLeft, mTLY, mTLY, vtxMgr);
                fillElementEdgeVert(elem.vertexListLeft, mBRY, mBRY, vtxMgr);
            }
            if (elem.endX == mTLX && elem.startY <= mBRY && mTLY <= elem.endY) {
                fillElementEdgeVert(elem.vertexListRight, mTLY, mTLY, vtxMgr);
                fillElementEdgeVert(elem.vertexListRight, mBRY, mBRY, vtxMgr);
            }
            if (elem.startY == mBRY && elem.startX <= mBRX && mTLX <= elem.endX) {
                fillElementEdgeHor(elem.vertexListTop, mTLX, mTLX, vtxMgr);
                fillElementEdgeHor(elem.vertexListTop, mBRX, mBRX, vtxMgr);
            }
            if (elem.endY == mTLY && elem.startX <= mBRX && mTLX <= elem.endX) {
                fillElementEdgeHor(elem.vertexListBottom, mTLX, mTLX, vtxMgr);
                fillElementEdgeHor(elem.vertexListBottom, mBRX, mBRX, vtxMgr);
            }
        }
    }

    // 8. Build index buffer
    std::vector<uint16_t> triangleIndices;
    triangleIndices.reserve(elements.size() * 32);

    for (const auto& elem : elements) {
        uint16_t midVtx = static_cast<uint16_t>(elem.middleVertex);

        // Top edge: (Mid, Top[i+1], Top[i])
        for (size_t i = 0; i + 1 < elem.vertexListTop.size(); ++i) {
            triangleIndices.push_back(midVtx);
            triangleIndices.push_back(static_cast<uint16_t>(elem.vertexListTop[i + 1]));
            triangleIndices.push_back(static_cast<uint16_t>(elem.vertexListTop[i]));
        }

        // Right edge: (Mid, Right[i+1], Right[i])
        for (size_t i = 0; i + 1 < elem.vertexListRight.size(); ++i) {
            triangleIndices.push_back(midVtx);
            triangleIndices.push_back(static_cast<uint16_t>(elem.vertexListRight[i + 1]));
            triangleIndices.push_back(static_cast<uint16_t>(elem.vertexListRight[i]));
        }

        // Bottom edge: (Mid, Bottom[i], Bottom[i+1])
        for (size_t i = 0; i + 1 < elem.vertexListBottom.size(); ++i) {
            triangleIndices.push_back(midVtx);
            triangleIndices.push_back(static_cast<uint16_t>(elem.vertexListBottom[i]));
            triangleIndices.push_back(static_cast<uint16_t>(elem.vertexListBottom[i + 1]));
        }

        // Left edge: (Mid, Left[i], Left[i+1])
        for (size_t i = 0; i + 1 < elem.vertexListLeft.size(); ++i) {
            triangleIndices.push_back(midVtx);
            triangleIndices.push_back(static_cast<uint16_t>(elem.vertexListLeft[i]));
            triangleIndices.push_back(static_cast<uint16_t>(elem.vertexListLeft[i + 1]));
        }
    }

    // Stripify indices using CTriangleStripifier2
    std::vector<uint16_t> outList;
    std::vector<uint16_t> outStrip;
    TriangleStripifier::generateTriangleListAndStrip(triangleIndices, outList, outStrip);

    // 9. Extra Header Block (16 bytes)
    // tag = 12, ver = 22, streamPos = 0, crc = cutoutBoxesCRC
    entry.extra.resize(16);
    uint32_t tag = 12;
    uint32_t ver = 22;
    uint32_t streamPos = 0;
    std::memcpy(entry.extra.data() + 0, &tag, 4);
    std::memcpy(entry.extra.data() + 4, &ver, 4);
    std::memcpy(entry.extra.data() + 8, &streamPos, 4);
    std::memcpy(entry.extra.data() + 12, &cutoutBoxesCRC, 4);

    // 10. Range compress vertices (12 bytes per vertex: float x, y, z)
    struct Vertex12 {
        float x, y, z;
    };
    std::vector<Vertex12> vtxBuf(vtxMgr.vertDefs.size());
    for (size_t i = 0; i < vtxMgr.vertDefs.size(); ++i) {
        vtxBuf[i].x = static_cast<float>(vtxMgr.vertDefs[i].x);
        vtxBuf[i].y = static_cast<float>(vtxMgr.vertDefs[i].y);
        vtxBuf[i].z = vtxMgr.vertDefs[i].z;
    }

    int32_t vtxCount = static_cast<int32_t>(vtxBuf.size());
    int32_t idxCount = static_cast<int32_t>(outStrip.size());

    std::vector<uint8_t> compVerts;
    if (vtxCount > 0) {
        compVerts = RangeCompressor::compress(vtxBuf.data(), vtxCount, sizeof(Vertex12));
    }

    std::vector<uint8_t> compIndices;
    if (idxCount > 0) {
        compIndices = RangeCompressor::compress(outStrip.data(), idxCount, sizeof(uint16_t));
    }

    // 11. Write payload:
    // int32_t vertex_count
    // int32_t index_count
    // int32_t compVertSize
    // compVerts
    // int32_t compIndexSize
    // compIndices
    entry.payload.clear();
    auto appendI32 = [&](int32_t val) {
        size_t off = entry.payload.size();
        entry.payload.resize(off + 4);
        std::memcpy(entry.payload.data() + off, &val, 4);
    };

    appendI32(vtxCount);
    appendI32(idxCount);

    if (vtxCount > 0) {
        appendI32(static_cast<int32_t>(compVerts.size()));
        entry.payload.insert(entry.payload.end(), compVerts.begin(), compVerts.end());
    }

    if (idxCount > 0) {
        appendI32(static_cast<int32_t>(compIndices.size()));
        entry.payload.insert(entry.payload.end(), compIndices.begin(), compIndices.end());
    }

    return entry;
}

std::vector<BBBEntry> SeaGenerator::generateAllSeas(
    const std::vector<SeaRegionJob>& jobs,
    const SeaDefConfig& defConfig,
    const std::vector<SeaMapInfo>& allWorldMaps,
    int numThreads,
    const std::function<void(const std::string&)>& logCallback
) {
    if (jobs.empty()) return {};

    if (numThreads <= 0) {
        numThreads = static_cast<int>(std::thread::hardware_concurrency());
        if (numThreads <= 0) numThreads = 4;
    }

    std::vector<BBBEntry> entries(jobs.size());

    auto worker = [&](size_t startIdx, size_t endIdx) {
        for (size_t i = startIdx; i < endIdx; ++i) {
            uint32_t entryId = static_cast<uint32_t>(i + 1);
            entries[i] = generateSeaEntry(entryId, jobs[i], defConfig, allWorldMaps);
            if (logCallback) {
                logCallback("  [Sea] Generated " + jobs[i].regionName +
                            " (id=" + std::to_string(entryId) +
                            ", payload=" + std::to_string(entries[i].payload.size()) + " bytes)");
            }
        }
    };

    std::vector<std::future<void>> futures;
    size_t total = jobs.size();
    size_t chunkSize = (total + numThreads - 1) / numThreads;

    for (int t = 0; t < numThreads; ++t) {
        size_t startIdx = t * chunkSize;
        size_t endIdx = std::min(startIdx + chunkSize, total);
        if (startIdx < endIdx) {
            futures.push_back(std::async(std::launch::async, worker, startIdx, endIdx));
        }
    }

    for (auto& f : futures) {
        f.get();
    }

    return entries;
}

} // namespace egostb
