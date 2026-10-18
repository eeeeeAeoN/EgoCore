#pragma once

#include <cstdint>
#include <vector>

namespace egostb {

class TriangleStripifier {
public:
    // Main entry point matching CTriangleStripifier2::ReorderTriangleListInPlace (0x145b7e0)
    static void reorderTriangleListInPlace(std::vector<uint16_t>& indices, int32_t cache_size = 20);
    static void reorderTriangleListInPlace(uint16_t* in_list, int32_t poly_count);

    // Matching CTriangleStripifier2::GenerateTriangleListAndStrip (0x145a9b0)
    static void generateTriangleListAndStrip(
        const std::vector<uint16_t>& in_list,
        std::vector<uint16_t>& out_list,
        std::vector<uint16_t>& out_strip
    );

    // Helper methods matching CTriangleStripifier2:
    static void buildVertexAndPolyMaps(
        const std::vector<uint16_t>& poly_buffer,
        std::vector<std::vector<int32_t>>& vertex_to_poly_map,
        std::vector<std::vector<uint16_t>>& vertex_link_map
    );

    static void getMeshIslandSubsection(
        const std::vector<uint16_t>& poly_buffer,
        const std::vector<bool>& poly_mask,
        const std::vector<std::vector<int32_t>>& vertex_to_poly_map,
        std::vector<uint16_t>& subsection,
        std::vector<int32_t>& sub_section_to_global_poly_map
    );

    static int32_t findBestStartVertex(
        const std::vector<uint16_t>& poly_buffer,
        const std::vector<std::vector<int32_t>>& vertex_to_poly_map,
        const std::vector<std::vector<uint16_t>>& vertex_link_map
    );

    static void buildRings(
        int32_t start_vert,
        const std::vector<uint16_t>& poly_buffer,
        const std::vector<std::vector<int32_t>>& vertex_to_poly_map,
        const std::vector<std::vector<uint16_t>>& vertex_link_map,
        std::vector<uint16_t>& vertex_to_ring_map,
        std::vector<uint16_t>& vertex_to_ring_pos_map,
        std::vector<std::vector<uint16_t>>& rings
    );

    static void buildCacheOptimizedVertexList(
        const std::vector<uint16_t>& poly_buffer,
        const std::vector<std::vector<int32_t>>& vertex_to_poly_map,
        const std::vector<std::vector<uint16_t>>& vertex_link_map,
        const std::vector<uint16_t>& vertex_to_ring_map,
        const std::vector<std::vector<uint16_t>>& rings,
        std::vector<uint16_t>& dest_vert_list
    );

    static int32_t calcSectionPolyMask(
        const std::vector<uint16_t>& poly_buffer,
        const std::vector<std::vector<int32_t>>& vertex_to_poly_map,
        const std::vector<uint16_t>& vertex_list,
        std::vector<bool>& mask
    );

    static void stripUnusedVerts(
        std::vector<uint16_t>& vertex_list,
        const std::vector<std::vector<int32_t>>& vertex_to_poly_map,
        const std::vector<bool>& poly_mask
    );

    static double calcSectionScore(
        const std::vector<uint16_t>& vert_list,
        const std::vector<uint16_t>& poly_buffer,
        const std::vector<std::vector<int32_t>>& vertex_to_poly_map
    );

    static void fillInPolygons(
        const std::vector<uint16_t>& poly_buffer,
        const std::vector<std::vector<int32_t>>& vertex_to_poly_map,
        const std::vector<uint16_t>& vertex_list,
        const std::vector<bool>& poly_mask,
        std::vector<int32_t>& dest_poly_ids,
        std::vector<uint16_t>& dest_polys,
        bool ignore_cache_checking
    );

    static void fillInTriStrip(
        const std::vector<uint16_t>& poly_buffer,
        const std::vector<std::vector<int32_t>>& vertex_to_poly_map,
        const std::vector<uint16_t>& vertex_list,
        const std::vector<bool>& poly_mask,
        std::vector<uint16_t>& strip,
        bool ignore_cache_checking,
        bool* back_face_toggle
    );
};

using CTriangleStripifier2 = TriangleStripifier;

} // namespace egostb
