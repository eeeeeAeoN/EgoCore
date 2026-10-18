#include "egostb/local_detail_builder.hpp"
#include "egostb/normal_lut.hpp"
#include "egostb/bank_resolver.hpp"
#include "egostb/lzo1x.hpp"
#include "egostb/local_detail_manager.hpp"
#include "egostb/theme_manager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <vector>
#include <array>
#include <unordered_map>
#include <map>
#include <set>
#include <memory>
#include <cassert>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

namespace fs = std::filesystem;

namespace egostb {

namespace {

static size_t alignUp(size_t val, size_t align) {
    if (align == 0) return val;
    return (val + align - 1) & ~(align - 1);
}

inline uint32_t GFROR13(uint32_t x) {
    return (x >> 13) | (x << 19);
}

// 3D vector and bounding volume helpers
struct Vec3 {
    float x = 0, y = 0, z = 0;
};

struct BoundingBox {
    Vec3 minCoord = {1.0e30f, 1.0e30f, 1.0e30f};
    Vec3 maxCoord = {-1.0e30f, -1.0e30f, -1.0e30f};

    void expand(const Vec3& pt) {
        if (pt.x < minCoord.x) minCoord.x = pt.x;
        if (pt.y < minCoord.y) minCoord.y = pt.y;
        if (pt.z < minCoord.z) minCoord.z = pt.z;
        if (pt.x > maxCoord.x) maxCoord.x = pt.x;
        if (pt.y > maxCoord.y) maxCoord.y = pt.y;
        if (pt.z > maxCoord.z) maxCoord.z = pt.z;
    }

    void expand(const Vec3& pt, float radius) {
        expand({pt.x - radius, pt.y - radius, pt.z - radius});
        expand({pt.x + radius, pt.y + radius, pt.z + radius});
    }

    void merge(const BoundingBox& b) {
        expand(b.minCoord);
        expand(b.maxCoord);
    }
};

// Exact retail C3DBoundingBox::InitialiseFromOBB (STBfunctions18.txt:769-935)
static void initialiseFromOBB(BoundingBox& outBox, const BoundingBox& src, const float mat[12]) {
    float cx = (src.minCoord.x + src.maxCoord.x) * 0.5f;
    float cy = (src.minCoord.y + src.maxCoord.y) * 0.5f;
    float cz = (src.minCoord.z + src.maxCoord.z) * 0.5f;

    float ex = (src.maxCoord.x - src.minCoord.x) * 0.5f;
    float ey = (src.maxCoord.y - src.minCoord.y) * 0.5f;
    float ez = (src.maxCoord.z - src.minCoord.z) * 0.5f;

    float c0x = ex * mat[0] + ey * mat[3] + ez * mat[6];
    float c0y = ex * mat[1] + ey * mat[4] + ez * mat[7];
    float c0z = ex * mat[2] + ey * mat[5] + ez * mat[8];

    float c1x = ex * mat[0] - ey * mat[3] + ez * mat[6];
    float c1y = ex * mat[1] - ey * mat[4] + ez * mat[7];
    float c1z = ex * mat[2] - ey * mat[5] + ez * mat[8];

    float c2x = -ex * mat[0] + ey * mat[3] + ez * mat[6];
    float c2y = -ex * mat[1] + ey * mat[4] + ez * mat[7];
    float c2z = -ex * mat[2] + ey * mat[5] + ez * mat[8];

    float c3x = -ex * mat[0] - ey * mat[3] + ez * mat[6];
    float c3y = -ex * mat[1] - ey * mat[4] + ez * mat[7];
    float c3z = -ex * mat[2] - ey * mat[5] + ez * mat[8];

    float tx = cx * mat[0] + cy * mat[3] + cz * mat[6] + mat[9];
    float ty = cx * mat[1] + cy * mat[4] + cz * mat[7] + mat[10];
    float tz = cx * mat[2] + cy * mat[5] + cz * mat[8] + mat[11];

    outBox.minCoord.x = tx + c0x;
    outBox.maxCoord.x = tx + c0x;
    outBox.minCoord.y = ty + c0y;
    outBox.maxCoord.y = ty + c0y;
    outBox.minCoord.z = tz + c0z;
    outBox.maxCoord.z = tz + c0z;

    auto update = [&](float x, float y, float z) {
        if (x < outBox.minCoord.x) outBox.minCoord.x = x;
        if (y < outBox.minCoord.y) outBox.minCoord.y = y;
        if (z < outBox.minCoord.z) outBox.minCoord.z = z;
        if (x > outBox.maxCoord.x) outBox.maxCoord.x = x;
        if (y > outBox.maxCoord.y) outBox.maxCoord.y = y;
        if (z > outBox.maxCoord.z) outBox.maxCoord.z = z;
    };

    update(tx - c0x, ty - c0y, tz - c0z);
    update(tx + c1x, ty + c1y, tz + c1z);
    update(tx - c1x, ty - c1y, tz - c1z);
    update(tx + c2x, ty + c2y, tz + c2z);
    update(tx - c2x, ty - c2y, tz - c2z);
    update(tx + c3x, ty + c3y, tz + c3z);
    update(tx - c3x, ty - c3y, tz - c3z);
}
struct BoundingSphere {
    Vec3 center = {0, 0, 0};
    float radius = 0;

    static BoundingSphere fromBox(const BoundingBox& box) {
        BoundingSphere bs;
        bs.center.x = (box.minCoord.x + box.maxCoord.x) * 0.5f;
        bs.center.y = (box.minCoord.y + box.maxCoord.y) * 0.5f;
        bs.center.z = (box.minCoord.z + box.maxCoord.z) * 0.5f;
        float dx = box.maxCoord.x - bs.center.x;
        float dy = box.maxCoord.y - bs.center.y;
        float dz = box.maxCoord.z - bs.center.z;
        bs.radius = std::sqrt(dx * dx + dy * dy + dz * dz);
        return bs;
    }

    static BoundingSphere merge(const std::vector<BoundingSphere>& list) {
        BoundingSphere bs;
        if (list.empty()) return bs;

        float minx = 1.0e30f;
        float miny = 1.0e30f;
        float minz = 1.0e30f;
        float maxx = -1.0e30f;
        float maxy = -1.0e30f;
        float maxz = -1.0e30f;
        for (const auto& s : list) {
            float c_minus_r_x = s.center.x - s.radius;
            if (c_minus_r_x < minx) minx = c_minus_r_x;
            float c_minus_r_y = s.center.y - s.radius;
            if (c_minus_r_y < miny) miny = c_minus_r_y;
            float c_minus_r_z = s.center.z - s.radius;
            if (c_minus_r_z < minz) minz = c_minus_r_z;

            float c_plus_r_x = s.center.x + s.radius;
            if (c_plus_r_x > maxx) maxx = c_plus_r_x;
            float c_plus_r_y = s.center.y + s.radius;
            if (c_plus_r_y > maxy) maxy = c_plus_r_y;
            float c_plus_r_z = s.center.z + s.radius;
            if (c_plus_r_z > maxz) maxz = c_plus_r_z;
        }

        bs.center.x = (maxx + minx) * 0.5f;
        bs.center.y = (maxy + miny) * 0.5f;
        bs.center.z = (maxz + minz) * 0.5f;
        bs.radius = 0.0f;

        for (const auto& s : list) {
            float dx = s.center.x - bs.center.x;
            float dy = s.center.y - bs.center.y;
            float dz = s.center.z - bs.center.z;
            float dist = std::sqrt(dx * dx + dy * dy + dz * dz) + s.radius;
            if (dist > bs.radius) bs.radius = dist;
        }
        return bs;
    }
};

// --- CLocalDetailPrimitiveRepeatedMesh Subsection Builder (FableWin 0x02EDF740 / 0x02EDFB20) ---
static int32_t x87Fistp(double x) {
    if (!(x >= -2147483648.0 && x <= 2147483647.0)) return INT32_MIN;
    const double r = std::nearbyint(x);
    if (!(r >= -2147483648.0 && r <= 2147483647.0)) return INT32_MIN;
    return int32_t(r);
}

static int32_t roundToInt(float x) { return x87Fistp(double(x)); }

static int32_t ceilToInt(float x) {
    int32_t n = x87Fistp(double(x) + 0.5);
    const float t = float(n - 1);
    uint32_t bx = 0, bt = 0;
    std::memcpy(&bx, &x, 4);
    std::memcpy(&bt, &t, 4);
    if (bx == bt) --n;
    return n;
}

static float engineSqrt(float x) { return float(std::sqrt(double(x))); }

static float magnitude(float dx, float dy, float dz) {
    const float sum = float(double(dx) * dx + double(dy) * dy + double(dz) * dz);
    return engineSqrt(sum);
}

struct SubsectionSphere {
    float x = 0.0f, y = 0.0f, z = 0.0f, radius = 0.0f;
};

struct SubsectionElement {
    float centreX[4]{}, centreY[4]{}, centreZ[4]{}, radius[4]{};
    uint8_t count[4]{}, startIndex[4]{}, childOffset[4]{};
    uint8_t tail[4]{};
};

struct SubsectionTable {
    bool present = false;
    std::vector<SubsectionElement> elements;
    std::vector<uint8_t> remap;
};

constexpr float kPlusHuge = 1e30f;
constexpr float kMinusHuge = -1e30f;
constexpr size_t kSubsectionScratch = 32;

static bool buildSubsectionsWorker(std::vector<SubsectionElement>& elements,
                            long first, long count, long leafThreshold,
                            const std::vector<SubsectionSphere>& spheres,
                            std::vector<uint8_t>& remap) {
    if (!(count > leafThreshold)) return false;

    int N = ceilToInt(engineSqrt(float(count)));
    if (N < 1) N = 1;
    else if (N > 8) N = 8;

    int gridCount[8][8];
    for (int a = 0; a < N; ++a)
        for (int b = 0; b < N; ++b) gridCount[b][a] = 0;
    std::vector<std::array<std::array<int, 8>, 8>> bucket(kSubsectionScratch);

    float boxMinX = kPlusHuge, boxMinY = kPlusHuge;
    float boxMaxX = kMinusHuge, boxMaxY = kMinusHuge;
    for (long i = first; i < first + count; ++i) {
        const SubsectionSphere& s = spheres[remap[size_t(i)]];
        if (!(boxMinX <= s.x)) boxMinX = s.x;
        if (!(boxMinY <= s.y)) boxMinY = s.y;
        if (!(boxMaxX >= s.x)) boxMaxX = s.x;
        if (!(boxMaxY >= s.y)) boxMaxY = s.y;
    }

    for (long i = first; i < first + count; ++i) {
        const int inst = remap[size_t(i)];
        const SubsectionSphere& s = spheres[size_t(inst)];
        const float fx = float((double(s.x - boxMinX) / double(boxMaxX - boxMinX))
                               * double(N - 1));
        int cx = roundToInt(fx);
        const float fy = float((double(s.y - boxMinY) / double(boxMaxY - boxMinY))
                               * double(N - 1));
        int cy = roundToInt(fy);
        if (cx < 0) cx = 0; else if (cx > N - 1) cx = N - 1;
        if (cy < 0) cy = 0; else if (cy > N - 1) cy = N - 1;
        const int depth = gridCount[cx][cy];
        if (size_t(depth) >= bucket.size())
            return false;
        bucket[size_t(depth)][size_t(cx)][size_t(cy)] = inst;
        gridCount[cx][cy] = depth + 1;
    }

    int nonEmpty = 0;
    uint8_t sectionList[4][kSubsectionScratch]{};
    int sectionSize[4] = {0, 0, 0, 0};
    long remaining = count;
    for (int q = 0; q < 4; ++q) {
        long quota = remaining;
        if (q < 3) {
            const long v = remaining / (4 - q);
            if (v <= leafThreshold) {
                quota = v;
            } else {
                quota = leafThreshold;
                while (quota * 4 < remaining) quota *= 4;
            }
        }
        int i = 0, j = 0;
        while (sectionSize[q] < quota && remaining > 0) {
            int ii = i, jj = j;
            if (q & 1) ii = (N - 1) - i;
            if (q & 2) jj = (N - 1) - j;
            int& cell = gridCount[ii][jj];
            while (cell > 0 && sectionSize[q] < quota) {
                --cell;
                const int inst = bucket[size_t(cell)][size_t(ii)][size_t(jj)];
                sectionList[q][sectionSize[q]] = uint8_t(inst);
                ++sectionSize[q];
                --remaining;
            }
            if (i > j) {
                ++j;
                if (i == j) i = 0;
            } else {
                ++i;
                if (i == j + 1) {
                    if (i == N) break;
                    j = 0;
                }
            }
        }
        if (sectionSize[q] > 0) ++nonEmpty;
    }
    if (sectionSize[0] + sectionSize[1] + sectionSize[2] + sectionSize[3] != count)
        return false;
    if (!(nonEmpty > 1)) return false;

    if (elements.size() >= kSubsectionScratch)
        return false;
    const size_t me = elements.size();
    elements.emplace_back();
    long writeIdx = first;

    for (int q = 0; q < 4; ++q) {
        if (sectionSize[q] <= 0) {
            SubsectionElement& e = elements[me];
            e.count[q] = 0;
            e.childOffset[q] = 0;
            e.centreX[q] = e.centreY[q] = e.centreZ[q] = 0.0f;
            e.radius[q] = 0.0f;
            e.startIndex[q] = 0;
            continue;
        }
        const long sectionStart = writeIdx;
        for (int k = 0; k < sectionSize[q]; ++k)
            remap[size_t(writeIdx++)] = sectionList[q][k];
        const int n = sectionSize[q];
        elements[me].count[q] = uint8_t(n);
        elements[me].startIndex[q] = uint8_t(sectionStart);

        float minX = kPlusHuge, minY = kPlusHuge, minZ = kPlusHuge;
        float maxX = kMinusHuge, maxY = kMinusHuge, maxZ = kMinusHuge;
        for (int k = 0; k < n; ++k) {
            const SubsectionSphere& s = spheres[sectionList[q][k]];
            if (!(minX <= s.x - s.radius)) minX = s.x - s.radius;
            if (!(minY <= s.y - s.radius)) minY = s.y - s.radius;
            if (!(minZ <= s.z - s.radius)) minZ = s.z - s.radius;
            if (!(maxX >= s.x + s.radius)) maxX = s.x + s.radius;
            if (!(maxY >= s.y + s.radius)) maxY = s.y + s.radius;
            if (!(maxZ >= s.z + s.radius)) maxZ = s.z + s.radius;
        }
        const float cX = float((double(minX) + double(maxX)) / 2.0);
        const float cY = float((double(minY) + double(maxY)) / 2.0);
        const float cZ = float((double(minZ) + double(maxZ)) / 2.0);
        float radius = 0.0f;
        for (int k = 0; k < n; ++k) {
            const SubsectionSphere& s = spheres[sectionList[q][k]];
            const float d = magnitude(s.x - cX, s.y - cY, s.z - cZ) + s.radius;
            if (d > radius) radius = d;
        }
        elements[me].centreX[q] = cX;
        elements[me].centreY[q] = cY;
        elements[me].centreZ[q] = cZ;
        elements[me].radius[q] = radius;

        const size_t offset = elements.size() - me;
        if (!(offset > 0 && offset < 256))
            return false;
        const bool hasChild = buildSubsectionsWorker(
            elements, sectionStart, n, leafThreshold, spheres, remap);
        elements[me].childOffset[q] = hasChild ? uint8_t(offset) : uint8_t(0);
    }
    return true;
}

static SubsectionTable buildSubSectionsAndObjectRemapTable(
    const std::vector<SubsectionSphere>& spheres, long maxObjectsPerSubSection) {
    SubsectionTable out;
    if (spheres.empty() || spheres.size() >= 256)
        return out;

    out.remap.resize(spheres.size());
    for (size_t i = 0; i < spheres.size(); ++i)
        out.remap[i] = uint8_t(i);
    out.elements.reserve(kSubsectionScratch);
    out.present = buildSubsectionsWorker(
        out.elements, 0, long(spheres.size()), maxObjectsPerSubSection,
        spheres, out.remap);
    if (!out.present) {
        out.elements.clear();
        for (size_t i = 0; i < spheres.size(); ++i) out.remap[i] = uint8_t(i);
    }
    return out;
}

// Exact retail sphere transform (STBfunctions18.txt:520-547)
static BoundingSphere transformMeshSphere(const MeshBounds& mb, const float mat[12], float scale) {
    BoundingSphere s;
    s.radius = mb.sphereRadius * scale;
    s.center.x = mb.sphereCenter[0] * mat[0] + mb.sphereCenter[1] * mat[3] + mb.sphereCenter[2] * mat[6] + mat[9];
    s.center.y = mb.sphereCenter[0] * mat[1] + mb.sphereCenter[1] * mat[4] + mb.sphereCenter[2] * mat[7] + mat[10];
    s.center.z = mb.sphereCenter[0] * mat[2] + mb.sphereCenter[1] * mat[5] + mb.sphereCenter[2] * mat[8] + mat[11];
    return s;
}

// 44-byte QuadTree Element file header
#pragma pack(push, 1)
struct QuadTreeElementFileHeader {
    float sphereX = 0, sphereY = 0, sphereZ = 0, sphereR = 0; // 16 bytes
    float maxFadeDistance = 118.0f; // 4 bytes
    uint32_t primitiveTypeMask = 7; // 4 bytes
    int32_t fileBlockPos = 0;       // 4 bytes
    int32_t fileBlockSize = 0;      // 4 bytes
    int32_t offsetIntoFileBlock = 0;// 4 bytes
    uint16_t mapX = 0;              // 2 bytes
    uint16_t mapY = 0;              // 2 bytes
    uint16_t width = 16;            // 2 bytes
    uint16_t height = 16;           // 2 bytes
};
#pragma pack(pop)
static_assert(sizeof(QuadTreeElementFileHeader) == 44, "QuadTreeElementFileHeader must be 44 bytes");

// 40-byte CObjectCacheGroupCollection::CFileHeader
#pragma pack(push, 1)
struct CacheGroupCollectionFileHeader {
    int32_t fileBlockPos = 0;
    int32_t fileBlockSize = 0;
    int32_t offsetIntoFileBlock = 0;
    float sphereX = 0, sphereY = 0, sphereZ = 0, sphereR = 0;
    float maxFadeDistance = 23.0f;
    uint32_t primitiveTypeMask = 3;
    uint16_t cacheGroupIndex = 4;
    uint8_t flags = 0;
    uint8_t padding = 0;
};
#pragma pack(pop)
static_assert(sizeof(CacheGroupCollectionFileHeader) == 40, "CacheGroupCollectionFileHeader must be 40 bytes");
// Retail CEngineMap landscape normal and height helpers
static float peekLandscapeHeight(const LevData& lev, int x, int y) {
    if (x < 0) x = 0;
    else if (x >= lev.width) x = lev.width - 1;
    if (y < 0) y = 0;
    else if (y >= lev.height) y = lev.height - 1;
    float raw = lev.getCell(x, y).height;
    int quantized = static_cast<int>(static_cast<double>(raw) * 128.0);
    return static_cast<float>(static_cast<double>(quantized) * 0.0078125);
}

static Vec3 peekMapNormal(const LevData& lev, int xp, int yp) {
    float height_left = peekLandscapeHeight(lev, xp - 1, yp);
    float height_right = peekLandscapeHeight(lev, xp + 1, yp);
    float height_top = peekLandscapeHeight(lev, xp, yp - 1);
    float height_bottom = peekLandscapeHeight(lev, xp, yp + 1);

    float horX = height_left - height_right;
    float horY = 2.0f;
    float h_inv = fastInvSqrtEngine(horX * horX + horY * horY);
    horX *= h_inv;
    horY *= h_inv;

    float verX = height_top - height_bottom;
    float verY = 2.0f;
    float v_inv = fastInvSqrtEngine(verX * verX + verY * verY);
    verX *= v_inv;
    verY *= v_inv;

    float normZ = verY * horY;
    float normX = horX;
    float normY = verX;

    float invLen = fastInvSqrtEngine(normX * normX + normY * normY + normZ * normZ);
    return {normX * invLen, normY * invLen, normZ * invLen};
}

static Vec3 peekInterpolatedMapNormal(const LevData& lev, float xp, float yp) {
    int v29 = static_cast<int>(xp - 0.5f);
    if (xp == static_cast<float>(v29 + 1.0f)) v29++;
    int ypa = static_cast<int>(yp - 0.5f);
    if (yp == static_cast<float>(ypa + 1.0f)) ypa++;

    float frac_x = xp - static_cast<float>(v29);
    float frac_y = yp - static_cast<float>(ypa);

    Vec3 normal00 = peekMapNormal(lev, v29, ypa);
    Vec3 normal10 = peekMapNormal(lev, v29 + 1, ypa);
    Vec3 normal01 = peekMapNormal(lev, v29, ypa + 1);
    Vec3 normal11 = peekMapNormal(lev, v29 + 1, ypa + 1);

    float w00 = (1.0f - frac_x) * (1.0f - frac_y);
    float w10 = frac_x * (1.0f - frac_y);
    float w01 = (1.0f - frac_x) * frac_y;
    float w11 = frac_x * frac_y;

    Vec3 res = {
        normal00.x * w00 + normal10.x * w10 + normal01.x * w01 + normal11.x * w11,
        normal00.y * w00 + normal10.y * w10 + normal01.y * w01 + normal11.y * w11,
        normal00.z * w00 + normal10.z * w10 + normal01.z * w01 + normal11.z * w11
    };

    float invLen = fastInvSqrtEngine(res.x * res.x + res.y * res.y + res.z * res.z);
    return {res.x * invLen, res.y * invLen, res.z * invLen};
}

// Displacement Table: 32x32x32 precomputed PRNG table from Fable
struct DisplacementTable {
    float table[32][32][32];

    void init() {
        uint32_t v4 = 0;
        for (int i = 0; i < 4096; i += 128) {
            int base = i;
            for (int mid = 0; mid < 32; ++mid) {
                int ptr = base;
                for (int inner = 0; inner < 32; ++inner) {
                    v4 = GFROR13(9377 * v4 + 9439);
                    float v59 = static_cast<float>(std::fmod(static_cast<double>(v4), 65536.0) * (1.0 / 65536.0));
                    reinterpret_cast<float*>(table)[ptr / 4] = v59;
                    ptr += 4;
                }
                base += 4096;
            }
        }
    }

    float get(int x, int y, int z) const {
        return table[x & 0x1F][y & 0x1F][z & 0x1F];
    }
};

static const DisplacementTable& getGlobalDisplacementTable() {
    static const DisplacementTable s_dispTable = []() {
        DisplacementTable t;
        t.init();
        return t;
    }();
    return s_dispTable;
}


// Placement Grid: 32x32 cells with Poisson-disc dart throwing and multi-layer quadtree clipping
// Matching CLocalDetailPlacementGrid::BuildElementGrid (STBfunctions25.txt:139-450)
struct PlacementGrid {
    struct Element {
        float x = 0; // [-0.5, 0.5]
        float y = 0; // [-0.5, 0.5]
    };

    struct Point {
        float x = 0;
        float y = 0;
    };

    std::vector<Element> cellElements[32][32];
    std::vector<Point> quadTreePoints; // Points placed in this grid layer
    std::vector<float> layerSpacings;  // Spacing from this layer to each layer 0..LayerID
    std::shared_ptr<PlacementGrid> baseLayer; // Pointer to layer - 1

    bool clipPoint(float x, float y, float r) const {
        float rSq = r * r;
        for (const auto& pt : quadTreePoints) {
            float dx = std::abs(pt.x - x);
            if (dx > 16.0f) dx = 32.0f - dx;
            float dy = std::abs(pt.y - y);
            if (dy > 16.0f) dy = 32.0f - dy;
            if (dx * dx + dy * dy < rSq) {
                return false;
            }
        }
        return true;
    }

    void build(const std::vector<float>& spacings, std::shared_ptr<PlacementGrid> base) {
        baseLayer = base;
        layerSpacings = spacings;

        uint32_t v58 = 0;
        uint32_t v59 = 0;
        int fails = 256;

        while (fails > 0) {
            fails--;
            v58 = GFROR13(9377 * v59 + 9439);
            float x = static_cast<float>(std::fmod(static_cast<double>(v58), 2097152.0) * 0.0000152587890625);
            v58 = GFROR13(9377 * v58 + 9439);
            v59 = v58;
            float y = static_cast<float>(std::fmod(static_cast<double>(v58), 2097152.0) * 0.0000152587890625);

            // Multi-layer clipping hierarchy: clip against this grid and all ancestor base layers
            // Matching lines 236-347 of STBfunctions25.txt
            const PlacementGrid* cur = this;
            int v8 = static_cast<int>(layerSpacings.size()) - 1;
            bool accepted = true;

            while (cur != nullptr && v8 >= 0) {
                float sp = this->layerSpacings[v8];
                if (!cur->clipPoint(x, y, sp)) {
                    accepted = false;
                    break;
                }
                cur = cur->baseLayer.get();
                v8--;
            }

            if (accepted) {
                fails = 256;

                // Add to spatial points for clipping subsequent darts in this layer
                quadTreePoints.push_back({x, y});

                // Wrap-around offset calculation and cell indexing (STBfunctions25.txt:247-295)
                int tx = static_cast<int>(x + 0.5f) & 0x1F;
                int ty = static_cast<int>(y + 0.5f) & 0x1F;

                float subX = x - static_cast<float>(tx);
                if (subX > 16.0f) subX -= 32.0f;
                else if (subX < -16.0f) subX += 32.0f;

                float subY = y - static_cast<float>(ty);
                if (subY > 16.0f) subY -= 32.0f;
                else if (subY < -16.0f) subY += 32.0f;

                // Retail 8-bit quantization and reconstruction (STBfunctions25.txt:384-415, STBfunctions23.txt:20-35)
                float valX = (subX + 0.5f) * 255.0f;
                int px = static_cast<int>(valX - 0.5f);
                if (valX == static_cast<float>(px + 1.0f)) px++;
                px = std::clamp(px, 0, 255);

                float valY = (subY + 0.5f) * 255.0f;
                int py = static_cast<int>(valY - 0.5f);
                if (valY == static_cast<float>(py + 1.0f)) py++;
                py = std::clamp(py, 0, 255);

                Element el;
                el.x = static_cast<float>(static_cast<double>(px) / 255.0 - 0.5);
                el.y = static_cast<float>(static_cast<double>(py) / 255.0 - 0.5);

                // Note: STBfunctions25.txt:297 uses v26 = v13 + 32 * v12 where v12 = tx, v13 = ty
                // cellElements[tx][ty]
                cellElements[tx][ty].push_back(el);
            }
        }
    }
};

static std::mutex s_gridCacheMutex;
static std::unordered_map<std::string, std::vector<std::shared_ptr<PlacementGrid>>> s_generatorGridCache;

// Placed clutter instance

struct ClutterInstance {
    int palIndex = 0;
    int cacheGroup = 4;
    float x = 0, y = 0, z = 0;
    float scale = 1.0f;
    float angle = 0.0f;
    float nx = 0, ny = 0, nz = 1.0f;
    bool hasCustomMat = false;
    float customMat[12] = {0};
};



// A collection of up to 64 instances (CObjectCacheGroupCollection)
struct ClutterGroup {
    int cacheGroupIndex = 4;
    std::vector<ClutterInstance> instances; // max 64
    std::vector<uint8_t> compressedFrame;
    size_t uncompressedSize = 0;
    uint32_t fileBlockPos = 0;
    uint32_t fileBlockSize = 0;
    uint32_t offsetIntoFileBlock = 0;
    BoundingSphere sphere;
    float maxFadeDistance = 23.0f;
    uint32_t primitiveTypeMask = 3;
    uint8_t flags = 0;
};

// Quadtree Node (arbitrary dimensions, power-of-2 subdivided, dynamic pruning)
struct QuadTreeNode {
    int x = 0, y = 0, w = 16, h = 16;
    BoundingBox box;
    BoundingSphere sphere;
    float maxFadeDistance = 23.0f;
    uint32_t primitiveTypeMask = 3;
    uint32_t fileBlockPos = 0;
    uint32_t fileBlockSize = 0;
    uint32_t offsetIntoFileBlock = 0;
    std::unique_ptr<QuadTreeNode> children[4];
    std::vector<ClutterGroup> collections;
};

// ---------------------------------------------------------------------------
// Retail AssignFileBlocks, SaveFileBlock, SaveSubFileBlocks, SaveTree
// ---------------------------------------------------------------------------

static int assignFileBlocks(QuadTreeNode* node, std::set<QuadTreeNode*>& blockMap, bool isRoot) {
    int v5 = 0;
    for (const auto& coll : node->collections) {
        int saveSize = static_cast<int>(coll.uncompressedSize + 40);
        if (saveSize <= 32768) {
            v5 += saveSize;
        } else {
            // Individual collection > 32KB (none in standard clutter)
        }
    }
    for (int c = 0; c < 4; ++c) {
        if (node->children[c]) {
            v5 += assignFileBlocks(node->children[c].get(), blockMap, false);
        }
    }
    int v12 = v5 + 64;
    if (v12 > 32768 || isRoot) {
        blockMap.insert(node);
        v12 = 0;
    }
    return v12;
}

static void saveFileBlock(QuadTreeNode* node, std::vector<uint8_t>& stream, size_t blockStartInStream,
                          size_t blockStartFilePos, size_t blockSize, const std::set<QuadTreeNode*>& blockMap, bool isPass1) {
    // 1. Collections: SaveContents
    for (auto& coll : node->collections) {
        coll.fileBlockPos = static_cast<uint32_t>(blockStartFilePos);
        coll.offsetIntoFileBlock = static_cast<uint32_t>(stream.size() - blockStartInStream);

        stream.insert(stream.end(), coll.compressedFrame.begin(), coll.compressedFrame.end());

        if (blockSize > 0) {
            coll.fileBlockSize = static_cast<uint32_t>(blockSize);
        }
    }

    // 2. Recurse to children that are NOT in blockMap
    for (int c = 0; c < 4; ++c) {
        QuadTreeNode* child = node->children[c].get();
        if (child && blockMap.find(child) == blockMap.end()) {
            saveFileBlock(child, stream, blockStartInStream, blockStartFilePos, blockSize, blockMap, isPass1);
        }
    }

    // 3. Write this node's header & children headers
    node->fileBlockPos = static_cast<uint32_t>(blockStartFilePos);
    node->offsetIntoFileBlock = static_cast<uint32_t>(stream.size() - blockStartInStream);

    int32_t colCount = static_cast<int32_t>(node->collections.size());
    stream.insert(stream.end(), reinterpret_cast<const uint8_t*>(&colCount), reinterpret_cast<const uint8_t*>(&colCount) + 4);

    for (const auto& coll : node->collections) {
        CacheGroupCollectionFileHeader chdr;
        chdr.fileBlockPos = coll.fileBlockPos;
        chdr.fileBlockSize = coll.fileBlockSize;
        chdr.offsetIntoFileBlock = coll.offsetIntoFileBlock;
        chdr.sphereX = coll.sphere.center.x;
        chdr.sphereY = coll.sphere.center.y;
        chdr.sphereZ = coll.sphere.center.z;
        chdr.sphereR = coll.sphere.radius;
        chdr.maxFadeDistance = coll.maxFadeDistance;
        chdr.primitiveTypeMask = coll.primitiveTypeMask;
        chdr.cacheGroupIndex = static_cast<uint16_t>(coll.cacheGroupIndex);
        chdr.flags = coll.flags;
        chdr.padding = 1;
        stream.insert(stream.end(), reinterpret_cast<const uint8_t*>(&chdr), reinterpret_cast<const uint8_t*>(&chdr) + 40);
    }

    for (int c = 0; c < 4; ++c) {
        QuadTreeNode* child = node->children[c].get();
        if (child) {
            int32_t hasChild = 1;
            stream.insert(stream.end(), reinterpret_cast<const uint8_t*>(&hasChild), reinterpret_cast<const uint8_t*>(&hasChild) + 4);

            QuadTreeElementFileHeader ehdr;
            ehdr.sphereX = child->sphere.center.x;
            ehdr.sphereY = child->sphere.center.y;
            ehdr.sphereZ = child->sphere.center.z;
            ehdr.sphereR = child->sphere.radius;
            ehdr.maxFadeDistance = child->maxFadeDistance;
            ehdr.primitiveTypeMask = child->primitiveTypeMask;
            ehdr.fileBlockPos = child->fileBlockPos;
            ehdr.fileBlockSize = child->fileBlockSize;
            ehdr.offsetIntoFileBlock = child->offsetIntoFileBlock;
            ehdr.mapX = static_cast<uint16_t>(child->x);
            ehdr.mapY = static_cast<uint16_t>(child->y);
            ehdr.width = static_cast<uint16_t>(child->w);
            ehdr.height = static_cast<uint16_t>(child->h);
            stream.insert(stream.end(), reinterpret_cast<const uint8_t*>(&ehdr), reinterpret_cast<const uint8_t*>(&ehdr) + 44);
        } else {
            int32_t hasChild = 0;
            stream.insert(stream.end(), reinterpret_cast<const uint8_t*>(&hasChild), reinterpret_cast<const uint8_t*>(&hasChild) + 4);
        }
    }

    if (blockSize > 0) {
        node->fileBlockSize = static_cast<uint32_t>(blockSize);
    }
}

static void saveTree(QuadTreeNode* node, std::vector<uint8_t>& stream, size_t initialPayloadOffset,
                     const std::set<QuadTreeNode*>& blockMap);

static void saveSubFileBlocks(QuadTreeNode* node, std::vector<uint8_t>& stream, size_t initialPayloadOffset,
                             const std::set<QuadTreeNode*>& blockMap) {
    for (int c = 0; c < 4; ++c) {
        QuadTreeNode* child = node->children[c].get();
        if (child) {
            if (blockMap.find(child) == blockMap.end()) {
                saveSubFileBlocks(child, stream, initialPayloadOffset, blockMap);
            } else {
                saveTree(child, stream, initialPayloadOffset, blockMap);
            }
        }
    }
}

static inline int largestPowerOf2LessThan(int v) {
    if (v <= 2) return 1;
    int x = v - 1;
    int p = 1;
    while ((p << 1) <= x) {
        p <<= 1;
    }
    return p;
}

static void insertOrUpdateDynamicArea(
    QuadTreeNode* node,
    int mapX, int mapY,
    std::unique_ptr<QuadTreeNode> leaf
) {
    if (node->w == 16 && node->h == 16) {
        node->collections = std::move(leaf->collections);
        node->sphere = leaf->sphere;
        node->maxFadeDistance = leaf->maxFadeDistance;
        node->primitiveTypeMask = leaf->primitiveTypeMask;
        return;
    }

    int splitW = (node->w > 16) ? largestPowerOf2LessThan(node->w) : node->w;
    int splitH = (node->h > 16) ? largestPowerOf2LessThan(node->h) : node->h;
    int c = 0;
    if (node->w == 16) {
        c = (mapY >= node->y + splitH) ? 2 : 0;
    } else if (node->h == 16) {
        c = (mapX >= node->x + splitW) ? 1 : 0;
    } else {
        c = (mapX >= node->x + splitW ? 1 : 0) + (mapY >= node->y + splitH ? 2 : 0);
    }

    if (!node->children[c]) {
        auto child = std::make_unique<QuadTreeNode>();
        child->x = (c & 1) ? (node->x + splitW) : node->x;
        child->y = (c & 2) ? (node->y + splitH) : node->y;
        child->w = (c & 1) ? (node->w - splitW) : ((node->w == 16) ? 16 : splitW);
        child->h = (c & 2) ? (node->h - splitH) : ((node->h == 16) ? 16 : splitH);
        node->children[c] = std::move(child);
    }

    insertOrUpdateDynamicArea(node->children[c].get(), mapX, mapY, std::move(leaf));

    // Prune if child has no collections and no children
    bool childHasDescendants = false;
    for (int i = 0; i < 4; ++i) {
        if (node->children[c]->children[i]) {
            childHasDescendants = true;
            break;
        }
    }
    if (!childHasDescendants && node->children[c]->collections.empty()) {
        node->children[c].reset();
    }

    // Update this node's bounding sphere, fade distance, and type mask from surviving children
    std::vector<BoundingSphere> childSpheres;
    node->maxFadeDistance = 0.0f;
    node->primitiveTypeMask = 0;
    for (int i = 0; i < 4; ++i) {
        if (node->children[i]) {
            if (node->children[i]->sphere.radius > 0.0f) {
                childSpheres.push_back(node->children[i]->sphere);
            }
            if (node->children[i]->maxFadeDistance > node->maxFadeDistance) {
                node->maxFadeDistance = node->children[i]->maxFadeDistance;
            }
            node->primitiveTypeMask |= node->children[i]->primitiveTypeMask;
        }
    }
    if (!childSpheres.empty()) {
        node->sphere = BoundingSphere::merge(childSpheres);
    } else {
        node->sphere = BoundingSphere();
    }
}

static void saveTree(QuadTreeNode* node, std::vector<uint8_t>& stream, size_t initialPayloadOffset,
                     const std::set<QuadTreeNode*>& blockMap) {
    size_t curPos = initialPayloadOffset + stream.size();
    size_t alignedPos = alignUp(curPos, 2048);
    size_t pad = alignedPos - curPos;
    if (pad > 0) stream.resize(stream.size() + pad, 0);

    size_t blockStartInStream = stream.size();
    size_t blockStartFilePos = initialPayloadOffset + blockStartInStream;

    // Pass 0
    saveFileBlock(node, stream, blockStartInStream, blockStartFilePos, 0, blockMap, false);
    size_t blockSize = stream.size() - blockStartInStream;

    node->fileBlockSize = static_cast<uint32_t>(blockSize);
    for (auto& coll : node->collections) {
        coll.fileBlockSize = static_cast<uint32_t>(blockSize);
    }
    // Update collection headers in stream
    for (size_t ci = 0; ci < node->collections.size(); ++ci) {
        size_t hdrOff = blockStartInStream + node->offsetIntoFileBlock + 4 + ci * 40;
        uint32_t sz = static_cast<uint32_t>(blockSize);
        std::memcpy(stream.data() + hdrOff + 4, &sz, 4);
    }

    // Sub file blocks
    saveSubFileBlocks(node, stream, initialPayloadOffset, blockMap);

    // Pass 1: only needed if node has children
    bool hasAnyChild = (node->children[0] != nullptr || node->children[1] != nullptr ||
                        node->children[2] != nullptr || node->children[3] != nullptr);
    if (hasAnyChild) {
        std::vector<uint8_t> pass1Stream;
        pass1Stream.reserve(blockSize);
        saveFileBlock(node, pass1Stream, 0, blockStartFilePos, blockSize, blockMap, true);

        if (pass1Stream.size() != blockSize) {
            std::cerr << "Warning: Pass 1 size mismatch: " << blockSize << " vs " << pass1Stream.size() << std::endl;
        }
        std::memcpy(stream.data() + blockStartInStream, pass1Stream.data(), blockSize);
    }
}

} // namespace

LocalDetailResult LocalDetailBuilder::build(
    const LevData& lev,
    int worldX, int worldY,
    size_t initialPayloadOffset,
    const std::string& fableRoot,
    const std::string& workDir,
    const std::vector<ObservingMapBounds>* observingMaps
) {
    LocalDetailResult res;

    // 1. Build dynamic clutter palette from level themes and local detail defs
    auto builtPal = LocalDetailManager::instance().buildPaletteForLevel(lev);

    std::vector<MeshBounds> palBounds(builtPal.entries.size());
    for (size_t i = 0; i < builtPal.entries.size(); ++i) {
        palBounds[i] = builtPal.entries[i].bounds;
        if (palBounds[i].sphereRadius == 0.0f) {
            const auto* mb = BankResolver::instance().getMeshBounds(builtPal.entries[i].data.meshBankIndex);
            if (!mb) mb = BankResolver::instance().getMeshBounds(builtPal.entries[i].name);
            if (mb) {
                palBounds[i] = *mb;
            } else {
                palBounds[i].sphereCenter[0] = 0.0f;
                palBounds[i].sphereCenter[1] = 0.0f;
                if (builtPal.entries[i].data.primitiveType == 2) { // Trees (HYBRID_MESH_ZSPRITE)
                    palBounds[i].sphereCenter[2] = 5.0f;
                    palBounds[i].sphereRadius = 8.0f;
                    palBounds[i].boxMin[0] = -5.0f; palBounds[i].boxMin[1] = -5.0f; palBounds[i].boxMin[2] = 0.0f;
                    palBounds[i].boxMax[0] =  5.0f; palBounds[i].boxMax[1] =  5.0f; palBounds[i].boxMax[2] = 15.0f;
                } else if (builtPal.entries[i].data.primitiveType == 0) { // Saplings (MESH)
                    palBounds[i].sphereCenter[2] = 1.5f;
                    palBounds[i].sphereRadius = 2.5f;
                    palBounds[i].boxMin[0] = -1.5f; palBounds[i].boxMin[1] = -1.5f; palBounds[i].boxMin[2] = 0.0f;
                    palBounds[i].boxMax[0] =  1.5f; palBounds[i].boxMax[1] =  1.5f; palBounds[i].boxMax[2] = 4.0f;
                } else { // Grass (REPEATED_MESH)
                    palBounds[i].sphereCenter[2] = 0.3f;
                    palBounds[i].sphereRadius = 0.6f;
                    palBounds[i].boxMin[0] = -0.5f; palBounds[i].boxMin[1] = -0.5f; palBounds[i].boxMin[2] = 0.0f;
                    palBounds[i].boxMax[0] =  0.5f; palBounds[i].boxMax[1] =  0.5f; palBounds[i].boxMax[2] = 0.8f;
                }
            }
        }
    }

    // 2. Initialize PRNG tables & Placement Grids with multi-layer clipping
    const auto& dispTable = getGlobalDisplacementTable();

    std::vector<std::shared_ptr<PlacementGrid>> placementGrids;
    for (auto& [genName, genRuntime] : builtPal.generatorRuntimes) {
        std::vector<std::shared_ptr<PlacementGrid>> cachedGrids;
        {
            std::lock_guard<std::mutex> lock(s_gridCacheMutex);
            auto it = s_generatorGridCache.find(genName);
            if (it != s_generatorGridCache.end()) {
                cachedGrids = it->second;
            }
        }

        if (cachedGrids.empty() && !genRuntime.layers.empty()) {
            std::shared_ptr<PlacementGrid> prevGrid = nullptr;
            for (size_t lIdx = 0; lIdx < genRuntime.layers.size(); ++lIdx) {
                auto& layer = genRuntime.layers[lIdx];
                auto grid = std::make_shared<PlacementGrid>();

                std::vector<float> spacings = layer.spacingFromLayer;
                if (spacings.empty()) {
                    spacings.push_back(layer.spacing);
                }
                grid->build(spacings, prevGrid);

                cachedGrids.push_back(grid);
                prevGrid = grid;
            }
            {
                std::lock_guard<std::mutex> lock(s_gridCacheMutex);
                s_generatorGridCache[genName] = cachedGrids;
            }
        }

        for (size_t lIdx = 0; lIdx < genRuntime.layers.size(); ++lIdx) {
            auto& layer = genRuntime.layers[lIdx];
            layer.gridIndex = static_cast<int>(placementGrids.size());
            placementGrids.push_back(cachedGrids[lIdx]);
        }
    }


    auto addToGroups = [&](std::vector<ClutterGroup>& groups, const ClutterInstance& inst) {
        ClutterGroup* target = nullptr;
        for (auto it = groups.rbegin(); it != groups.rend(); ++it) {
            if (it->cacheGroupIndex == inst.cacheGroup && it->instances.size() < 64) {
                target = &(*it);
                break;
            }
        }
        if (!target) {
            ClutterGroup newG;
            newG.cacheGroupIndex = inst.cacheGroup;
            if (inst.cacheGroup >= 0 && inst.cacheGroup < static_cast<int>(builtPal.cacheFadeGroups.size())) {
                const auto& cg = builtPal.cacheFadeGroups[inst.cacheGroup];
                newG.maxFadeDistance = cg.endDistance;
                newG.primitiveTypeMask = cg.primitiveTypeMask;
                newG.flags = (cg.primitiveTypeMask & 4) ? 1 : 0;
            } else {
                newG.maxFadeDistance = 23.0f;
                newG.primitiveTypeMask = 3;
                newG.flags = 0;
            }
            groups.push_back(std::move(newG));
            target = &groups.back();
        }
        target->instances.push_back(inst);
    };

    // 3. Build Leaf Nodes (16x16 cells each) for arbitrary map dimensions
    int numLeavesX = (lev.width + 15) / 16;
    int numLeavesY = (lev.height + 15) / 16;
    int totalLeaves = numLeavesX * numLeavesY;
    std::vector<QuadTreeNode> leaves(totalLeaves);
    if (totalLeaves > 0) {
        std::atomic<int> nextLeaf{0};
        unsigned int numThreads = std::min(
            std::max(1u, std::thread::hardware_concurrency()),
            static_cast<unsigned int>(totalLeaves)
        );
        std::vector<std::thread> workers;
        workers.reserve(numThreads);
        for (unsigned int t = 0; t < numThreads; ++t) {
            workers.emplace_back([&]() {
                while (true) {
                    int leafIdx = nextLeaf.fetch_add(1);
                    if (leafIdx >= totalLeaves) break;
                    int ly = leafIdx / numLeavesX;
                    int lx = leafIdx % numLeavesX;
                    auto& leaf = leaves[leafIdx];
                    leaf.x = lx * 16;
                    leaf.y = ly * 16;
                    leaf.w = 16;
                    leaf.h = 16;


            for (int p = 0; p < 256; ++p) {
                int vx = 0;
                int vy = 0;
                int v7 = 128;
                int i = 1;
                while (v7 > 0) {
                    if (i) {
                        vx = ((p & v7) != 0) | (2 * vx);
                        i = 0;
                    } else {
                        vy = ((p & v7) != 0) | (2 * vy);
                        i = 1;
                    }
                    v7 /= 2;
                }
                int cx = leaf.x + vx;
                int cy = leaf.y + vy;
                if (cx >= lev.width || cy >= lev.height) continue;
                const auto& cell = lev.getCell(cx, cy);

                uint8_t b0 = cell.blends[0];
                uint8_t b1 = cell.blends[1];
                uint8_t b2 = static_cast<uint8_t>(std::clamp(255 - static_cast<int>(b0) - static_cast<int>(b1), 0, 255));
                uint8_t blends[3] = {b0, b1, b2};

                const LocalDetailGeneratorRuntime* themes[3] = {nullptr, nullptr, nullptr};
                for (int id = 0; id < 3; ++id) {
                    int tSlot = cell.themes[id];
                    if (tSlot >= 0 && tSlot < static_cast<int>(lev.themePalette.size())) {
                        const auto* eth = ThemeManager::instance().getTheme(lev.themePalette[tSlot].name);
                        if (eth && !eth->localDetailGeneratorDef.empty()) {
                            auto it = builtPal.generatorRuntimes.find(eth->localDetailGeneratorDef);
                            if (it != builtPal.generatorRuntimes.end()) {
                                themes[id] = &it->second;
                            }
                        }
                    }
                }

                int v8 = cx + worldX;
                int world_y = cy + worldY;
                int random_seed_index = 0;
                int layer_id = 0;
                bool search_layers = false;
                do {
                    search_layers = false;
                    int element_index = 0;
                    bool search_elements = false;
                    do {
                        int v19 = random_seed_index++;
                        search_elements = false;
                        float v22 = dispTable.get(cx, cy, v19) * 255.0f;
                        int v28 = static_cast<int>(v22 - 0.5f);
                        if (v22 == static_cast<float>(v28 + 1.0f)) v28++;
                        int v11 = 0;
                        if (v28 > blends[0]) {
                            v11 = (v28 > blends[0] + blends[1]) ? 2 : 1;
                        } else {
                            v11 = 0;
                        }
                        const auto* chosenGen = themes[v11];
                        if (chosenGen && layer_id < static_cast<int>(chosenGen->layers.size())) {
                            const auto& layer = chosenGen->layers[layer_id];
                            if (layer.gridIndex >= 0 && layer.gridIndex < static_cast<int>(placementGrids.size())) {
                                const auto& grid = placementGrids[layer.gridIndex];
                                int count = static_cast<int>(grid->cellElements[v8 & 0x1F][world_y & 0x1F].size());
                                if (element_index < count) {
                                    const auto& element = grid->cellElements[v8 & 0x1F][world_y & 0x1F][element_index];
                                    int vRandSlot = random_seed_index++;
                                    float rPick = dispTable.get(cx, cy, vRandSlot) * 32.0f;
                                    int slot = static_cast<int>(rPick - 0.5f);
                                    if (rPick == static_cast<float>(slot + 1.0f)) slot++;
                                    slot = std::clamp(slot, 0, 31);
                                    int objIdx = layer.objectSelectionTable[slot];
                                    if (objIdx >= 0 && objIdx < static_cast<int>(layer.objects.size())) {
                                        const auto& objDef = layer.objects[objIdx];
                                        if (objDef.themeBlendThreshold <= static_cast<float>(blends[v11]) / 255.0f) {
                                            int vRandScale = random_seed_index++;
                                            float rScale = dispTable.get(cx, cy, vRandScale);
                                            float scaleVal = (rScale * 2.0f - 1.0f) * objDef.scaleRandomElement + objDef.scale;
                                            float scale = scaleVal * 0.01f;

                                            float px = static_cast<float>(v8) + element.x;
                                            float py = static_cast<float>(world_y) + element.y;

                                            int cell_world_y = static_cast<int>(std::nearbyint(py - 0.5f));
                                            if (py == static_cast<float>(cell_world_y + 1.0f)) ++cell_world_y;
                                            int cell_world_x = static_cast<int>(std::nearbyint(px - 0.5f));
                                            if (px == static_cast<float>(cell_world_x + 1.0f)) ++cell_world_x;
                                            if (cell_world_x >= worldX && cell_world_y >= worldY &&
                                                cell_world_x < worldX + lev.width && cell_world_y < worldY + lev.height) {

                                                // Retail CEngineMap::PeekInterpolatedMapNormal(xp, yp)
                                                Vec3 norm = peekInterpolatedMapNormal(lev, px - static_cast<float>(worldX), py - static_cast<float>(worldY));

                                                int vRandSlope = random_seed_index++;
                                                float slope_filter_rand = dispTable.get(cx, cy, vRandSlope);

                                                int vRandAngle = random_seed_index++;
                                                float angleDisp = dispTable.get(cx, cy, vRandAngle);

                                                float slope_ref = 1.0f;
                                                if (objDef.slopeFadeStart < objDef.slopeFadeEnd) {
                                                    slope_ref = (norm.z - objDef.slopeFadeStart) / (objDef.slopeFadeEnd - objDef.slopeFadeStart);
                                                    slope_ref = std::clamp(slope_ref, 0.0f, 1.0f);
                                                }
                                                if (slope_filter_rand < slope_ref) {
                                                    float angle = angleDisp * 6.28318530718f;

                                                    // Retail orientation matrix & TiltToSlope (STBfunctions23.txt:383-438)
                                                    float cosA = std::cos(angle);
                                                    float sinA = std::sin(angle);
                                                    float mE11 = cosA * scale;
                                                    float mE12 = sinA * scale;
                                                    float mE13 = 0.0f;
                                                    float mE21 = -sinA * scale;
                                                    float mE22 = cosA * scale;
                                                    float mE23 = 0.0f;
                                                    float mE31 = 0.0f;
                                                    float mE32 = 0.0f;
                                                    float mE33 = scale;

                                                    if (objDef.tiltToSlope) {
                                                        Vec3 z_vec = norm;
                                                        Vec3 x_vec = {z_vec.z, 0.0f, -z_vec.x};
                                                        float xLen = std::sqrt(x_vec.x * x_vec.x + x_vec.y * x_vec.y + x_vec.z * x_vec.z);
                                                        if (xLen > 1e-6f) { x_vec.x /= xLen; x_vec.y /= xLen; x_vec.z /= xLen; }

                                                        Vec3 y_vec = {
                                                            x_vec.z * z_vec.y - x_vec.y * z_vec.z,
                                                            z_vec.z * x_vec.x - z_vec.x * x_vec.z,
                                                            z_vec.x * x_vec.y - x_vec.y * z_vec.x
                                                        };
                                                        float yLen = std::sqrt(y_vec.x * y_vec.x + y_vec.y * y_vec.y + y_vec.z * y_vec.z);
                                                        if (yLen > 1e-6f) { y_vec.x /= yLen; y_vec.y /= yLen; y_vec.z /= yLen; }

                                                        float oldE11 = mE11, oldE12 = mE12, oldE13 = mE13;
                                                        float oldE21 = mE21, oldE22 = mE22, oldE23 = mE23;
                                                        float oldE31 = mE31, oldE32 = mE32, oldE33 = mE33;

                                                        mE11 = oldE12 * y_vec.x + oldE11 * x_vec.x;
                                                        mE12 = oldE11 * x_vec.y + y_vec.y * oldE12;
                                                        mE13 = oldE11 * x_vec.z + y_vec.z * oldE12;

                                                        mE21 = x_vec.x * oldE21 + y_vec.x * oldE22;
                                                        mE22 = x_vec.y * oldE21 + y_vec.y * oldE22;
                                                        mE23 = x_vec.z * oldE21 + y_vec.z * oldE22;

                                                        mE31 = z_vec.x * oldE33 + y_vec.x * oldE32 + x_vec.x * oldE31;
                                                        mE32 = z_vec.y * oldE33 + x_vec.y * oldE31 + y_vec.y * oldE32;
                                                        mE33 = oldE33 * z_vec.z + oldE32 * y_vec.z + oldE31 * x_vec.z;
                                                    }

                                                    // Bilinear interpolation of quantized corner heights (STBfunctions23.txt:439-917)
                                                    float elemX = px - static_cast<float>(worldX);
                                                    float elemY = py - static_cast<float>(worldY);
                                                    int sX = static_cast<int>(elemX - 0.5f);
                                                    if (elemX == static_cast<float>(sX + 1.0f)) sX++;
                                                    int sY = static_cast<int>(elemY - 0.5f);
                                                    if (elemY == static_cast<float>(sY + 1.0f)) sY++;

                                                    float fx = elemX - static_cast<float>(sX);
                                                    float fy = elemY - static_cast<float>(sY);

                                                    float h00 = peekLandscapeHeight(lev, sX, sY);
                                                    float h10 = peekLandscapeHeight(lev, sX + 1, sY);
                                                    float h01 = peekLandscapeHeight(lev, sX, sY + 1);
                                                    float h11 = peekLandscapeHeight(lev, sX + 1, sY + 1);

                                                    float wz = (1.0f - fx) * (1.0f - fy) * h00
                                                             + fx * (1.0f - fy) * h10
                                                             + (1.0f - fx) * fy * h01
                                                             + fx * fy * h11;

                                                    int palIndex = layer.objectPalIndices[objIdx];
                                                    const auto& palEntry = builtPal.entries[palIndex];

                                                    ClutterInstance inst;
                                                    inst.palIndex = palIndex;
                                                    inst.cacheGroup = palEntry.cacheGroupIndex;
                                                    inst.x = px;
                                                    inst.y = py;
                                                    inst.z = wz;
                                                    inst.scale = scale;
                                                    inst.angle = angle;
                                                    inst.nx = norm.x; inst.ny = norm.y; inst.nz = norm.z;
                                                    inst.hasCustomMat = objDef.tiltToSlope;
                                                    if (objDef.tiltToSlope) {
                                                        inst.customMat[0] = mE11; inst.customMat[1] = mE12; inst.customMat[2] = mE13;
                                                        inst.customMat[3] = mE21; inst.customMat[4] = mE22; inst.customMat[5] = mE23;
                                                        inst.customMat[6] = mE31; inst.customMat[7] = mE32; inst.customMat[8] = mE33;
                                                        inst.customMat[9] = px;   inst.customMat[10] = py;  inst.customMat[11] = wz;
                                                    }

                                                    addToGroups(leaf.collections, inst);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        for (int tid = 0; tid < 3; ++tid) {
                            const auto* t = themes[tid];
                            if (t && layer_id < static_cast<int>(t->layers.size())) {
                                search_layers = true;
                                int gIdx = t->layers[layer_id].gridIndex;
                                if (gIdx >= 0 && gIdx < static_cast<int>(placementGrids.size())) {
                                    int count = static_cast<int>(placementGrids[gIdx]->cellElements[v8 & 0x1F][world_y & 0x1F].size());
                                    if (element_index < count) {
                                        search_elements = true;
                                    }
                                }
                            }
                        }
                        ++element_index;
                    } while (search_elements);
                    ++layer_id;
                } while (search_layers);
            }

            // Reverse collections to match retail prepending linked-list serialization order
            std::reverse(leaf.collections.begin(), leaf.collections.end());

            // Retail observing_maps primitive culling (0x13114f0)
            if (observingMaps) {
                for (auto& g : leaf.collections) {
                    std::vector<ClutterInstance> keptInstances;
                    std::map<int, std::vector<ClutterInstance>> byPal;
                    for (const auto& inst : g.instances) {
                        byPal[inst.palIndex].push_back(inst);
                    }
                    for (const auto& [palIdx, instList] : byPal) {
                        const auto& palEntry = builtPal.entries[palIdx];
                        const auto& mb = palBounds[palIdx];
                        BoundingBox meshBox;
                        meshBox.minCoord = {mb.boxMin[0], mb.boxMin[1], mb.boxMin[2]};
                        meshBox.maxCoord = {mb.boxMax[0], mb.boxMax[1], mb.boxMax[2]};

                        BoundingBox batchBox;
                        for (size_t i = 0; i < instList.size(); ++i) {
                            const auto& inst = instList[i];
                            float mat[12];
                            if (inst.hasCustomMat) {
                                std::memcpy(mat, inst.customMat, sizeof(mat));
                            } else {
                                float cosA = std::cos(inst.angle) * inst.scale;
                                float sinA = std::sin(inst.angle) * inst.scale;
                                mat[0] = cosA;  mat[1] = sinA;  mat[2] = 0.0f;
                                mat[3] = -sinA; mat[4] = cosA;  mat[5] = 0.0f;
                                mat[6] = 0.0f;  mat[7] = 0.0f;  mat[8] = inst.scale;
                                mat[9] = inst.x; mat[10] = inst.y; mat[11] = inst.z;
                            }
                            BoundingBox instBox;
                            initialiseFromOBB(instBox, meshBox, mat);
                            if (i == 0) batchBox = instBox;
                            else batchBox.merge(instBox);
                        }

                        float dist = findShortestDistanceToObservingMapSet(
                            batchBox.minCoord.x, batchBox.minCoord.y, batchBox.minCoord.z,
                            batchBox.maxCoord.x, batchBox.maxCoord.y, batchBox.maxCoord.z,
                            *observingMaps
                        );
                        if (palEntry.data.fadeEnd >= dist) {
                            keptInstances.insert(keptInstances.end(), instList.begin(), instList.end());
                        }
                    }
                    g.instances = std::move(keptInstances);
                }
                leaf.collections.erase(
                    std::remove_if(leaf.collections.begin(), leaf.collections.end(),
                                   [](const ClutterGroup& cg) { return cg.instances.empty(); }),
                    leaf.collections.end()
                );
            }
        }
    });
}
for (auto& w : workers) {
    w.join();
}
}



    // 4. Root QuadTree Element
    QuadTreeNode rootNode;
    rootNode.x = 0;
    rootNode.y = 0;
    rootNode.w = lev.width;
    rootNode.h = lev.height;
    rootNode.maxFadeDistance = 0.0f;
    rootNode.primitiveTypeMask = 0;

    // 5. Build Dynamic Clutter Palette from surviving clutter instances (exact retail behavior)
    struct DynamicPalette {
        std::vector<int> originalToMapped;
        std::vector<ClutterPaletteEntry> entries;

        int getOrAdd(int origIdx, const ClutterPaletteEntry& e) {
            if (origIdx >= static_cast<int>(originalToMapped.size())) {
                originalToMapped.resize(origIdx + 1, -1);
            }
            if (originalToMapped[origIdx] != -1) {
                return originalToMapped[origIdx];
            }
            int newIdx = static_cast<int>(entries.size());
            originalToMapped[origIdx] = newIdx;
            entries.push_back(e);
            return newIdx;
        }
    };

    DynamicPalette dynamicPalette;
    for (const auto& leaf : leaves) {
        for (const auto& g : leaf.collections) {
            if (g.instances.empty()) continue;
            std::vector<int> typeOrder;
            for (const auto& inst : g.instances) {
                if (std::find(typeOrder.begin(), typeOrder.end(), inst.palIndex) == typeOrder.end()) {
                    typeOrder.push_back(inst.palIndex);
                }
            }
            std::reverse(typeOrder.begin(), typeOrder.end());
            for (int palIdx : typeOrder) {
                dynamicPalette.getOrAdd(palIdx, builtPal.entries[palIdx]);
            }
        }
    }
    // 6. Serialize and compress leaf collections
    std::atomic<int> totalInstancesCount{0};
    std::atomic<int> totalBatchesCount{0};

    if (totalLeaves > 0) {
        std::atomic<int> nextLeaf{0};
        unsigned int numThreads = std::min(
            std::max(1u, std::thread::hardware_concurrency()),
            static_cast<unsigned int>(totalLeaves)
        );
        std::vector<std::thread> workers;
        workers.reserve(numThreads);
        for (unsigned int t = 0; t < numThreads; ++t) {
            workers.emplace_back([&]() {
                int localInstances = 0;
                int localBatches = 0;
                while (true) {
                    int leafIdx = nextLeaf.fetch_add(1);
                    if (leafIdx >= totalLeaves) break;
                    auto& leaf = leaves[leafIdx];

                    float leafMaxFade = 23.0f;
                    uint32_t leafPrimMask = 3;

                    for (const auto& g : leaf.collections) {
                        leafMaxFade = std::max(leafMaxFade, g.maxFadeDistance);
                        leafPrimMask |= g.primitiveTypeMask;
                        localInstances += static_cast<int>(g.instances.size());
                    }
                    leaf.maxFadeDistance = leafMaxFade;
                    leaf.primitiveTypeMask = leafPrimMask;

                    for (auto& g : leaf.collections) {
                        if (g.instances.empty()) continue;


            // Group instances by palette index preserving prepending linked-list type order
            std::vector<int> typeOrder;
            std::map<int, std::vector<ClutterInstance>> byPal;
            for (const auto& inst : g.instances) {
                if (byPal.find(inst.palIndex) == byPal.end()) {
                    typeOrder.push_back(inst.palIndex);
                }
                byPal[inst.palIndex].push_back(inst);
            }
            std::reverse(typeOrder.begin(), typeOrder.end());

            std::vector<uint8_t> uncompressedStream;
            int32_t typeCount = static_cast<int32_t>(typeOrder.size());
            uncompressedStream.resize(4);
            std::memcpy(uncompressedStream.data(), &typeCount, 4);

            std::vector<BoundingSphere> groupSpheres;

            for (int palIdx : typeOrder) {
                const auto& instList = byPal[palIdx];
                const auto& mb = palBounds[palIdx];
                BoundingBox meshBox;
                meshBox.minCoord = {mb.boxMin[0], mb.boxMin[1], mb.boxMin[2]};
                meshBox.maxCoord = {mb.boxMax[0], mb.boxMax[1], mb.boxMax[2]};

                    const auto& palEntry = builtPal.entries[palIdx];
                    int primType = palEntry.data.primitiveType;

                    if (primType == 0) {
                        // Sapling: each instance is its own primitive
                        int32_t renderUpdate = 0;
                        int32_t indexFromType = dynamicPalette.originalToMapped[palIdx];
                        int32_t primCount = static_cast<int32_t>(instList.size());

                        size_t curHdr = uncompressedStream.size();
                        uncompressedStream.resize(curHdr + 12);
                        std::memcpy(uncompressedStream.data() + curHdr + 0, &renderUpdate, 4);
                        std::memcpy(uncompressedStream.data() + curHdr + 4, &indexFromType, 4);
                        std::memcpy(uncompressedStream.data() + curHdr + 8, &primCount, 4);

                        for (const auto& inst : instList) {
                            int32_t pType = 0;
                            uncompressedStream.insert(uncompressedStream.end(),
                                                      reinterpret_cast<const uint8_t*>(&pType),
                                                      reinterpret_cast<const uint8_t*>(&pType) + 4);

                            float mat[12];
                            if (inst.hasCustomMat) {
                                std::memcpy(mat, inst.customMat, sizeof(mat));
                            } else {
                                float cosA = std::cos(inst.angle) * inst.scale;
                                float sinA = std::sin(inst.angle) * inst.scale;
                                mat[0] = cosA;  mat[1] = sinA;  mat[2] = 0.0f;
                                mat[3] = -sinA; mat[4] = cosA;  mat[5] = 0.0f;
                                mat[6] = 0.0f;  mat[7] = 0.0f;  mat[8] = inst.scale;
                                mat[9] = inst.x; mat[10] = inst.y; mat[11] = inst.z;
                            }

                            BoundingBox pBox;
                            initialiseFromOBB(pBox, meshBox, mat);

                            BoundingSphere pSphere = transformMeshSphere(mb, mat, inst.scale);
                            groupSpheres.push_back(pSphere);

                            // BoundingBox (24 bytes)
                            float boxData[6] = {pBox.minCoord.x, pBox.minCoord.y, pBox.minCoord.z,
                                                pBox.maxCoord.x, pBox.maxCoord.y, pBox.maxCoord.z};
                            uncompressedStream.insert(uncompressedStream.end(),
                                                      reinterpret_cast<const uint8_t*>(boxData),
                                                      reinterpret_cast<const uint8_t*>(boxData) + 24);

                            // BoundingSphere (16 bytes)
                            float sphData[4] = {pSphere.center.x, pSphere.center.y, pSphere.center.z, pSphere.radius};
                            uncompressedStream.insert(uncompressedStream.end(),
                                                      reinterpret_cast<const uint8_t*>(sphData),
                                                      reinterpret_cast<const uint8_t*>(sphData) + 16);

                            // Matrix (48 bytes: 12 floats)
                            uncompressedStream.insert(uncompressedStream.end(),
                                                      reinterpret_cast<const uint8_t*>(mat),
                                                      reinterpret_cast<const uint8_t*>(mat) + 48);

                            // Scale (4 bytes)
                            uncompressedStream.insert(uncompressedStream.end(),
                                                      reinterpret_cast<const uint8_t*>(&inst.scale),
                                                      reinterpret_cast<const uint8_t*>(&inst.scale) + 4);
                        }
                    } else if (primType == 2) {
                        // Tree: MeshZSpriteBatch
                        int32_t renderUpdate = 1;
                        int32_t indexFromType = dynamicPalette.originalToMapped[palIdx];
                        int32_t primCount = 1;
                        int32_t pType = 2;

                        size_t curHdr = uncompressedStream.size();
                        uncompressedStream.resize(curHdr + 16);
                        std::memcpy(uncompressedStream.data() + curHdr + 0, &renderUpdate, 4);
                        std::memcpy(uncompressedStream.data() + curHdr + 4, &indexFromType, 4);
                        std::memcpy(uncompressedStream.data() + curHdr + 8, &primCount, 4);
                        std::memcpy(uncompressedStream.data() + curHdr + 12, &pType, 4);

                        BoundingBox pBox;
                        std::vector<BoundingSphere> batchSpheres;
                        batchSpheres.reserve(instList.size());
                        std::vector<std::array<float, 12>> batchMats(instList.size());

                        for (size_t i = 0; i < instList.size(); ++i) {
                            const auto& inst = instList[i];
                            float mat[12];
                            if (inst.hasCustomMat) {
                                std::memcpy(mat, inst.customMat, sizeof(mat));
                            } else {
                                float cosA = std::cos(inst.angle) * inst.scale;
                                float sinA = std::sin(inst.angle) * inst.scale;
                                mat[0] = cosA;  mat[1] = sinA;  mat[2] = 0.0f;
                                mat[3] = -sinA; mat[4] = cosA;  mat[5] = 0.0f;
                                mat[6] = 0.0f;  mat[7] = 0.0f;  mat[8] = inst.scale;
                                mat[9] = inst.x; mat[10] = inst.y; mat[11] = inst.z;
                            }
                            std::memcpy(batchMats[i].data(), mat, 48);

                            BoundingBox bbox;
                            initialiseFromOBB(bbox, meshBox, mat);
                            if (i == 0) {
                                pBox = bbox;
                            } else {
                                pBox.merge(bbox);
                            }

                            BoundingSphere objSph = transformMeshSphere(mb, mat, inst.scale);
                            batchSpheres.push_back(objSph);
                        }
                        BoundingSphere pSphere = BoundingSphere::merge(batchSpheres);
                        groupSpheres.push_back(pSphere);

                        // BoundingBox (24 bytes)
                        float boxData[6] = {pBox.minCoord.x, pBox.minCoord.y, pBox.minCoord.z,
                                            pBox.maxCoord.x, pBox.maxCoord.y, pBox.maxCoord.z};
                        uncompressedStream.insert(uncompressedStream.end(),
                                                  reinterpret_cast<const uint8_t*>(boxData),
                                                  reinterpret_cast<const uint8_t*>(boxData) + 24);

                        // BoundingSphere (16 bytes)
                        float sphData[4] = {pSphere.center.x, pSphere.center.y, pSphere.center.z, pSphere.radius};
                        uncompressedStream.insert(uncompressedStream.end(),
                                                  reinterpret_cast<const uint8_t*>(sphData),
                                                  reinterpret_cast<const uint8_t*>(sphData) + 16);

                        // ObjectCount (4 bytes)
                        int32_t objCnt = static_cast<int32_t>(instList.size());
                        uncompressedStream.insert(uncompressedStream.end(),
                                                  reinterpret_cast<const uint8_t*>(&objCnt),
                                                  reinterpret_cast<const uint8_t*>(&objCnt) + 4);

                        // Objects (68 bytes each: 48 mat + 4 scale + 16 bsphere)
                        for (size_t i = 0; i < instList.size(); ++i) {
                            const auto& inst = instList[i];
                            uncompressedStream.insert(uncompressedStream.end(),
                                                      reinterpret_cast<const uint8_t*>(batchMats[i].data()),
                                                      reinterpret_cast<const uint8_t*>(batchMats[i].data()) + 48);

                            uncompressedStream.insert(uncompressedStream.end(),
                                                      reinterpret_cast<const uint8_t*>(&inst.scale),
                                                      reinterpret_cast<const uint8_t*>(&inst.scale) + 4);

                            float objSph[4] = {batchSpheres[i].center.x, batchSpheres[i].center.y, batchSpheres[i].center.z, batchSpheres[i].radius};
                            uncompressedStream.insert(uncompressedStream.end(),
                                                      reinterpret_cast<const uint8_t*>(objSph),
                                                      reinterpret_cast<const uint8_t*>(objSph) + 16);
                        }

                        // ZSprite entries (16 bytes each: x, y, z, radius)
                        for (size_t i = 0; i < instList.size(); ++i) {
                            float zs[4] = {batchMats[i][9], batchMats[i][10], batchMats[i][11], batchSpheres[i].radius};
                            uncompressedStream.insert(uncompressedStream.end(),
                                                      reinterpret_cast<const uint8_t*>(zs),
                                                      reinterpret_cast<const uint8_t*>(zs) + 16);
                        }
                    } else {
                        // Grass: RepeatedMesh
                        std::vector<std::vector<ClutterInstance>> batches;
                        for (size_t i = 0; i < instList.size(); i += 32) {
                            size_t endIdx = std::min(i + 32, instList.size());
                            batches.emplace_back(instList.begin() + i, instList.begin() + endIdx);
                        }

                        int32_t renderUpdate = 0;
                        int32_t indexFromType = dynamicPalette.originalToMapped[palIdx];
                        int32_t primCount = static_cast<int32_t>(batches.size());

                        size_t curHdr = uncompressedStream.size();
                        uncompressedStream.resize(curHdr + 12);
                        std::memcpy(uncompressedStream.data() + curHdr + 0, &renderUpdate, 4);
                        std::memcpy(uncompressedStream.data() + curHdr + 4, &indexFromType, 4);
                        std::memcpy(uncompressedStream.data() + curHdr + 8, &primCount, 4);

                        for (const auto& batch : batches) {
                            int32_t pType = 1;
                            uncompressedStream.insert(uncompressedStream.end(),
                                                      reinterpret_cast<const uint8_t*>(&pType),
                                                      reinterpret_cast<const uint8_t*>(&pType) + 4);

                            BoundingBox pBox;
                            std::vector<BoundingSphere> batchSpheres;
                            batchSpheres.reserve(batch.size());
                            float maxScale = 0.0f;

                            for (size_t i = 0; i < batch.size(); ++i) {
                                const auto& inst = batch[i];
                                if (inst.scale > maxScale) maxScale = inst.scale;

                                float cosA = std::cos(inst.angle) * inst.scale;
                                float sinA = std::sin(inst.angle) * inst.scale;
                                float mat[12] = {
                                    cosA, sinA, 0.0f,
                                    -sinA, cosA, 0.0f,
                                    0.0f, 0.0f, inst.scale,
                                    inst.x, inst.y, inst.z
                                };

                                BoundingBox bbox;
                                initialiseFromOBB(bbox, meshBox, mat);
                                if (i == 0) {
                                    pBox = bbox;
                                } else {
                                    pBox.merge(bbox);
                                }

                                BoundingSphere objSph = transformMeshSphere(mb, mat, inst.scale);
                                batchSpheres.push_back(objSph);
                            }
                            BoundingSphere pSphere = BoundingSphere::merge(batchSpheres);
                            groupSpheres.push_back(pSphere);

                            // 1. BoundingBox (24 bytes)
                            float boxData[6] = {pBox.minCoord.x, pBox.minCoord.y, pBox.minCoord.z,
                                                pBox.maxCoord.x, pBox.maxCoord.y, pBox.maxCoord.z};
                            uncompressedStream.insert(uncompressedStream.end(),
                                                      reinterpret_cast<const uint8_t*>(boxData),
                                                      reinterpret_cast<const uint8_t*>(boxData) + 24);

                            // 2. BoundingSphere (16 bytes)
                            float sphData[4] = {pSphere.center.x, pSphere.center.y, pSphere.center.z, pSphere.radius};
                            uncompressedStream.insert(uncompressedStream.end(),
                                                      reinterpret_cast<const uint8_t*>(sphData),
                                                      reinterpret_cast<const uint8_t*>(sphData) + 16);

                            // 3. ObjectCount (4 bytes)
                            int32_t objCnt = static_cast<int32_t>(batch.size());
                            uncompressedStream.insert(uncompressedStream.end(),
                                                      reinterpret_cast<const uint8_t*>(&objCnt),
                                                      reinterpret_cast<const uint8_t*>(&objCnt) + 4);

                            // 4. MaxObjectScale (4 bytes)
                            uncompressedStream.insert(uncompressedStream.end(),
                                                      reinterpret_cast<const uint8_t*>(&maxScale),
                                                      reinterpret_cast<const uint8_t*>(&maxScale) + 4);

                            // Build subsections and object remap table
                            std::vector<SubsectionSphere> instanceSpheres;
                            instanceSpheres.reserve(batch.size());
                            for (size_t i = 0; i < batch.size(); ++i) {
                                instanceSpheres.push_back({
                                    batchSpheres[i].center.x,
                                    batchSpheres[i].center.y,
                                    batchSpheres[i].center.z,
                                    batchSpheres[i].radius
                                });
                            }

                            SubsectionTable subsections = buildSubSectionsAndObjectRemapTable(instanceSpheres, 4);
                            const auto& order = subsections.remap;

                            // 5. ObjectMatricies (16 * count bytes)
                            for (size_t i = 0; i < batch.size(); ++i) {
                                const auto& inst = batch[order[i]];
                                float cosA = std::cos(inst.angle) * inst.scale;
                                float sinA = std::sin(inst.angle) * inst.scale;
                                float mat[4] = {cosA, sinA, 0.0f, 0.0f};
                                uncompressedStream.insert(uncompressedStream.end(),
                                                          reinterpret_cast<const uint8_t*>(mat),
                                                          reinterpret_cast<const uint8_t*>(mat) + 16);
                            }

                            // 6. ObjectOffsets (16 * count bytes)
                            for (size_t i = 0; i < batch.size(); ++i) {
                                const auto& inst = batch[order[i]];
                                float off[4] = {inst.x, inst.y, inst.z, inst.scale};
                                uncompressedStream.insert(uncompressedStream.end(),
                                                          reinterpret_cast<const uint8_t*>(off),
                                                          reinterpret_cast<const uint8_t*>(off) + 16);
                            }

                            // 7. LandscapeNormalArray (EBOOL 1 byte + normals)
                            uint8_t hasNormals = 1;
                            uncompressedStream.push_back(hasNormals);
                            size_t paddedCount = (batch.size() + 3) & ~3;
                            for (int axis = 0; axis < 3; ++axis) {
                                for (size_t i = 0; i < batch.size(); ++i) {
                                    const auto& inst = batch[order[i]];
                                    float val = (axis == 0) ? inst.nx : ((axis == 1) ? inst.ny : inst.nz);
                                    uncompressedStream.insert(uncompressedStream.end(),
                                                              reinterpret_cast<const uint8_t*>(&val),
                                                              reinterpret_cast<const uint8_t*>(&val) + 4);
                                }
                                uint32_t pad = 0u;
                                for (size_t i = batch.size(); i < paddedCount; ++i) {
                                    uncompressedStream.insert(uncompressedStream.end(),
                                                              reinterpret_cast<const uint8_t*>(&pad),
                                                              reinterpret_cast<const uint8_t*>(&pad) + 4);
                                }
                            }

                            // 8. WindDelayArray (EBOOL 1 byte + delays if enabled)
                            const auto& entry = palEntry;
                            if (entry.data.hasWindSkew != 0) {
                                uint8_t hasWind = 1;
                                uncompressedStream.push_back(hasWind);
                                for (size_t i = 0; i < batch.size(); ++i) {
                                    float pos = batch[order[i]].x;
                                    float pos_4 = batch[order[i]].y;
                                    float s1 = std::sin(pos);
                                    float s2 = std::sin(pos_4);
                                    float s3 = std::sin(pos_4 * 0.1333f - pos * 0.0333f);
                                    float avg = (s1 + s2 + s3) / 3.0f;
                                    int delay = static_cast<int>((avg * 0.5f + 0.5f) * 64.0f);
                                    delay = std::clamp(delay, 0, 63);
                                    uncompressedStream.push_back(static_cast<uint8_t>(delay));
                                }
                            } else {
                                uint8_t hasWind = 0;
                                uncompressedStream.push_back(hasWind);
                            }

                            // 9. SubSections
                            if (subsections.present) {
                                uint8_t hasSubSec = 1;
                                uncompressedStream.push_back(hasSubSec);
                                int32_t subCount = static_cast<int32_t>(subsections.elements.size());
                                uncompressedStream.insert(uncompressedStream.end(),
                                                          reinterpret_cast<const uint8_t*>(&subCount),
                                                          reinterpret_cast<const uint8_t*>(&subCount) + 4);

                                for (const auto& e : subsections.elements) {
                                    for (int q = 0; q < 4; ++q) {
                                        uncompressedStream.insert(uncompressedStream.end(),
                                            reinterpret_cast<const uint8_t*>(&e.centreX[q]),
                                            reinterpret_cast<const uint8_t*>(&e.centreX[q]) + 4);
                                    }
                                    for (int q = 0; q < 4; ++q) {
                                        uncompressedStream.insert(uncompressedStream.end(),
                                            reinterpret_cast<const uint8_t*>(&e.centreY[q]),
                                            reinterpret_cast<const uint8_t*>(&e.centreY[q]) + 4);
                                    }
                                    for (int q = 0; q < 4; ++q) {
                                        uncompressedStream.insert(uncompressedStream.end(),
                                            reinterpret_cast<const uint8_t*>(&e.centreZ[q]),
                                            reinterpret_cast<const uint8_t*>(&e.centreZ[q]) + 4);
                                    }
                                    for (int q = 0; q < 4; ++q) {
                                        uncompressedStream.insert(uncompressedStream.end(),
                                            reinterpret_cast<const uint8_t*>(&e.radius[q]),
                                            reinterpret_cast<const uint8_t*>(&e.radius[q]) + 4);
                                    }
                                    for (int q = 0; q < 4; ++q) uncompressedStream.push_back(e.count[q]);
                                    for (int q = 0; q < 4; ++q) uncompressedStream.push_back(e.startIndex[q]);
                                    for (int q = 0; q < 4; ++q) uncompressedStream.push_back(e.childOffset[q]);
                                    for (int q = 0; q < 4; ++q) uncompressedStream.push_back(e.tail[q]);
                                }
                            } else {
                                uint8_t hasSubSec = 0;
                                uncompressedStream.push_back(hasSubSec);
                            }

                            localBatches++;
                        }
                    }
                }

                g.sphere = BoundingSphere::merge(groupSpheres);
                g.uncompressedSize = uncompressedStream.size();

                // Compress using albion::lzo1x optimal compressor
                auto comp = albion::lzo1x::compress(uncompressedStream.data(), uncompressedStream.size());
                uint32_t uLen = static_cast<uint32_t>(uncompressedStream.size());
                uint32_t cLen = static_cast<uint32_t>(comp.size());

                g.compressedFrame.resize(8 + comp.size());
                std::memcpy(g.compressedFrame.data() + 0, &uLen, 4);
                std::memcpy(g.compressedFrame.data() + 4, &cLen, 4);
                std::memcpy(g.compressedFrame.data() + 8, comp.data(), comp.size());
            }
        }
        totalInstancesCount.fetch_add(localInstances);
        totalBatchesCount.fetch_add(localBatches);
    });
}
for (auto& w : workers) {
    w.join();
}
}

    // Step 6b: Sequential insertion into quadtree
    for (auto& leaf : leaves) {
        if (!leaf.collections.empty()) {
            auto leafNode = std::make_unique<QuadTreeNode>();
            leafNode->x = leaf.x;
            leafNode->y = leaf.y;
            leafNode->w = leaf.w;
            leafNode->h = leaf.h;
            leafNode->collections = std::move(leaf.collections);
            leafNode->maxFadeDistance = leaf.maxFadeDistance;
            leafNode->primitiveTypeMask = leaf.primitiveTypeMask;
            std::vector<BoundingSphere> colSpheres;
            for (const auto& col : leafNode->collections) {
                if (col.sphere.radius > 0.0f) colSpheres.push_back(col.sphere);
            }
            if (!colSpheres.empty()) leafNode->sphere = BoundingSphere::merge(colSpheres);

            insertOrUpdateDynamicArea(&rootNode, leaf.x, leaf.y, std::move(leafNode));
        }
    }


    // 6. Assign File Blocks according to MinimumFileBlockSize (32768)
    std::set<QuadTreeNode*> blockMap;
    assignFileBlocks(&rootNode, blockMap, true);

    std::cout << "LocalDetail: blockMap has " << blockMap.size() << " file blocks (expected 63)" << std::endl;
    std::vector<int> instCounts(builtPal.entries.size(), 0);
    for (const auto& leaf : leaves) {
        for (const auto& g : leaf.collections) {
            for (const auto& inst : g.instances) {
                if (inst.palIndex >= 0 && inst.palIndex < static_cast<int>(instCounts.size())) {
                    instCounts[inst.palIndex]++;
                }
            }
        }
    }
    for (size_t i = 0; i < builtPal.entries.size(); ++i) {
        std::cout << "  Pal[" << i << "] " << builtPal.entries[i].name 
                  << " (mesh=" << builtPal.entries[i].data.meshBankIndex << "): " 
                  << instCounts[i] << " instances\n";
    }

    // 7. Save Tree using retail 2-pass algorithm
    size_t rootBlockFilePos = alignUp(initialPayloadOffset, 2048);
    res.payloadStartOffset = static_cast<uint32_t>(rootBlockFilePos);

    std::vector<uint8_t> payloadStream;
    payloadStream.reserve(5 * 1024 * 1024);

    saveTree(&rootNode, payloadStream, rootBlockFilePos, blockMap);

    // 8. Build Common Header Payload (Exact 945 bytes)
    // - 44 bytes: Root QuadTreeElementFileHeader
    // - 1 byte: Retail EBOOL (1)
    // - 4 bytes: Palette Count (14)
    // - 896 bytes: Exact 14 RetailPaletteEntries from reference
    std::vector<uint8_t> headerPayload;
    headerPayload.reserve(945);

    QuadTreeElementFileHeader rootHdr;
    rootHdr.sphereX = rootNode.sphere.center.x;
    rootHdr.sphereY = rootNode.sphere.center.y;
    rootHdr.sphereZ = rootNode.sphere.center.z;
    rootHdr.sphereR = rootNode.sphere.radius;
    rootHdr.maxFadeDistance = rootNode.maxFadeDistance;
    rootHdr.primitiveTypeMask = rootNode.primitiveTypeMask;
    rootHdr.fileBlockPos = rootNode.fileBlockPos;
    rootHdr.fileBlockSize = rootNode.fileBlockSize;
    rootHdr.offsetIntoFileBlock = rootNode.offsetIntoFileBlock;
    rootHdr.mapX = 0;
    rootHdr.mapY = 0;
    rootHdr.width = static_cast<uint16_t>(lev.width);
    rootHdr.height = static_cast<uint16_t>(lev.height);

    headerPayload.insert(headerPayload.end(),
                         reinterpret_cast<const uint8_t*>(&rootHdr),
                         reinterpret_cast<const uint8_t*>(&rootHdr) + 44);

    uint8_t retailFlag = 1;
    headerPayload.push_back(retailFlag);

    int32_t palCount = static_cast<int32_t>(dynamicPalette.entries.size());
    headerPayload.insert(headerPayload.end(),
                         reinterpret_cast<const uint8_t*>(&palCount),
                         reinterpret_cast<const uint8_t*>(&palCount) + 4);

    for (const auto& entry : dynamicPalette.entries) {
        headerPayload.insert(headerPayload.end(),
                             reinterpret_cast<const uint8_t*>(&entry.data),
                             reinterpret_cast<const uint8_t*>(&entry.data) + 64);
    }

    res.hasData = true;
    res.headerBytes = std::move(headerPayload);
    res.payloadBytes = std::move(payloadStream);
    res.totalInstances = totalInstancesCount.load();
    res.totalBatches = totalBatchesCount.load();


    return res;
}

} // namespace egostb
