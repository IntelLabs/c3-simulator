/*
 Copyright Intel Corporation
 SPDX-License-Identifier: MIT
*/

#ifndef MODULES_COMMON_CCSIMICS_ICV_MAP_H_
#define MODULES_COMMON_CCSIMICS_ICV_MAP_H_

#include <cmath>
#include <map>
#include <memory>
#include <utility>
#include "ccsimics/simics_util.h"
#include "malloc/cc_globals.h"

namespace ccsimics {

/**
 * @brief Simple ICV map with on-demand page creation
 */
class ICVMap final {
    static constexpr const uint64_t kPage1Size = (1LU << 21);

    /**
     * @brief Internal class for last-level ICV map page
     */
    class Page1 final {
        unsigned count = 0;
        icv_t icv_map_[kPage1Size];

     public:
        Page1 &operator=(const Page1 &) = default;
        Page1 &operator=(Page1 &&) = default;

        unsigned set(uint64_t index, icv_t val) {
            const auto i = get_page1_index(index);
            if (icv_map_[i] != 0 && val == 0)
                --count;
            if (icv_map_[i] == 0 && val != 0)
                ++count;
            icv_map_[i] = val;
            return count;
        }

        icv_t get(uint64_t i) const { return icv_map_[get_page1_index(i)]; }
    };

    std::map<uint64_t, Page1> icv_map_;

 public:
    void clear() { icv_map_.clear(); }

    void set(uint64_t i, icv_t val) {
        const auto page2_i = get_page2_index(i);

        // Check if we have a old page to update
        auto found = icv_map_.find(page2_i);
        if (found != icv_map_.end()) {
            if (found->second.set(i, val) == 0) {
                // The last index got set to 0, clear page
                icv_map_.erase(found);
            }
            return;
        }

        // If we're just resetting ICV, we do't need the page entry!
        if (val == 0) {
            return;
        }

        // Otherwise, create new entry and add value (note that emplace returns
        // a pair with inserted value and bool indicating insertion).
        icv_map_[page2_i].set(i, val);
    }

    icv_t get(uint64_t i) {
        const auto page2_i = get_page2_index(i);

        auto found = icv_map_.find(page2_i);
        if (found != icv_map_.end()) {
            return found->second.get(i);
        }
        return 0;
    }

    static inline uint64_t get_page1_index(uint64_t i) {
        return i % kPage1Size;
    }

    static inline uint64_t get_page2_index(uint64_t i) {
        return i / kPage1Size;
    }
};

}  // namespace ccsimics

#endif  // MODULES_COMMON_CCSIMICS_ICV_MAP_H_
