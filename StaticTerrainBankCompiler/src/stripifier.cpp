#include "egostb/stripifier.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace egostb {

namespace {

constexpr int kNext3[3] = {1, 2, 0};
constexpr int kPrev3[3] = {2, 0, 1};

struct VertexPolyCSR {
    uint32_t vert_count = 0;
    std::vector<uint32_t> poly_offsets;
    std::vector<int32_t> flat_polys;
    std::vector<uint32_t> link_offsets;
    std::vector<uint16_t> flat_links;
    std::vector<uint16_t> active_verts;
    std::vector<uint32_t> write_pos;
};

struct RingStampEntry {
    uint32_t gen = 0;
    uint16_t ring_id = 0xFFFF;
};

struct CacheStampEntry {
    uint32_t gen = 0;
    int32_t stamp = -1;
};

struct StripifierScratch {
    // Island extraction stamps & stacks
    std::vector<int32_t> poly_stack;
    std::vector<uint16_t> vertex_stack;
    std::vector<uint32_t> island_poly_stamp;
    uint32_t island_poly_gen = 0;
    std::vector<uint32_t> island_vert_stamp;
    uint32_t island_vert_gen = 0;

    // Ring building scratch & stamps
    std::vector<RingStampEntry> ring_entries;
    uint32_t ring_gen = 0;
    std::vector<uint32_t> ordered_stamp;
    uint32_t ordered_gen = 0;
    std::vector<uint16_t> prev_ring_list;
    std::vector<uint16_t> ring_list;
    std::vector<uint16_t> flat_rings;
    std::vector<uint32_t> ring_offsets;

    // Cache-optimized vertex list stamps
    std::vector<CacheStampEntry> cache_stamps;
    uint32_t cache_gen = 0;

    // Section mask & scoring stamps
    std::vector<uint32_t> poly_proc_stamp;
    uint32_t poly_proc_gen = 0;
    std::vector<CacheStampEntry> sec_vert_stamps;
    uint32_t sec_vert_gen = 0;
    std::vector<uint32_t> poly_mask_stamp;
    uint32_t poly_mask_gen = 0;
    std::vector<uint16_t> cand_vertex_list;

    // FillInPolygons / FillInTriStrip stamps
    std::vector<uint32_t> poly_added_stamp;
    uint32_t poly_added_gen = 0;
    std::vector<uint32_t> vert_cached_stamp;
    uint32_t vert_cached_gen = 0;
    std::vector<CacheStampEntry> vert_pos_stamps;
    uint32_t vert_pos_gen = 0;

    void ensureSizes(uint32_t vert_count, uint32_t poly_count) {
        if (island_poly_stamp.size() < poly_count) island_poly_stamp.assign(poly_count, 0);
        if (poly_proc_stamp.size() < poly_count) poly_proc_stamp.assign(poly_count, 0);
        if (poly_mask_stamp.size() < poly_count) poly_mask_stamp.assign(poly_count, 0);
        if (poly_added_stamp.size() < poly_count) poly_added_stamp.assign(poly_count, 0);

        if (island_vert_stamp.size() < vert_count) island_vert_stamp.assign(vert_count, 0);
        if (ring_entries.size() < vert_count) ring_entries.resize(vert_count);
        if (ordered_stamp.size() < vert_count) ordered_stamp.assign(vert_count, 0);
        if (cache_stamps.size() < vert_count) cache_stamps.resize(vert_count);
        if (sec_vert_stamps.size() < vert_count) sec_vert_stamps.resize(vert_count);
        if (vert_cached_stamp.size() < vert_count) vert_cached_stamp.assign(vert_count, 0);
        if (vert_pos_stamps.size() < vert_count) vert_pos_stamps.resize(vert_count);
    }

    uint32_t nextGen(uint32_t& gen, std::vector<uint32_t>& vec) {
        if (++gen == 0) {
            std::fill(vec.begin(), vec.end(), 0u);
            gen = 1;
        }
        return gen;
    }

    uint32_t nextGen(uint32_t& gen, std::vector<RingStampEntry>& vec) {
        if (++gen == 0) {
            for (auto& e : vec) e.gen = 0;
            gen = 1;
        }
        return gen;
    }

    uint32_t nextGen(uint32_t& gen, std::vector<CacheStampEntry>& vec) {
        if (++gen == 0) {
            for (auto& e : vec) e.gen = 0;
            gen = 1;
        }
        return gen;
    }
};

void buildCSR(
    const std::vector<uint16_t>& poly_buffer,
    VertexPolyCSR& csr,
    bool build_links)
{
    const uint32_t poly_count = static_cast<uint32_t>(poly_buffer.size() / 3);
    if (poly_count == 0) {
        csr.vert_count = 0;
        csr.poly_offsets.assign(1, 0);
        csr.flat_polys.clear();
        csr.link_offsets.assign(1, 0);
        csr.flat_links.clear();
        csr.active_verts.clear();
        return;
    }

    uint16_t max_v = 0;
    for (uint16_t v : poly_buffer) {
        if (v > max_v) max_v = v;
    }
    const uint32_t vert_count = static_cast<uint32_t>(max_v) + 1;
    csr.vert_count = vert_count;

    csr.poly_offsets.assign(vert_count + 1, 0);
    for (uint32_t i = 0; i < poly_count * 3; ++i) {
        ++csr.poly_offsets[poly_buffer[i] + 1];
    }

    csr.active_verts.clear();
    uint32_t running = 0;
    csr.write_pos.resize(vert_count);
    for (uint32_t v = 0; v < vert_count; ++v) {
        const uint32_t cnt = csr.poly_offsets[v + 1];
        csr.poly_offsets[v] = running;
        csr.write_pos[v] = running;
        if (cnt > 0) {
            csr.active_verts.push_back(static_cast<uint16_t>(v));
        }
        running += cnt;
    }
    csr.poly_offsets[vert_count] = running;

    csr.flat_polys.resize(running);
    for (uint32_t pi = 0; pi < poly_count; ++pi) {
        const uint16_t* tri = &poly_buffer[3 * pi];
        for (int c = 0; c < 3; ++c) {
            const uint16_t v = tri[c];
            csr.flat_polys[csr.write_pos[v]++] = static_cast<int32_t>(pi);
        }
    }

    if (!build_links) return;

    csr.link_offsets.resize(vert_count + 1);
    csr.flat_links.clear();
    csr.flat_links.reserve(running * 2);

    uint32_t cursor = 0;
    for (uint16_t vta : csr.active_verts) {
        while (cursor <= vta) {
            csr.link_offsets[cursor++] = static_cast<uint32_t>(csr.flat_links.size());
        }
        const uint32_t link_start = static_cast<uint32_t>(csr.flat_links.size());
        const uint32_t p_begin = csr.poly_offsets[vta];
        const uint32_t p_end = csr.poly_offsets[vta + 1];
        for (uint32_t pi_idx = p_begin; pi_idx < p_end; ++pi_idx) {
            const uint16_t* tri = &poly_buffer[3 * csr.flat_polys[pi_idx]];
            for (int c = 0; c < 3; ++c) {
                const uint16_t neighbor = tri[c];
                if (neighbor != vta) {
                    bool found = false;
                    for (uint32_t li = link_start; li < csr.flat_links.size(); ++li) {
                        if (csr.flat_links[li] == neighbor) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        csr.flat_links.push_back(neighbor);
                    }
                }
            }
        }
    }
    while (cursor <= vert_count) {
        csr.link_offsets[cursor++] = static_cast<uint32_t>(csr.flat_links.size());
    }
}

void getMeshIslandSubsectionFast(
    const std::vector<uint16_t>& poly_buffer,
    const std::vector<uint8_t>& poly_mask,
    int& first_unprocessed_poly,
    const VertexPolyCSR& csr,
    StripifierScratch& scratch,
    std::vector<uint16_t>& subsection,
    std::vector<int32_t>& sub_section_to_global_poly_map)
{
    subsection.clear();
    sub_section_to_global_poly_map.clear();
    const int poly_count = static_cast<int>(poly_buffer.size() / 3);

    int start_poly = first_unprocessed_poly;
    for (; start_poly < poly_count; ++start_poly) {
        if (poly_mask[start_poly]) break;
    }
    first_unprocessed_poly = start_poly;
    if (start_poly >= poly_count) return;

    const uint32_t poly_gen = scratch.nextGen(scratch.island_poly_gen, scratch.island_poly_stamp);
    const uint32_t vert_gen = scratch.nextGen(scratch.island_vert_gen, scratch.island_vert_stamp);

    scratch.poly_stack.clear();
    scratch.vertex_stack.clear();

    scratch.poly_stack.push_back(start_poly);
    scratch.island_poly_stamp[start_poly] = poly_gen;

    while (!scratch.poly_stack.empty() || !scratch.vertex_stack.empty()) {
        while (!scratch.poly_stack.empty()) {
            const int32_t p = scratch.poly_stack.back();
            scratch.poly_stack.pop_back();

            sub_section_to_global_poly_map.push_back(p);

            const uint16_t* tri = &poly_buffer[3 * p];
            for (int c = 0; c < 3; ++c) {
                const uint16_t v = tri[c];
                subsection.push_back(v);
                if (v < csr.vert_count && scratch.island_vert_stamp[v] != vert_gen) {
                    scratch.island_vert_stamp[v] = vert_gen;
                    scratch.vertex_stack.push_back(v);
                }
            }
        }

        while (!scratch.vertex_stack.empty()) {
            const uint16_t v = scratch.vertex_stack.back();
            scratch.vertex_stack.pop_back();

            if (v < csr.vert_count) {
                const uint32_t p_begin = csr.poly_offsets[v];
                const uint32_t p_end = csr.poly_offsets[v + 1];
                for (uint32_t pi = p_begin; pi < p_end; ++pi) {
                    const int32_t p = csr.flat_polys[pi];
                    if (p >= 0 && p < poly_count && scratch.island_poly_stamp[p] != poly_gen && poly_mask[p]) {
                        scratch.poly_stack.push_back(p);
                        scratch.island_poly_stamp[p] = poly_gen;
                    }
                }
            }
        }
    }
}

void buildRingsFast(
    int32_t start_vert,
    const std::vector<uint16_t>& poly_buffer,
    const VertexPolyCSR& csr,
    StripifierScratch& scratch)
{
    const uint32_t vertex_count = csr.vert_count;
    const uint32_t ring_gen = scratch.nextGen(scratch.ring_gen, scratch.ring_entries);

    scratch.flat_rings.clear();
    scratch.ring_offsets.clear();
    scratch.prev_ring_list.clear();

    scratch.prev_ring_list.push_back(static_cast<uint16_t>(start_vert));
    scratch.ring_entries[start_vert] = {ring_gen, 0};

    scratch.ring_offsets.push_back(0);
    scratch.flat_rings.push_back(static_cast<uint16_t>(start_vert));
    scratch.ring_offsets.push_back(1);

    int ring_id = 1;
    while (!scratch.prev_ring_list.empty()) {
        scratch.ring_list.clear();
        for (uint16_t v : scratch.prev_ring_list) {
            if (v < vertex_count) {
                const uint32_t l_begin = csr.link_offsets[v];
                const uint32_t l_end = csr.link_offsets[v + 1];
                for (uint32_t li = l_begin; li < l_end; ++li) {
                    const uint16_t neighbor = csr.flat_links[li];
                    if (neighbor < vertex_count && scratch.ring_entries[neighbor].gen != ring_gen) {
                        scratch.ring_entries[neighbor] = {ring_gen, static_cast<uint16_t>(ring_id)};
                        scratch.ring_list.push_back(neighbor);
                    }
                }
            }
        }
        if (scratch.ring_list.empty()) break;

        const uint32_t ordered_gen = scratch.nextGen(scratch.ordered_gen, scratch.ordered_stamp);
        const uint32_t ordered_start = static_cast<uint32_t>(scratch.flat_rings.size());
        const uint32_t prev_ring_off = scratch.ring_offsets[ring_id - 1];
        uint16_t v37 = scratch.ring_list[0];
        int prev_ring_vert_pos = 0;

        while (true) {
            int verts_searched = 0;
            uint16_t current_prev_ring_link = scratch.flat_rings[prev_ring_off + prev_ring_vert_pos];
            uint16_t current_vert = v37;

            // Search 1:
            while (true) {
                bool found = false;
                uint16_t candidate = 0;
                if (v37 < vertex_count) {
                    const uint32_t p_begin = csr.poly_offsets[v37];
                    const uint32_t p_end = csr.poly_offsets[v37 + 1];
                    for (uint32_t pi = p_begin; pi < p_end; ++pi) {
                        const uint16_t* tri = &poly_buffer[3 * csr.flat_polys[pi]];
                        int v42 = 0;
                        for (; v42 < 3; ++v42) {
                            if (tri[v42] == v37) break;
                        }
                        if (v42 < 3) {
                            const int v44 = kNext3[v42];
                            const int v45 = kNext3[v44];
                            if (tri[v44] == current_prev_ring_link) {
                                candidate = tri[v45];
                                found = true;
                                break;
                            }
                        }
                    }
                }
                if (found) {
                    if (candidate < vertex_count &&
                        scratch.ring_entries[candidate].gen == ring_gen &&
                        scratch.ring_entries[candidate].ring_id == ring_id &&
                        scratch.ordered_stamp[candidate] != ordered_gen) {
                        v37 = candidate;
                        current_vert = v37;
                        ++verts_searched;
                        if (verts_searched <= static_cast<int>(scratch.ring_list.size())) {
                            continue;
                        }
                    }
                }
                break;
            }

            scratch.flat_rings.push_back(v37);
            scratch.ordered_stamp[v37] = ordered_gen;

            const uint32_t ordered_size = static_cast<uint32_t>(scratch.flat_rings.size()) - ordered_start;
            if (ordered_size == scratch.ring_list.size()) {
                break;
            }

            // Search 2:
            uint16_t v56 = 0;
            bool found_v56 = false;
            while (!found_v56) {
                if (current_vert < vertex_count) {
                    const uint32_t p_begin = csr.poly_offsets[current_vert];
                    const uint32_t p_end = csr.poly_offsets[current_vert + 1];
                    for (uint32_t pi = p_begin; pi < p_end; ++pi) {
                        const uint16_t* tri = &poly_buffer[3 * csr.flat_polys[pi]];
                        int v52 = 0;
                        for (; v52 < 3; ++v52) {
                            if (tri[v52] == current_vert) break;
                        }
                        if (v52 < 3) {
                            const int v54 = kPrev3[v52];
                            if (tri[v54] == current_prev_ring_link) {
                                const uint16_t candidate = tri[kPrev3[v54]];
                                if (candidate < vertex_count &&
                                    scratch.ring_entries[candidate].gen == ring_gen &&
                                    scratch.ring_entries[candidate].ring_id == ring_id &&
                                    scratch.ordered_stamp[candidate] != ordered_gen) {
                                    v56 = candidate;
                                    found_v56 = true;
                                    break;
                                }
                            }
                        }
                    }
                }
                if (found_v56) break;

                if (current_prev_ring_link < vertex_count) {
                    const uint32_t l_begin = csr.link_offsets[current_prev_ring_link];
                    const uint32_t l_end = csr.link_offsets[current_prev_ring_link + 1];
                    for (uint32_t li = l_begin; li < l_end; ++li) {
                        const uint16_t neighbor = csr.flat_links[li];
                        if (neighbor < vertex_count &&
                            scratch.ring_entries[neighbor].gen == ring_gen &&
                            scratch.ring_entries[neighbor].ring_id == ring_id &&
                            scratch.ordered_stamp[neighbor] != ordered_gen) {
                            v56 = neighbor;
                            found_v56 = true;
                            break;
                        }
                    }
                }
                if (found_v56) break;

                ++prev_ring_vert_pos;
                current_prev_ring_link = scratch.flat_rings[prev_ring_off + prev_ring_vert_pos];
            }

            v37 = v56;
        }

        const uint32_t ordered_end = static_cast<uint32_t>(scratch.flat_rings.size());
        scratch.ring_offsets.push_back(ordered_end);
        scratch.prev_ring_list.assign(scratch.flat_rings.begin() + ordered_start, scratch.flat_rings.begin() + ordered_end);
        ++ring_id;
    }
}

void buildCacheOptimizedVertexListFast(
    const VertexPolyCSR& csr,
    StripifierScratch& scratch,
    std::vector<uint16_t>& dest_vert_list)
{
    (void)csr;
    dest_vert_list.clear();
    const uint32_t cache_gen = scratch.nextGen(scratch.cache_gen, scratch.cache_stamps);
    const uint32_t ring_gen = scratch.ring_gen;
    int current_cache_stamp = 0;

    const size_t num_rings = scratch.ring_offsets.size() - 1;
    for (size_t ring_idx = 0; ring_idx < num_rings; ++ring_idx) {
        const uint32_t r_begin = scratch.ring_offsets[ring_idx];
        const uint32_t r_end = scratch.ring_offsets[ring_idx + 1];
        for (uint32_t vt = r_begin; vt < r_end; ++vt) {
            const uint16_t vert = scratch.flat_rings[vt];
            const int v42 = current_cache_stamp + 1;
            int look_ahead_stamp = current_cache_stamp + 1;

            if (ring_idx > 0 && ring_idx + 1 < num_rings) {
                const uint32_t nr_begin = scratch.ring_offsets[ring_idx + 1];
                const uint32_t nr_end = scratch.ring_offsets[ring_idx + 2];
                const int nr_size = static_cast<int>(nr_end - nr_begin);
                if (nr_size > 0) {
                    int lavt = 0;
                    int v20 = 0;
                    bool broken = false;
                    for (uint32_t next_i = nr_begin; next_i < nr_end; ++next_i) {
                        const uint16_t next_vert = scratch.flat_rings[next_i];
                        ++look_ahead_stamp;
                        const uint32_t l_begin = csr.link_offsets[next_vert];
                        const uint32_t l_end = csr.link_offsets[next_vert + 1];
                        ++v20;
                        bool v22 = false;
                        for (uint32_t li = l_begin; li < l_end; ++li) {
                            const uint16_t link_vert = csr.flat_links[li];
                            const auto& re = scratch.ring_entries[link_vert];
                            if (re.gen != ring_gen || re.ring_id != ring_idx) continue;
                            const auto& cs = scratch.cache_stamps[link_vert];
                            const int32_t stamp = (cs.gen == cache_gen) ? cs.stamp : -1;
                            bool v27;
                            if (stamp < 0) {
                                if (link_vert != vert) continue;
                                v27 = (v20 < 16);
                            } else {
                                v27 = (look_ahead_stamp - stamp < 16);
                            }
                            if (!v27) {
                                broken = true;
                                break;
                            }
                            v22 = true;
                        }
                        if (broken) break;
                        if (v22) {
                            ++lavt;
                            if (lavt >= nr_size) break;
                        } else {
                            break;
                        }
                    }
                    if (broken) {
                        break;
                    }
                }
            }

            dest_vert_list.push_back(vert);
            scratch.cache_stamps[vert] = {cache_gen, v42};
            current_cache_stamp = v42;
        }
    }
}

int32_t calcSectionPolyMaskFast(
    const std::vector<uint16_t>& poly_buffer,
    const VertexPolyCSR& csr,
    const std::vector<uint16_t>& vertex_list,
    StripifierScratch& scratch,
    uint32_t& out_mask_gen)
{
    const int poly_count = static_cast<int>(poly_buffer.size() / 3);
    const uint32_t vert_count = csr.vert_count;

    const uint32_t poly_proc_gen = scratch.nextGen(scratch.poly_proc_gen, scratch.poly_proc_stamp);
    const uint32_t sec_vert_gen = scratch.nextGen(scratch.sec_vert_gen, scratch.sec_vert_stamps);
    const uint32_t poly_mask_gen = scratch.nextGen(scratch.poly_mask_gen, scratch.poly_mask_stamp);
    out_mask_gen = poly_mask_gen;

    int32_t stamp = 0;
    for (uint16_t v : vertex_list) {
        if (v < vert_count) {
            scratch.sec_vert_stamps[v] = {sec_vert_gen, ++stamp};
        }
    }

    int matched_poly_count = 0;
    for (uint16_t vert : vertex_list) {
        if (vert >= vert_count) continue;
        const uint32_t p_begin = csr.poly_offsets[vert];
        const uint32_t p_end = csr.poly_offsets[vert + 1];
        for (uint32_t pi = p_begin; pi < p_end; ++pi) {
            const int32_t poly = csr.flat_polys[pi];
            if (poly >= 0 && poly < poly_count && scratch.poly_proc_stamp[poly] != poly_proc_gen) {
                scratch.poly_proc_stamp[poly] = poly_proc_gen;
                const uint16_t v0 = poly_buffer[3 * poly + 0];
                const uint16_t v1 = poly_buffer[3 * poly + 1];
                const uint16_t v2 = poly_buffer[3 * poly + 2];
                if (v0 < vert_count && v1 < vert_count && v2 < vert_count) {
                    const auto& st0 = scratch.sec_vert_stamps[v0];
                    const auto& st1 = scratch.sec_vert_stamps[v1];
                    const auto& st2 = scratch.sec_vert_stamps[v2];
                    if (st0.gen == sec_vert_gen && st1.gen == sec_vert_gen && st2.gen == sec_vert_gen) {
                        const int32_t min_s = std::min({st0.stamp, st1.stamp, st2.stamp});
                        const int32_t max_s = std::max({st0.stamp, st1.stamp, st2.stamp});
                        if (max_s - min_s < 15) {
                            scratch.poly_mask_stamp[poly] = poly_mask_gen;
                            ++matched_poly_count;
                        }
                    }
                }
            }
        }
    }

    return matched_poly_count;
}

void stripUnusedVertsFast(
    std::vector<uint16_t>& vertex_list,
    const VertexPolyCSR& csr,
    const StripifierScratch& scratch,
    int poly_count,
    uint32_t mask_gen)
{
    size_t write_idx = 0;
    for (size_t i = 0; i < vertex_list.size(); ++i) {
        const uint16_t v = vertex_list[i];
        if (v < csr.vert_count) {
            bool used = false;
            const uint32_t p_begin = csr.poly_offsets[v];
            const uint32_t p_end = csr.poly_offsets[v + 1];
            for (uint32_t pi = p_begin; pi < p_end; ++pi) {
                const int32_t p = csr.flat_polys[pi];
                if (p >= 0 && p < poly_count && scratch.poly_mask_stamp[p] == mask_gen) {
                    used = true;
                    break;
                }
            }
            if (used) {
                vertex_list[write_idx++] = v;
            }
        }
    }
    vertex_list.resize(write_idx);
}

double calcSectionScoreFast(
    const std::vector<uint16_t>& vert_list,
    const std::vector<uint16_t>& poly_buffer,
    const VertexPolyCSR& csr,
    StripifierScratch& scratch)
{
    uint32_t mask_gen = 0;
    const int poly_count = calcSectionPolyMaskFast(poly_buffer, csr, vert_list, scratch, mask_gen);
    if (poly_count == 0) {
        return 0.0;
    }

    const int total_polys = static_cast<int>(poly_buffer.size() / 3);
    int unfinished_vertex_count = 0;
    for (uint16_t v : vert_list) {
        if (v < csr.vert_count) {
            bool has_masked = false;
            bool has_unmasked = false;
            const uint32_t p_begin = csr.poly_offsets[v];
            const uint32_t p_end = csr.poly_offsets[v + 1];
            for (uint32_t pi = p_begin; pi < p_end; ++pi) {
                const int32_t p = csr.flat_polys[pi];
                if (p >= 0 && p < total_polys) {
                    if (scratch.poly_mask_stamp[p] == mask_gen) {
                        has_masked = true;
                    } else {
                        has_unmasked = true;
                    }
                    if (has_masked && has_unmasked) {
                        ++unfinished_vertex_count;
                        break;
                    }
                }
            }
        }
    }

    if (unfinished_vertex_count == 0) {
        return 1.0e30;
    }

    return static_cast<double>(poly_count) / static_cast<double>(unfinished_vertex_count);
}

int32_t findBestStartVertexFast(
    const std::vector<uint16_t>& poly_buffer,
    const VertexPolyCSR& csr,
    StripifierScratch& scratch)
{
    float best_score = 0.0f;
    int32_t best_start_vert = -1;

    // Pass 1: Boundary vertices only
    for (uint16_t start_vert : csr.active_verts) {
        const uint32_t l_begin = csr.link_offsets[start_vert];
        const uint32_t l_end = csr.link_offsets[start_vert + 1];
        if (l_begin == l_end) continue;

        uint16_t surround_vert = csr.flat_links[l_begin];
        const uint16_t score = surround_vert;
        int verts_searched = 0;
        const int num_links = static_cast<int>(l_end - l_begin);
        bool skip_start_vert = false;

        while (true) {
            const uint32_t p_begin = csr.poly_offsets[surround_vert];
            const uint32_t p_end = csr.poly_offsets[surround_vert + 1];
            if (p_begin == p_end) {
                if (++verts_searched >= num_links) break;
                surround_vert = score;
                continue;
            }

            bool found = false;
            for (uint32_t pi = p_begin; pi < p_end; ++pi) {
                const uint16_t* tri = &poly_buffer[3 * csr.flat_polys[pi]];
                int v15 = 0;
                for (; v15 < 3; ++v15) {
                    if (tri[v15] == surround_vert) break;
                }
                if (v15 < 3) {
                    const int v17 = kNext3[v15];
                    const int v18 = kNext3[v17];
                    if (tri[v17] == start_vert) {
                        surround_vert = tri[v18];
                        found = true;
                        break;
                    }
                }
            }

            if (!found || surround_vert == score) {
                if (found && surround_vert == score) {
                    skip_start_vert = true;
                }
                break;
            }
            if (++verts_searched >= num_links) break;
        }

        if (skip_start_vert) continue;

        buildRingsFast(start_vert, poly_buffer, csr, scratch);
        buildCacheOptimizedVertexListFast(csr, scratch, scratch.cand_vertex_list);

        const float score_val = static_cast<float>(calcSectionScoreFast(scratch.cand_vertex_list, poly_buffer, csr, scratch));
        if (best_score < score_val) {
            best_score = score_val;
            best_start_vert = start_vert;
        }
        if (best_score == 1.0e30f) break;
    }

    if (best_score != 0.0f) {
        return best_start_vert;
    }

    // Pass 2: All active vertices
    for (uint16_t v22 : csr.active_verts) {
        buildRingsFast(v22, poly_buffer, csr, scratch);
        buildCacheOptimizedVertexListFast(csr, scratch, scratch.cand_vertex_list);

        const float score_val = static_cast<float>(calcSectionScoreFast(scratch.cand_vertex_list, poly_buffer, csr, scratch));
        if (best_score < score_val) {
            best_score = score_val;
            best_start_vert = v22;
        }
        if (best_score == 1.0e30f) break;
    }

    if (best_score != 0.0f) {
        return best_start_vert;
    }

    // Pass 3: First active vertex with polys
    if (!csr.active_verts.empty()) {
        return csr.active_verts[0];
    }

    return best_start_vert;
}

void fillInPolygonsFast(
    const std::vector<uint16_t>& poly_buffer,
    const VertexPolyCSR& csr,
    const std::vector<uint16_t>& vertex_list,
    uint32_t mask_gen,
    StripifierScratch& scratch,
    std::vector<int32_t>& dest_poly_ids,
    std::vector<uint16_t>& dest_polys)
{
    dest_poly_ids.clear();
    dest_polys.clear();

    const int poly_count = static_cast<int>(poly_buffer.size() / 3);
    const uint32_t vert_pos_gen = scratch.nextGen(scratch.vert_pos_gen, scratch.vert_pos_stamps);
    const uint32_t poly_added_gen = scratch.nextGen(scratch.poly_added_gen, scratch.poly_added_stamp);
    const uint32_t vert_cached_gen = scratch.nextGen(scratch.vert_cached_gen, scratch.vert_cached_stamp);

    for (size_t i = 0; i < vertex_list.size(); ++i) {
        scratch.vert_pos_stamps[vertex_list[i]] = {vert_pos_gen, static_cast<int32_t>(i)};
    }

    auto getPos = [&](uint16_t v) -> int32_t {
        const auto& e = scratch.vert_pos_stamps[v];
        return (e.gen == vert_pos_gen) ? e.stamp : -1;
    };
    auto isVertCached = [&](uint16_t v) -> bool {
        return scratch.vert_cached_stamp[v] == vert_cached_gen;
    };

    for (int vt = 0; vt < static_cast<int>(vertex_list.size()); ++vt) {
        const uint16_t vert = vertex_list[vt];
        const uint32_t p_begin = csr.poly_offsets[vert];
        const uint32_t p_end = csr.poly_offsets[vert + 1];

        while (true) {
            int best_poly = -1;
            int best_poly_score = -0x7FFFFFFF;

            for (uint32_t pi = p_begin; pi < p_end; ++pi) {
                const int32_t poly = csr.flat_polys[pi];
                if (poly < 0 || poly >= poly_count) continue;
                if (scratch.poly_mask_stamp[poly] != mask_gen || scratch.poly_added_stamp[poly] == poly_added_gen) continue;

                const uint16_t tri[3] = {
                    poly_buffer[3 * poly + 0],
                    poly_buffer[3 * poly + 1],
                    poly_buffer[3 * poly + 2]
                };

                bool skip = false;
                for (int c = 0; c < 3; ++c) {
                    const uint16_t v = tri[c];
                    if (!isVertCached(v) && getPos(v) > vt) {
                        skip = true;
                        break;
                    }
                }
                if (skip) continue;

                int score = 0;
                if (isVertCached(tri[0]) || tri[0] == vert) ++score;
                if (isVertCached(tri[1]) || tri[1] == vert) ++score;
                if (isVertCached(tri[2]) || tri[2] == vert) ++score;

                uint16_t new_vert_ids[3] = {tri[0], tri[1], tri[2]};
                for (int rot = 0; rot < 2; ++rot) {
                    bool inverted = false;
                    for (int v69 = 1; v69 >= 0; --v69) {
                        const uint16_t v39 = new_vert_ids[v69 + 1];
                        const int32_t cache_order1 = getPos(v39);
                        const bool c39 = isVertCached(v39);
                        for (int v38 = v69; v38 >= 0; --v38) {
                            const uint16_t v40 = new_vert_ids[v38];
                            const int32_t v41 = getPos(v40);
                            if (!isVertCached(v40) && !c39 && v41 > cache_order1) {
                                inverted = true;
                                break;
                            }
                        }
                        if (inverted) break;
                    }

                    if (!inverted) {
                        score += 100;
                        break;
                    }

                    const uint16_t tmp = new_vert_ids[0];
                    new_vert_ids[0] = new_vert_ids[2];
                    new_vert_ids[2] = new_vert_ids[1];
                    new_vert_ids[1] = tmp;
                }

                if (score > best_poly_score) {
                    best_poly_score = score;
                    best_poly = poly;
                }
            }

            if (best_poly < 0) break;

            uint16_t rot_tri[3] = {
                poly_buffer[3 * best_poly + 0],
                poly_buffer[3 * best_poly + 1],
                poly_buffer[3 * best_poly + 2]
            };

            for (int rot = 0; rot < 2; ++rot) {
                bool inverted = false;
                for (int v69 = 1; v69 >= 0; --v69) {
                    const uint16_t v39 = rot_tri[v69 + 1];
                    const int32_t cache_order1 = getPos(v39);
                    const bool c39 = isVertCached(v39);
                    for (int v38 = v69; v38 >= 0; --v38) {
                        const uint16_t v40 = rot_tri[v38];
                        const int32_t v41 = getPos(v40);
                        if (!isVertCached(v40) && !c39 && v41 > cache_order1) {
                            inverted = true;
                            break;
                        }
                    }
                    if (inverted) break;
                }

                if (!inverted) break;

                const uint16_t tmp = rot_tri[0];
                rot_tri[0] = rot_tri[2];
                rot_tri[2] = rot_tri[1];
                rot_tri[1] = tmp;
            }

            dest_poly_ids.push_back(best_poly);
            for (int c = 0; c < 3; ++c) {
                dest_polys.push_back(rot_tri[c]);
                scratch.vert_cached_stamp[rot_tri[c]] = vert_cached_gen;
            }
            scratch.poly_added_stamp[best_poly] = poly_added_gen;
        }
    }
}

void fillInTriStripFast(
    const std::vector<uint16_t>& poly_buffer,
    const VertexPolyCSR& csr,
    const std::vector<uint16_t>& vertex_list,
    uint32_t mask_gen,
    StripifierScratch& scratch,
    std::vector<uint16_t>& strip,
    bool* back_face_toggle)
{
    strip.clear();
    if (vertex_list.size() < 2) return;

    const int poly_count = static_cast<int>(poly_buffer.size() / 3);
    const uint32_t vert_pos_gen = scratch.nextGen(scratch.vert_pos_gen, scratch.vert_pos_stamps);
    const uint32_t poly_added_gen = scratch.nextGen(scratch.poly_added_gen, scratch.poly_added_stamp);
    const uint32_t vert_cached_gen = scratch.nextGen(scratch.vert_cached_gen, scratch.vert_cached_stamp);

    for (size_t j = 0; j < vertex_list.size(); ++j) {
        scratch.vert_pos_stamps[vertex_list[j]] = {vert_pos_gen, static_cast<int32_t>(j)};
    }

    auto getPos = [&](uint16_t v) -> int32_t {
        const auto& e = scratch.vert_pos_stamps[v];
        return (e.gen == vert_pos_gen) ? e.stamp : -1;
    };
    auto isVertCached = [&](uint16_t v) -> bool {
        return scratch.vert_cached_stamp[v] == vert_cached_gen;
    };

    scratch.vert_cached_stamp[vertex_list[0]] = vert_cached_gen;
    scratch.vert_cached_stamp[vertex_list[1]] = vert_cached_gen;
    strip.push_back(vertex_list[0]);
    strip.push_back(vertex_list[1]);

    uint16_t last_vert2 = vertex_list[0];
    uint16_t last_vert = vertex_list[1];
    int vertex_pos = 2;

    while (vertex_pos < static_cast<int>(vertex_list.size())) {
        const uint16_t ring_vert = vertex_list[vertex_pos];
        const int next_ring_vert = (vertex_pos < static_cast<int>(vertex_list.size()) - 1) ? vertex_list[vertex_pos + 1] : -1;
        const uint32_t p_begin = csr.poly_offsets[ring_vert];
        const uint32_t p_end = csr.poly_offsets[ring_vert + 1];

        while (true) {
            int vert_rating = 0;
            int new_vert = -1;
            int new_poly = -1;

            for (uint32_t pi = p_begin; pi < p_end; ++pi) {
                const int32_t poly = csr.flat_polys[pi];
                if (poly < 0 || poly >= poly_count) continue;
                if (scratch.poly_added_stamp[poly] == poly_added_gen || scratch.poly_mask_stamp[poly] != mask_gen) continue;

                const uint16_t* tri = &poly_buffer[3 * poly];
                int verts_cached_count = 0;
                bool cannot_use = false;
                for (int i = 0; i < 3; ++i) {
                    const uint16_t tv = tri[i];
                    if (isVertCached(tv)) {
                        ++verts_cached_count;
                    } else if (getPos(tv) > vertex_pos) {
                        cannot_use = true;
                        break;
                    }
                }
                if (cannot_use) continue;
                if (isVertCached(ring_vert) && verts_cached_count < 3) continue;

                if (verts_cached_count == 0) {
                    if (vert_rating < 3 && last_vert == last_vert2) {
                        vert_rating = 3;
                        new_vert = ring_vert;
                        new_poly = -1;
                    } else if (vert_rating < 2 && last_vert != last_vert2) {
                        vert_rating = 2;
                        new_vert = last_vert;
                        new_poly = -1;
                    }
                } else if (verts_cached_count == 1) {
                    if (vert_rating < 7 && last_vert == last_vert2) {
                        bool matched = false;
                        for (int c = 0; c < 3; ++c) {
                            int v35 = kNext3[c];
                            int v36 = kNext3[v35];
                            if (!*back_face_toggle) {
                                std::swap(v35, v36);
                            }
                            if (tri[c] == last_vert && tri[v35] == ring_vert && tri[v36] == next_ring_vert) {
                                matched = true;
                                break;
                            }
                        }
                        if (matched) {
                            vert_rating = 7;
                            new_vert = ring_vert;
                            new_poly = -1;
                            continue;
                        }
                    }
                    if (vert_rating < 6 && last_vert == last_vert2) {
                        vert_rating = 6;
                        new_vert = ring_vert;
                        new_poly = -1;
                    } else if (vert_rating < 5 && last_vert != last_vert2) {
                        vert_rating = 5;
                        new_vert = last_vert;
                        new_poly = -1;
                    }
                } else if (verts_cached_count == 2) {
                    if (vert_rating < 13) {
                        bool matched = false;
                        for (int c = 0; c < 3; ++c) {
                            int v44 = kNext3[c];
                            int v45 = kNext3[v44];
                            if (*back_face_toggle) {
                                std::swap(v44, v45);
                            }
                            if (tri[c] == ring_vert && tri[v44] == last_vert2 && tri[v45] == last_vert) {
                                matched = true;
                                break;
                            }
                        }
                        if (matched) {
                            vert_rating = 13;
                            new_vert = ring_vert;
                            new_poly = poly;
                            continue;
                        }
                    }
                    if (vert_rating < 12 && last_vert == last_vert2) {
                        bool matched = false;
                        uint16_t third_vert = 0;
                        for (int c = 0; c < 3; ++c) {
                            int v49 = kNext3[c];
                            int v50 = kNext3[v49];
                            if (!*back_face_toggle) {
                                std::swap(v49, v50);
                            }
                            if (tri[c] == last_vert && tri[v50] == ring_vert) {
                                third_vert = tri[v49];
                                matched = true;
                                break;
                            }
                        }
                        if (matched) {
                            vert_rating = 12;
                            new_vert = third_vert;
                            new_poly = -1;
                            continue;
                        }
                    }
                    if (vert_rating < 11 && last_vert != last_vert2) {
                        bool matched = false;
                        for (int c = 0; c < 3; ++c) {
                            const int v55 = (*back_face_toggle) ? kPrev3[c] : kNext3[c];
                            if (tri[c] == ring_vert && tri[v55] == last_vert) {
                                matched = true;
                                break;
                            }
                        }
                        if (matched) {
                            vert_rating = 11;
                            new_vert = last_vert;
                            new_poly = -1;
                            continue;
                        }
                    }
                    if (vert_rating < 10 && last_vert == last_vert2) {
                        vert_rating = 10;
                        new_vert = ring_vert;
                        new_poly = -1;
                    } else if (vert_rating < 9 && last_vert != last_vert2) {
                        vert_rating = 9;
                        new_vert = last_vert;
                        new_poly = -1;
                    }
                } else if (verts_cached_count == 3) {
                    if (vert_rating < 19) {
                        bool matched = false;
                        uint16_t third_vert = 0;
                        for (int c = 0; c < 3; ++c) {
                            int v57 = kNext3[c];
                            int v58 = kNext3[v57];
                            if (*back_face_toggle) {
                                std::swap(v57, v58);
                            }
                            if (tri[v57] == last_vert2 && tri[v58] == last_vert) {
                                third_vert = tri[c];
                                matched = true;
                                break;
                            }
                        }
                        if (matched) {
                            vert_rating = 19;
                            new_vert = third_vert;
                            new_poly = poly;
                            continue;
                        }
                    }
                    if (vert_rating < 17 && last_vert != last_vert2) {
                        bool matched = false;
                        for (int c = 0; c < 3; ++c) {
                            if (tri[c] == last_vert) {
                                matched = true;
                                break;
                            }
                        }
                        if (matched) {
                            vert_rating = 17;
                            new_vert = last_vert;
                            new_poly = -1;
                            continue;
                        }
                    }
                    if (vert_rating < 18 && last_vert == last_vert2) {
                        bool matched = false;
                        uint16_t chosen = 0;
                        for (int c = 0; c < 3; ++c) {
                            const int v64 = (!*back_face_toggle) ? kPrev3[c] : kNext3[c];
                            if (tri[c] == last_vert) {
                                chosen = tri[v64];
                                matched = true;
                                break;
                            }
                        }
                        if (matched) {
                            vert_rating = 18;
                            new_vert = chosen;
                            new_poly = -1;
                            continue;
                        }
                    }
                    if (vert_rating < 16 && last_vert == last_vert2) {
                        vert_rating = 16;
                        new_vert = ring_vert;
                        new_poly = -1;
                    } else if (vert_rating < 15 && last_vert != last_vert2) {
                        vert_rating = 15;
                        new_vert = last_vert;
                        new_poly = -1;
                    }
                }
            }

            if (new_vert >= 0) {
                if (new_poly >= 0) {
                    scratch.poly_added_stamp[new_poly] = poly_added_gen;
                }
                strip.push_back(static_cast<uint16_t>(new_vert));
                scratch.vert_cached_stamp[new_vert] = vert_cached_gen;
                last_vert2 = last_vert;
                last_vert = static_cast<uint16_t>(new_vert);
                *back_face_toggle = !*back_face_toggle;
            } else {
                if (!isVertCached(ring_vert)) {
                    if (last_vert != last_vert2) {
                        strip.push_back(last_vert);
                        last_vert2 = last_vert;
                        *back_face_toggle = !*back_face_toggle;
                    }
                    strip.push_back(ring_vert);
                    last_vert = ring_vert;
                    scratch.vert_cached_stamp[ring_vert] = vert_cached_gen;
                    *back_face_toggle = !*back_face_toggle;
                }
                break;
            }
        }

        ++vertex_pos;
    }
}

} // namespace

void TriangleStripifier::buildVertexAndPolyMaps(
    const std::vector<uint16_t>& poly_buffer,
    std::vector<std::vector<int32_t>>& vertex_to_poly_map,
    std::vector<std::vector<uint16_t>>& vertex_link_map)
{
    vertex_to_poly_map.clear();
    vertex_link_map.clear();
    const uint32_t poly_count = static_cast<uint32_t>(poly_buffer.size() / 3);
    if (poly_count == 0) return;

    uint16_t max_v = 0;
    for (uint16_t v : poly_buffer) {
        if (v > max_v) max_v = v;
    }
    const size_t vert_count = static_cast<size_t>(max_v) + 1;

    std::vector<uint32_t> counts(vert_count, 0);
    for (uint32_t i = 0; i < poly_count * 3; ++i) {
        ++counts[poly_buffer[i]];
    }

    vertex_to_poly_map.resize(vert_count);
    for (size_t v = 0; v < vert_count; ++v) {
        if (counts[v] > 0) vertex_to_poly_map[v].reserve(counts[v]);
    }

    for (uint32_t pi = 0; pi < poly_count; ++pi) {
        for (int c = 0; c < 3; ++c) {
            const uint16_t v = poly_buffer[3 * pi + c];
            vertex_to_poly_map[v].push_back(static_cast<int32_t>(pi));
        }
    }

    vertex_link_map.resize(vert_count);
    for (size_t vta = 0; vta < vert_count; ++vta) {
        if (vertex_to_poly_map[vta].empty()) continue;
        for (int32_t pi : vertex_to_poly_map[vta]) {
            for (int c = 0; c < 3; ++c) {
                const uint16_t neighbor = poly_buffer[3 * pi + c];
                if (neighbor != vta) {
                    bool found = false;
                    for (uint16_t existing : vertex_link_map[vta]) {
                        if (existing == neighbor) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        vertex_link_map[vta].push_back(neighbor);
                    }
                }
            }
        }
    }
}

void TriangleStripifier::getMeshIslandSubsection(
    const std::vector<uint16_t>& poly_buffer,
    const std::vector<bool>& poly_mask,
    const std::vector<std::vector<int32_t>>& vertex_to_poly_map,
    std::vector<uint16_t>& subsection,
    std::vector<int32_t>& sub_section_to_global_poly_map)
{
    subsection.clear();
    int poly_count = static_cast<int>(poly_buffer.size() / 3);
    sub_section_to_global_poly_map.resize(poly_count);

    int start_poly = 0;
    for (; start_poly < poly_count; ++start_poly) {
        if (poly_mask[start_poly]) break;
    }
    if (start_poly >= poly_count) return;

    std::vector<int32_t> poly_stack;
    std::vector<uint16_t> vertex_stack;
    std::vector<bool> polys_processed(poly_count, false);
    std::vector<bool> verts_processed(vertex_to_poly_map.size(), false);

    poly_stack.push_back(start_poly);
    polys_processed[start_poly] = true;

    while (!poly_stack.empty() || !vertex_stack.empty()) {
        while (!poly_stack.empty()) {
            int32_t p = poly_stack.back();
            poly_stack.pop_back();

            int out_poly_idx = static_cast<int>(subsection.size() / 3);
            sub_section_to_global_poly_map[out_poly_idx] = p;

            for (int c = 0; c < 3; ++c) {
                uint16_t v = poly_buffer[3 * p + c];
                subsection.push_back(v);
                if (v < verts_processed.size() && !verts_processed[v]) {
                    verts_processed[v] = true;
                    vertex_stack.push_back(v);
                }
            }
        }

        while (!vertex_stack.empty()) {
            uint16_t v = vertex_stack.back();
            vertex_stack.pop_back();

            if (v < vertex_to_poly_map.size()) {
                for (int32_t p : vertex_to_poly_map[v]) {
                    if (p >= 0 && p < poly_count && !polys_processed[p] && poly_mask[p]) {
                        poly_stack.push_back(p);
                        polys_processed[p] = true;
                    }
                }
            }
        }
    }
}

int32_t TriangleStripifier::calcSectionPolyMask(
    const std::vector<uint16_t>& poly_buffer,
    const std::vector<std::vector<int32_t>>& vertex_to_poly_map,
    const std::vector<uint16_t>& vertex_list,
    std::vector<bool>& mask)
{
    int poly_count = static_cast<int>(poly_buffer.size() / 3);
    int vert_count = static_cast<int>(vertex_to_poly_map.size());
    std::vector<bool> polys_processed(poly_count, false);
    std::vector<bool> verts_in_section(vert_count, false);
    std::vector<int32_t> vert_cache_stamp(vert_count, -1073741824);

    int stamp = 0;
    for (uint16_t v : vertex_list) {
        if (v < vert_count) {
            verts_in_section[v] = true;
            vert_cache_stamp[v] = ++stamp;
        }
    }

    mask.assign(poly_count, false);
    int matched_poly_count = 0;

    for (uint16_t vert : vertex_list) {
        if (vert >= vert_count) continue;
        for (int32_t poly : vertex_to_poly_map[vert]) {
            if (poly >= 0 && poly < poly_count && !polys_processed[poly]) {
                polys_processed[poly] = true;
                uint16_t v0 = poly_buffer[3 * poly + 0];
                uint16_t v1 = poly_buffer[3 * poly + 1];
                uint16_t v2 = poly_buffer[3 * poly + 2];
                if (v0 < vert_count && v1 < vert_count && v2 < vert_count &&
                    verts_in_section[v0] && verts_in_section[v1] && verts_in_section[v2]) {
                    int32_t s0 = vert_cache_stamp[v0];
                    int32_t s1 = vert_cache_stamp[v1];
                    int32_t s2 = vert_cache_stamp[v2];
                    int32_t min_s = std::min({s0, s1, s2});
                    int32_t max_s = std::max({s0, s1, s2});
                    if (max_s - min_s < 15) {
                        mask[poly] = true;
                        ++matched_poly_count;
                    }
                }
            }
        }
    }

    return matched_poly_count;
}

void TriangleStripifier::stripUnusedVerts(
    std::vector<uint16_t>& vertex_list,
    const std::vector<std::vector<int32_t>>& vertex_to_poly_map,
    const std::vector<bool>& poly_mask)
{
    std::vector<uint16_t> new_list;
    for (uint16_t v : vertex_list) {
        if (v < vertex_to_poly_map.size()) {
            bool used = false;
            for (int32_t p : vertex_to_poly_map[v]) {
                if (p >= 0 && static_cast<size_t>(p) < poly_mask.size() && poly_mask[p]) {
                    used = true;
                    break;
                }
            }
            if (used) {
                new_list.push_back(v);
            }
        }
    }
    vertex_list = std::move(new_list);
}

double TriangleStripifier::calcSectionScore(
    const std::vector<uint16_t>& vert_list,
    const std::vector<uint16_t>& poly_buffer,
    const std::vector<std::vector<int32_t>>& vertex_to_poly_map)
{
    std::vector<bool> poly_mask;
    int poly_count = calcSectionPolyMask(poly_buffer, vertex_to_poly_map, vert_list, poly_mask);
    if (poly_count == 0) {
        return 0.0;
    }

    std::vector<uint16_t> new_vert_list = vert_list;
    stripUnusedVerts(new_vert_list, vertex_to_poly_map, poly_mask);

    int unfinished_vertex_count = 0;
    for (uint16_t v : new_vert_list) {
        if (v < vertex_to_poly_map.size()) {
            for (int32_t p : vertex_to_poly_map[v]) {
                if (p >= 0 && static_cast<size_t>(p) < poly_mask.size() && !poly_mask[p]) {
                    ++unfinished_vertex_count;
                    break;
                }
            }
        }
    }

    if (unfinished_vertex_count == 0) {
        return 1.0e30;
    }

    return static_cast<double>(poly_count) / static_cast<double>(unfinished_vertex_count);
}

void TriangleStripifier::buildRings(
    int32_t start_vert,
    const std::vector<uint16_t>& poly_buffer,
    const std::vector<std::vector<int32_t>>& vertex_to_poly_map,
    const std::vector<std::vector<uint16_t>>& vertex_link_map,
    std::vector<uint16_t>& vertex_to_ring_map,
    std::vector<uint16_t>& vertex_to_ring_pos_map,
    std::vector<std::vector<uint16_t>>& rings)
{
    int vertex_count = static_cast<int>(vertex_to_poly_map.size());
    std::vector<int32_t> vertex_to_prev_ring_vert_map(vertex_count, -1);
    vertex_to_ring_map.assign(vertex_count, 0xFFFF);
    vertex_to_ring_pos_map.assign(vertex_count, 0xFFFF);
    rings.clear();

    std::vector<uint16_t> prev_ring_list;
    prev_ring_list.push_back(static_cast<uint16_t>(start_vert));
    vertex_to_ring_map[start_vert] = 0;
    vertex_to_ring_pos_map[start_vert] = 0;
    rings.push_back(prev_ring_list);

    int ring_id = 1;
    while (!prev_ring_list.empty()) {
        std::vector<uint16_t> ring_list;
        for (uint16_t v : prev_ring_list) {
            if (v < vertex_link_map.size()) {
                for (uint16_t neighbor : vertex_link_map[v]) {
                    if (neighbor < vertex_count && vertex_to_ring_map[neighbor] == 0xFFFF) {
                        vertex_to_ring_map[neighbor] = static_cast<uint16_t>(ring_id);
                        vertex_to_prev_ring_vert_map[neighbor] = v;
                        ring_list.push_back(neighbor);
                    }
                }
            }
        }
        if (ring_list.empty()) break;

        std::vector<uint16_t> ordered_ring_list;
        std::vector<bool> verts_in_ordered_list(vertex_count, false);
        uint16_t v37 = ring_list[0];
        int prev_ring_vert_pos = 0;

        while (true) {
            int verts_searched = 0;
            uint16_t current_prev_ring_link = rings[ring_id - 1][prev_ring_vert_pos];
            uint16_t current_vert = v37;

            // Search 1:
            while (true) {
                bool found = false;
                uint16_t candidate = 0;
                if (v37 < vertex_to_poly_map.size()) {
                    for (int32_t poly : vertex_to_poly_map[v37]) {
                        const uint16_t* tri = &poly_buffer[3 * poly];
                        int v42 = 0;
                        for (; v42 < 3; ++v42) {
                            if (tri[v42] == v37) break;
                        }
                        if (v42 < 3) {
                            int v44 = (v42 + 1) % 3;
                            int v45 = (v44 + 1) % 3;
                            if (tri[v44] == current_prev_ring_link) {
                                candidate = tri[v45];
                                found = true;
                                break;
                            }
                        }
                    }
                }
                if (found) {
                    if (candidate < vertex_count &&
                        vertex_to_ring_map[candidate] == ring_id &&
                        !verts_in_ordered_list[candidate]) {
                        v37 = candidate;
                        current_vert = v37;
                        ++verts_searched;
                        if (verts_searched <= static_cast<int>(ring_list.size())) {
                            continue;
                        }
                    }
                }
                break;
            }

            vertex_to_ring_pos_map[v37] = static_cast<uint16_t>(ordered_ring_list.size());
            ordered_ring_list.push_back(v37);
            verts_in_ordered_list[v37] = true;

            if (ordered_ring_list.size() == ring_list.size()) {
                break;
            }

            // Search 2:
            uint16_t v56 = 0;
            bool found_v56 = false;
            while (!found_v56) {
                if (current_vert < vertex_to_poly_map.size()) {
                    for (int32_t poly : vertex_to_poly_map[current_vert]) {
                        const uint16_t* tri = &poly_buffer[3 * poly];
                        int v52 = 0;
                        for (; v52 < 3; ++v52) {
                            if (tri[v52] == current_vert) break;
                        }
                        if (v52 < 3) {
                            int v54 = (v52 + 2) % 3;
                            if (tri[v54] == current_prev_ring_link) {
                                uint16_t candidate = tri[(v54 + 2) % 3];
                                if (candidate < vertex_count &&
                                    vertex_to_ring_map[candidate] == ring_id &&
                                    !verts_in_ordered_list[candidate]) {
                                    v56 = candidate;
                                    found_v56 = true;
                                    break;
                                }
                            }
                        }
                    }
                }
                if (found_v56) break;

                if (current_prev_ring_link < vertex_link_map.size()) {
                    for (uint16_t neighbor : vertex_link_map[current_prev_ring_link]) {
                        if (neighbor < vertex_count &&
                            vertex_to_ring_map[neighbor] == ring_id &&
                            !verts_in_ordered_list[neighbor]) {
                            v56 = neighbor;
                            found_v56 = true;
                            break;
                        }
                    }
                }
                if (found_v56) break;

                ++prev_ring_vert_pos;
                current_prev_ring_link = rings[ring_id - 1][prev_ring_vert_pos];
            }

            v37 = v56;
        }

        rings.push_back(ordered_ring_list);
        prev_ring_list = ordered_ring_list;
        ++ring_id;
    }
}

void TriangleStripifier::buildCacheOptimizedVertexList(
    const std::vector<uint16_t>& poly_buffer,
    const std::vector<std::vector<int32_t>>& vertex_to_poly_map,
    const std::vector<std::vector<uint16_t>>& vertex_link_map,
    const std::vector<uint16_t>& vertex_to_ring_map,
    const std::vector<std::vector<uint16_t>>& rings,
    std::vector<uint16_t>& dest_vert_list)
{
    (void)poly_buffer;
    dest_vert_list.clear();
    int vert_count = static_cast<int>(vertex_to_poly_map.size());
    std::vector<bool> verts_processed(vert_count, false);
    std::vector<int32_t> vert_cache_stamp(vert_count, -1);
    int current_cache_stamp = 0;

    for (size_t ring_idx = 0; ring_idx < rings.size(); ++ring_idx) {
        const std::vector<uint16_t>& ring = rings[ring_idx];
        for (size_t vt = 0; vt < ring.size(); ++vt) {
            uint16_t vert = ring[vt];
            int v42 = current_cache_stamp + 1;
            int look_ahead_stamp = current_cache_stamp + 1;

            if (ring_idx > 0 && ring_idx < rings.size() - 1) {
                const std::vector<uint16_t>& next_ring = rings[ring_idx + 1];
                if (!next_ring.empty()) {
                    int lavt = 0;
                    int v20 = 0;
                    bool broken = false;
                    for (size_t next_i = 0; next_i < next_ring.size(); ++next_i) {
                        uint16_t next_vert = next_ring[next_i];
                        ++look_ahead_stamp;
                        const std::vector<uint16_t>& links = vertex_link_map[next_vert];
                        ++v20;
                        bool v22 = false;
                        for (uint16_t link_vert : links) {
                            if (vertex_to_ring_map[link_vert] != ring_idx) continue;
                            int32_t stamp = vert_cache_stamp[link_vert];
                            bool v27;
                            if (stamp < 0) {
                                if (link_vert != vert) continue;
                                v27 = (v20 < 16);
                            } else {
                                v27 = (look_ahead_stamp - stamp < 16);
                            }
                            if (!v27) {
                                broken = true;
                                break;
                            }
                            v22 = true;
                        }
                        if (broken) break;
                        if (v22) {
                            ++lavt;
                            if (lavt >= static_cast<int>(next_ring.size())) break;
                        } else {
                            break;
                        }
                    }
                    if (broken) {
                        break;
                    }
                }
            }

            verts_processed[vert] = true;
            dest_vert_list.push_back(vert);
            vert_cache_stamp[vert] = v42;
            current_cache_stamp = v42;
        }
    }
}

int32_t TriangleStripifier::findBestStartVertex(
    const std::vector<uint16_t>& poly_buffer,
    const std::vector<std::vector<int32_t>>& vertex_to_poly_map,
    const std::vector<std::vector<uint16_t>>& vertex_link_map)
{
    float best_score = 0.0f;
    int32_t best_start_vert = -1;
    int vertex_count = static_cast<int>(vertex_to_poly_map.size());

    // Pass 1: Boundary vertices only
    for (int start_vert = 0; start_vert < vertex_count; ++start_vert) {
        if (vertex_to_poly_map[start_vert].empty()) continue;
        const auto& links = vertex_link_map[start_vert];
        if (links.empty()) continue;

        uint16_t surround_vert = links[0];
        uint16_t score = surround_vert;
        int verts_searched = 0;
        int num_links = static_cast<int>(links.size());
        bool skip_start_vert = false;

        while (true) {
            const auto& polys = vertex_to_poly_map[surround_vert];
            if (polys.empty()) {
                if (++verts_searched >= num_links) break;
                surround_vert = score;
                continue;
            }

            bool found = false;
            for (int32_t poly : polys) {
                const uint16_t* tri = &poly_buffer[3 * poly];
                int v15 = 0;
                for (; v15 < 3; ++v15) {
                    if (tri[v15] == surround_vert) break;
                }
                if (v15 < 3) {
                    int v17 = (v15 + 1) % 3;
                    int v18 = (v17 + 1) % 3;
                    if (tri[v17] == start_vert) {
                        surround_vert = tri[v18];
                        found = true;
                        break;
                    }
                }
            }

            if (!found || surround_vert == score) {
                if (found && surround_vert == score) {
                    skip_start_vert = true;
                }
                break;
            }
            if (++verts_searched >= num_links) break;
        }

        if (skip_start_vert) continue;

        std::vector<uint16_t> vertex_to_ring_map;
        std::vector<uint16_t> vertex_to_ring_pos_map;
        std::vector<std::vector<uint16_t>> rings;
        buildRings(start_vert, poly_buffer, vertex_to_poly_map, vertex_link_map, vertex_to_ring_map, vertex_to_ring_pos_map, rings);

        std::vector<uint16_t> vertex_list;
        buildCacheOptimizedVertexList(poly_buffer, vertex_to_poly_map, vertex_link_map, vertex_to_ring_map, rings, vertex_list);

        float score_val = static_cast<float>(calcSectionScore(vertex_list, poly_buffer, vertex_to_poly_map));
        if (best_score < score_val) {
            best_score = score_val;
            best_start_vert = start_vert;
        }
        if (best_score == 1.0e30f) break;
    }

    if (best_score != 0.0f) {
        return best_start_vert;
    }

    // Pass 2: All vertices
    for (int v22 = 0; v22 < vertex_count; ++v22) {
        if (vertex_to_poly_map[v22].empty()) continue;

        std::vector<uint16_t> vertex_to_ring_map;
        std::vector<uint16_t> vertex_to_ring_pos_map;
        std::vector<std::vector<uint16_t>> rings;
        buildRings(v22, poly_buffer, vertex_to_poly_map, vertex_link_map, vertex_to_ring_map, vertex_to_ring_pos_map, rings);

        std::vector<uint16_t> vertex_list;
        buildCacheOptimizedVertexList(poly_buffer, vertex_to_poly_map, vertex_link_map, vertex_to_ring_map, rings, vertex_list);

        float score_val = static_cast<float>(calcSectionScore(vertex_list, poly_buffer, vertex_to_poly_map));
        if (best_score < score_val) {
            best_score = score_val;
            best_start_vert = v22;
        }
        if (best_score == 1.0e30f) break;
    }

    if (best_score != 0.0f) {
        return best_start_vert;
    }

    // Pass 3: First vertex with polys
    for (int i = 0; i < vertex_count; ++i) {
        if (!vertex_to_poly_map[i].empty()) {
            return i;
        }
    }

    return best_start_vert;
}

void TriangleStripifier::fillInPolygons(
    const std::vector<uint16_t>& poly_buffer,
    const std::vector<std::vector<int32_t>>& vertex_to_poly_map,
    const std::vector<uint16_t>& vertex_list,
    const std::vector<bool>& poly_mask,
    std::vector<int32_t>& dest_poly_ids,
    std::vector<uint16_t>& dest_polys,
    bool ignore_cache_checking)
{
    (void)ignore_cache_checking;
    dest_poly_ids.clear();
    dest_polys.clear();

    int poly_count = static_cast<int>(poly_buffer.size() / 3);
    int vert_count = static_cast<int>(vertex_to_poly_map.size());

    std::vector<bool> polys_added(poly_count, false);
    std::vector<bool> verts_cached(vert_count, false);
    std::vector<int32_t> vert_to_list_pos(vert_count, -1);

    for (size_t i = 0; i < vertex_list.size(); ++i) {
        vert_to_list_pos[vertex_list[i]] = static_cast<int32_t>(i);
    }

    for (int vt = 0; vt < static_cast<int>(vertex_list.size()); ++vt) {
        uint16_t vert = vertex_list[vt];

        while (true) {
            int best_poly = -1;
            int best_poly_score = -0x7FFFFFFF;

            for (int32_t poly : vertex_to_poly_map[vert]) {
                if (poly < 0 || poly >= poly_count) continue;
                if (!poly_mask[poly] || polys_added[poly]) continue;

                uint16_t tri[3] = {
                    poly_buffer[3 * poly + 0],
                    poly_buffer[3 * poly + 1],
                    poly_buffer[3 * poly + 2]
                };

                bool skip = false;
                for (int c = 0; c < 3; ++c) {
                    uint16_t v = tri[c];
                    if (!verts_cached[v] && vert_to_list_pos[v] > vt) {
                        skip = true;
                        break;
                    }
                }
                if (skip) continue;

                int score = 0;
                if (verts_cached[tri[0]] || tri[0] == vert) ++score;
                if (verts_cached[tri[1]] || tri[1] == vert) ++score;
                if (verts_cached[tri[2]] || tri[2] == vert) ++score;

                uint16_t new_vert_ids[3] = { tri[0], tri[1], tri[2] };
                for (int rot = 0; rot < 2; ++rot) {
                    bool inverted = false;
                    for (int v69 = 1; v69 >= 0; --v69) {
                        uint16_t v39 = new_vert_ids[v69 + 1];
                        int32_t cache_order1 = vert_to_list_pos[v39];
                        for (int v38 = v69; v38 >= 0; --v38) {
                            uint16_t v40 = new_vert_ids[v38];
                            int32_t v41 = vert_to_list_pos[v40];
                            if (!verts_cached[v40] && !verts_cached[v39] && v41 > cache_order1) {
                                inverted = true;
                                break;
                            }
                        }
                        if (inverted) break;
                    }

                    if (!inverted) {
                        score += 100;
                        break;
                    }

                    uint16_t tmp = new_vert_ids[0];
                    new_vert_ids[0] = new_vert_ids[2];
                    new_vert_ids[2] = new_vert_ids[1];
                    new_vert_ids[1] = tmp;
                }

                if (score > best_poly_score) {
                    best_poly_score = score;
                    best_poly = poly;
                }
            }

            if (best_poly < 0) break;

            uint16_t rot_tri[3] = {
                poly_buffer[3 * best_poly + 0],
                poly_buffer[3 * best_poly + 1],
                poly_buffer[3 * best_poly + 2]
            };

            for (int rot = 0; rot < 2; ++rot) {
                bool inverted = false;
                for (int v69 = 1; v69 >= 0; --v69) {
                    uint16_t v39 = rot_tri[v69 + 1];
                    int32_t cache_order1 = vert_to_list_pos[v39];
                    for (int v38 = v69; v38 >= 0; --v38) {
                        uint16_t v40 = rot_tri[v38];
                        int32_t v41 = vert_to_list_pos[v40];
                        if (!verts_cached[v40] && !verts_cached[v39] && v41 > cache_order1) {
                            inverted = true;
                            break;
                        }
                    }
                    if (inverted) break;
                }

                if (!inverted) break;

                uint16_t tmp = rot_tri[0];
                rot_tri[0] = rot_tri[2];
                rot_tri[2] = rot_tri[1];
                rot_tri[1] = tmp;
            }

            dest_poly_ids.push_back(best_poly);
            for (int c = 0; c < 3; ++c) {
                dest_polys.push_back(rot_tri[c]);
                verts_cached[rot_tri[c]] = true;
            }
            polys_added[best_poly] = true;
        }
    }
}

void TriangleStripifier::fillInTriStrip(
    const std::vector<uint16_t>& poly_buffer,
    const std::vector<std::vector<int32_t>>& vertex_to_poly_map,
    const std::vector<uint16_t>& vertex_list,
    const std::vector<bool>& poly_mask,
    std::vector<uint16_t>& strip,
    bool ignore_cache_checking,
    bool* back_face_toggle)
{
    (void)ignore_cache_checking;
    strip.clear();
    int vert_count = static_cast<int>(vertex_to_poly_map.size());
    int poly_count = static_cast<int>(poly_buffer.size() / 3);

    std::vector<int32_t> vert_to_list_pos(vert_count, -1);
    for (size_t j = 0; j < vertex_list.size(); ++j) {
        vert_to_list_pos[vertex_list[j]] = static_cast<int32_t>(j);
    }

    std::vector<bool> polys_added(poly_count, false);
    std::vector<bool> verts_cached(vert_count, false);
    std::vector<int32_t> vert_cache_stamp(vert_count, -1073741824);

    if (vertex_list.size() < 2) return;

    verts_cached[vertex_list[0]] = true;
    verts_cached[vertex_list[1]] = true;
    strip.push_back(vertex_list[0]);
    strip.push_back(vertex_list[1]);
    vert_cache_stamp[vertex_list[0]] = 0;
    vert_cache_stamp[vertex_list[1]] = 1;

    uint16_t last_vert2 = vertex_list[0];
    uint16_t last_vert = vertex_list[1];
    int current_cache_stamp = 2;
    int vertex_pos = 2;

    while (vertex_pos < static_cast<int>(vertex_list.size())) {
        uint16_t ring_vert = vertex_list[vertex_pos];
        int next_ring_vert = (vertex_pos < static_cast<int>(vertex_list.size()) - 1) ? vertex_list[vertex_pos + 1] : -1;

        while (true) {
            int vert_rating = 0;
            int new_vert = -1;
            int new_poly = -1;

            for (int32_t poly : vertex_to_poly_map[ring_vert]) {
                if (poly < 0 || poly >= poly_count) continue;
                if (polys_added[poly] || !poly_mask[poly]) continue;

                const uint16_t* tri = &poly_buffer[3 * poly];
                int verts_cached_count = 0;
                bool cannot_use = false;
                for (int i = 0; i < 3; ++i) {
                    uint16_t tv = tri[i];
                    if (verts_cached[tv]) {
                        ++verts_cached_count;
                    } else if (vert_to_list_pos[tv] > vertex_pos) {
                        cannot_use = true;
                        break;
                    }
                }
                if (cannot_use) continue;
                if (verts_cached[ring_vert] && verts_cached_count < 3) continue;

                if (verts_cached_count == 0) {
                    if (vert_rating < 3 && last_vert == last_vert2) {
                        vert_rating = 3;
                        new_vert = ring_vert;
                        new_poly = -1;
                    } else if (vert_rating < 2 && last_vert != last_vert2) {
                        vert_rating = 2;
                        new_vert = last_vert;
                        new_poly = -1;
                    }
                } else if (verts_cached_count == 1) {
                    if (vert_rating < 7 && last_vert == last_vert2) {
                        bool matched = false;
                        for (int c = 0; c < 3; ++c) {
                            int v35 = (c + 1) % 3;
                            int v36 = (v35 + 1) % 3;
                            if (!*back_face_toggle) {
                                v35 = (v35 + 1) % 3;
                                v36 = (c + 1) % 3;
                            }
                            if (tri[c] == last_vert && tri[v35] == ring_vert && tri[v36] == next_ring_vert) {
                                matched = true;
                                break;
                            }
                        }
                        if (matched) {
                            vert_rating = 7;
                            new_vert = ring_vert;
                            new_poly = -1;
                            continue;
                        }
                    }
                    if (vert_rating < 6 && last_vert == last_vert2) {
                        vert_rating = 6;
                        new_vert = ring_vert;
                        new_poly = -1;
                    } else if (vert_rating < 5 && last_vert != last_vert2) {
                        vert_rating = 5;
                        new_vert = last_vert;
                        new_poly = -1;
                    }
                } else if (verts_cached_count == 2) {
                    if (vert_rating < 13) {
                        bool matched = false;
                        for (int c = 0; c < 3; ++c) {
                            int v44 = (c + 1) % 3;
                            int v45 = (v44 + 1) % 3;
                            if (*back_face_toggle) {
                                v44 = (v44 + 1) % 3;
                                v45 = (c + 1) % 3;
                            }
                            if (tri[c] == ring_vert && tri[v44] == last_vert2 && tri[v45] == last_vert) {
                                matched = true;
                                break;
                            }
                        }
                        if (matched) {
                            vert_rating = 13;
                            new_vert = ring_vert;
                            new_poly = poly;
                            continue;
                        }
                    }
                    if (vert_rating < 12 && last_vert == last_vert2) {
                        bool matched = false;
                        uint16_t third_vert = 0;
                        for (int c = 0; c < 3; ++c) {
                            int v49 = (c + 1) % 3;
                            int v50 = (v49 + 1) % 3;
                            if (!*back_face_toggle) {
                                v49 = (v49 + 1) % 3;
                                v50 = (c + 1) % 3;
                            }
                            if (tri[c] == last_vert && tri[v50] == ring_vert) {
                                third_vert = tri[v49];
                                matched = true;
                                break;
                            }
                        }
                        if (matched) {
                            vert_rating = 12;
                            new_vert = third_vert;
                            new_poly = -1;
                            continue;
                        }
                    }
                    if (vert_rating < 11 && last_vert != last_vert2) {
                        bool matched = false;
                        for (int c = 0; c < 3; ++c) {
                            int v55 = (c + 1) % 3;
                            if (*back_face_toggle) {
                                v55 = (v55 + 1) % 3;
                            }
                            if (tri[c] == ring_vert && tri[v55] == last_vert) {
                                matched = true;
                                break;
                            }
                        }
                        if (matched) {
                            vert_rating = 11;
                            new_vert = last_vert;
                            new_poly = -1;
                            continue;
                        }
                    }
                    if (vert_rating < 10 && last_vert == last_vert2) {
                        vert_rating = 10;
                        new_vert = ring_vert;
                        new_poly = -1;
                    } else if (vert_rating < 9 && last_vert != last_vert2) {
                        vert_rating = 9;
                        new_vert = last_vert;
                        new_poly = -1;
                    }
                } else if (verts_cached_count == 3) {
                    if (vert_rating < 19) {
                        bool matched = false;
                        uint16_t third_vert = 0;
                        for (int c = 0; c < 3; ++c) {
                            int v57 = (c + 1) % 3;
                            int v58 = (v57 + 1) % 3;
                            if (*back_face_toggle) {
                                v57 = (v57 + 1) % 3;
                                v58 = (c + 1) % 3;
                            }
                            if (tri[v57] == last_vert2 && tri[v58] == last_vert) {
                                third_vert = tri[c];
                                matched = true;
                                break;
                            }
                        }
                        if (matched) {
                            vert_rating = 19;
                            new_vert = third_vert;
                            new_poly = poly;
                            continue;
                        }
                    }
                    if (vert_rating < 17 && last_vert != last_vert2) {
                        bool matched = false;
                        for (int c = 0; c < 3; ++c) {
                            if (tri[c] == last_vert) {
                                matched = true;
                                break;
                            }
                        }
                        if (matched) {
                            vert_rating = 17;
                            new_vert = last_vert;
                            new_poly = -1;
                            continue;
                        }
                    }
                    if (vert_rating < 18 && last_vert == last_vert2) {
                        bool matched = false;
                        uint16_t chosen = 0;
                        for (int c = 0; c < 3; ++c) {
                            int v64 = (c + 1) % 3;
                            if (!*back_face_toggle) {
                                v64 = (v64 + 1) % 3;
                            }
                            if (tri[c] == last_vert) {
                                chosen = tri[v64];
                                matched = true;
                                break;
                            }
                        }
                        if (matched) {
                            vert_rating = 18;
                            new_vert = chosen;
                            new_poly = -1;
                            continue;
                        }
                    }
                    if (vert_rating < 16 && last_vert == last_vert2) {
                        vert_rating = 16;
                        new_vert = ring_vert;
                        new_poly = -1;
                    } else if (vert_rating < 15 && last_vert != last_vert2) {
                        vert_rating = 15;
                        new_vert = last_vert;
                        new_poly = -1;
                    }
                }
            }

            if (new_vert >= 0) {
                if (new_poly >= 0) {
                    polys_added[new_poly] = true;
                }
                strip.push_back(static_cast<uint16_t>(new_vert));
                if (!verts_cached[new_vert]) {
                    verts_cached[new_vert] = true;
                    vert_cache_stamp[ring_vert] = ++current_cache_stamp;
                }
                last_vert2 = last_vert;
                last_vert = static_cast<uint16_t>(new_vert);
                *back_face_toggle = !*back_face_toggle;
            } else {
                if (!verts_cached[ring_vert]) {
                    if (last_vert != last_vert2) {
                        strip.push_back(last_vert);
                        last_vert2 = last_vert;
                        *back_face_toggle = !*back_face_toggle;
                    }
                    strip.push_back(ring_vert);
                    last_vert = ring_vert;
                    verts_cached[ring_vert] = true;
                    vert_cache_stamp[ring_vert] = ++current_cache_stamp;
                    *back_face_toggle = !*back_face_toggle;
                }
                break;
            }
        }

        ++vertex_pos;
    }
}

void TriangleStripifier::generateTriangleListAndStrip(
    const std::vector<uint16_t>& in_list,
    std::vector<uint16_t>& out_list,
    std::vector<uint16_t>& out_strip)
{
    int poly_count = static_cast<int>(in_list.size() / 3);
    if (poly_count == 0) {
        out_list.clear();
        out_strip.clear();
        return;
    }

    int polys_processed_count = 0;
    std::vector<uint8_t> polys_still_to_add(poly_count, 1);
    out_list.clear();
    out_strip.clear();

    VertexPolyCSR global_csr;
    buildCSR(in_list, global_csr, false);

    StripifierScratch scratch;
    scratch.ensureSizes(global_csr.vert_count, static_cast<uint32_t>(poly_count));

    VertexPolyCSR sub_csr;
    std::vector<int32_t> sub_section_to_main_buffer_poly_map;
    std::vector<uint16_t> sub_poly_buffer;
    std::vector<uint16_t> vertex_list;
    std::vector<int32_t> sec_poly_ids;
    std::vector<uint16_t> sec_polys;
    std::vector<uint16_t> sec_strip;

    bool back_face_toggle = false;
    int last_strip_vertex = -1;
    int first_unprocessed_poly = 0;

    while (polys_processed_count < poly_count) {
        getMeshIslandSubsectionFast(
            in_list,
            polys_still_to_add,
            first_unprocessed_poly,
            global_csr,
            scratch,
            sub_poly_buffer,
            sub_section_to_main_buffer_poly_map
        );

        buildCSR(sub_poly_buffer, sub_csr, true);

        int best_start_vert = findBestStartVertexFast(sub_poly_buffer, sub_csr, scratch);

        buildRingsFast(best_start_vert, sub_poly_buffer, sub_csr, scratch);
        buildCacheOptimizedVertexListFast(sub_csr, scratch, vertex_list);

        const int sub_poly_count = static_cast<int>(sub_poly_buffer.size() / 3);
        uint32_t mask_gen = 0;
        if (calcSectionPolyMaskFast(sub_poly_buffer, sub_csr, vertex_list, scratch, mask_gen) != 0) {
            stripUnusedVertsFast(vertex_list, sub_csr, scratch, sub_poly_count, mask_gen);
            calcSectionPolyMaskFast(sub_poly_buffer, sub_csr, vertex_list, scratch, mask_gen);
        } else {
            mask_gen = scratch.nextGen(scratch.poly_mask_gen, scratch.poly_mask_stamp);
            std::fill(scratch.poly_mask_stamp.begin(), scratch.poly_mask_stamp.begin() + sub_poly_count, mask_gen);
        }

        fillInPolygonsFast(
            sub_poly_buffer,
            sub_csr,
            vertex_list,
            mask_gen,
            scratch,
            sec_poly_ids,
            sec_polys
        );

        fillInTriStripFast(
            sub_poly_buffer,
            sub_csr,
            vertex_list,
            mask_gen,
            scratch,
            sec_strip,
            &back_face_toggle
        );

        if (!sec_strip.empty()) {
            if (last_strip_vertex >= 0) {
                out_strip.push_back(static_cast<uint16_t>(last_strip_vertex));
                out_strip.push_back(sec_strip[0]);
            }
            for (uint16_t sv : sec_strip) {
                out_strip.push_back(sv);
            }
            last_strip_vertex = out_strip.back();
        }

        for (size_t i = 0; i < sec_poly_ids.size(); ++i) {
            int32_t main_poly = sub_section_to_main_buffer_poly_map[sec_poly_ids[i]];
            ++polys_processed_count;
            polys_still_to_add[main_poly] = 0;
            out_list.push_back(sec_polys[3 * i + 0]);
            out_list.push_back(sec_polys[3 * i + 1]);
            out_list.push_back(sec_polys[3 * i + 2]);
        }
    }
}

void TriangleStripifier::reorderTriangleListInPlace(uint16_t* in_list, int32_t poly_count)
{
    if (!in_list || poly_count <= 0) return;
    std::vector<uint16_t> src_list(in_list, in_list + 3 * poly_count);
    std::vector<uint16_t> dest_list;
    std::vector<uint16_t> dest_strip;
    generateTriangleListAndStrip(src_list, dest_list, dest_strip);
    for (int i = 0; i < 3 * poly_count; ++i) {
        in_list[i] = dest_list[i];
    }
}

void TriangleStripifier::reorderTriangleListInPlace(std::vector<uint16_t>& indices, int32_t cache_size)
{
    (void)cache_size;
    int32_t poly_count = static_cast<int32_t>(indices.size() / 3);
    if (poly_count <= 0) return;
    reorderTriangleListInPlace(indices.data(), poly_count);
}

} // namespace egostb
