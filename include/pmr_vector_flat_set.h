#include "flat_set.h"

#include <vector>

namespace Ubpa {
    template<typename Key, typename Compare = std::less<Key>>
    using pmr_vector_flat_set = flat_set<Key, std::pmr::vector, Compare>;
}