#pragma once

#include "details/flat_base_multimap.h"

namespace Ubpa {
    // require
    // - Vector<std::pair<const Key, T>>::iterator <=> Vector<std::pair<Key, T>>::iterator
    // - Vector<std::pair<const Key, T>>::const_iterator <=> Vector<std::pair<Key, T>>::const_iterator
    template <typename Key, typename T, template<typename>class Vector, typename Compare>
    class flat_map : public details::flat_base_multimap<flat_map<Key, T, Vector, Compare>, false, Key, T, Vector, Compare> {
        using mybase = details::flat_base_multimap<flat_map<Key, T, Vector, Compare>, false, Key, T, Vector, Compare>;
    public:
        //////////////////
        // Member types //
        //////////////////

        using key_type = typename mybase::key_type;
        using mapped_type = typename mybase::mapped_type;
        using value_type = typename mybase::value_type;
        using size_type = typename mybase::size_type;
        using difference_type = typename mybase::difference_type;
        using key_compare = typename mybase::key_compare;

        using container_type = typename mybase::container_type;

        using iterator = typename mybase::iterator;
        using const_iterator = typename mybase::const_iterator;
        using reverse_iterator = typename mybase::reverse_iterator;
        using const_reverse_iterator = typename mybase::const_reverse_iterator;

        //////////////////////
        // Member functions //
        //////////////////////

        using mybase::mybase;
        using mybase::operator=;

        //
        // Element access
        ///////////////////

        mapped_type& at(const key_type& key) {
            auto target = mybase::find(key);
            if (target == mybase::end())
                throw_out_of_range();
            return std::get<1>(*target);
        }

        const mapped_type& at(const key_type& key) const { return const_cast<flat_map*>(this)->at(key); }

        mapped_type& operator[](const key_type& key) { return std::get<1>(*try_emplace(key)->first); }
        mapped_type& operator[](key_type&& key) { return std::get<1>(*try_emplace(std::move(key))->first); }

        //
        // Modifiers
        //////////////

        template<typename... Args>
        std::pair<iterator, bool> try_emplace(const key_type& k, Args&&... args)
        { return try_emplace_impl(k, std::forward<Args>(args)...); }

        template<typename... Args>
        std::pair<iterator, bool> try_emplace(key_type&& k, Args&&... args)
        { return try_emplace_impl(std::move(k), std::forward<Args>(args)...); }

        template<typename... Args>
        iterator try_emplace(const_iterator hint, const key_type& k, Args&&... args)
        { return try_emplace_hint_impl(hint, k, std::forward<Args>(args)...); }

        template<typename... Args>
        iterator try_emplace(const_iterator hint, key_type&& k, Args&&... args)
        { return try_emplace_hint_impl(hint, std::move(k), std::forward<Args>(args)...); }

    private:
        [[noreturn]] void throw_out_of_range() const { throw std::out_of_range("invalid flat_map subscript"); }

        template<typename K, typename... Args>
        void storage_emplace_back(K&& k, Args&&... args) {
            if constexpr (std::is_constructible_v<value_type, std::piecewise_construct_t, std::tuple<K&&>, std::tuple<Args&&...>>) {
                mybase::storage.emplace_back(std::piecewise_construct_t{},
                    std::forward_as_tuple(std::forward<K>(k)),
                    std::forward_as_tuple(std::forward<Args>(args)...));
            }
            else
                mybase::storage.emplace_back(std::forward<K>(k), mapped_type(std::forward<Args>(args)...));
        }

        template<typename K, typename... Args>
        iterator storage_emplace_hint(const_iterator hint, K&& k, Args&&... args) {
            if constexpr (std::is_constructible_v<value_type, std::piecewise_construct_t, std::tuple<K&&>, std::tuple<Args&&...>>) {
                return mybase::cast_iterator(mybase::template emplace_hint(mybase::cast_iterator(hint), std::piecewise_construct_t{},
                    std::forward_as_tuple(std::forward<K>(k)),
                    std::forward_as_tuple(std::forward<Args>(args)...)));
            }
            else {
                return mybase::cast_iterator(mybase::template emplace_hint(mybase::cast_iterator(hint),
                    std::forward<K>(k),
                    mapped_type(std::forward<Args>(args)...)));
            }
        }

        template<typename K, typename... Args>
        std::pair<iterator, bool> try_emplace_impl(K&& k, Args&&... args) {
            auto lb = mybase::lower_bound(k); // key <= lb
            if (lb == mybase::end() || mybase::key_comp()(k, *lb)) // key < lb
                return { storage_emplace_hint(lb, std::forward<K>(k), mapped_type(std::forward<Args>(args)...)), true };
            else
                return { lb, false }; // key == lb
        }

        template<typename K, typename... Args>
        iterator try_emplace_hint_impl(const_iterator hint, K&& k, Args&&... args) {
            assert(mybase::begin() <= hint && hint <= mybase::end());

            const_iterator first;
            const_iterator last;

            if (hint == mybase::begin() || mybase::key_comp()(*std::prev(hint), k)) { // k > hint - 1
                if (hint < mybase::end()) {
                    if (mybase::key_comp()(k, *hint)) // k < hint
                        return storage_emplace_hint(hint, std::forward<K>(k), mapped_type(std::forward<Args>(args)...));
                    else { // k >= hint
                        first = hint;
                        last = mybase::cend();
                    }
                }
                else { // hint == end()
                    storage_emplace_back(std::forward<K>(k), mapped_type(std::forward<Args>(args)...));
                    return std::prev(mybase::end());
                }
            }
            else { // k <= hint - 1
                first = mybase::cbegin();
                last = hint;
            }

            auto lb = std::lower_bound(first, last, k, mybase::key_comp()); // value <= lb


            if (lb == mybase::end() || mybase::key_comp()(k, *lb)) // value < lb
                return storage_emplace_hint(lb, std::forward<K>(k), mapped_type(std::forward<Args>(args)...));
            else
                return mybase::begin() + std::distance(mybase::cbegin(), lb); // value == lb
        }
    };

    template <typename Key, typename T, template<typename>class Vector, typename Compare>
    bool operator==(const flat_map<Key, T, Vector, Compare>& lhs, const flat_map<Key, T, Vector, Compare>& rhs) {
        return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    template <typename Key, typename T, template<typename>class Vector, typename Compare>
    bool operator<(const flat_map<Key, T, Vector, Compare>& lhs, const flat_map<Key, T, Vector, Compare>& rhs) {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    template <typename Key, typename T, template<typename>class Vector, typename Compare>
    bool operator!=(const flat_map<Key, T, Vector, Compare>& lhs, const flat_map<Key, T, Vector, Compare>& rhs) {
        return !(lhs == rhs);
    }

    template <typename Key, typename T, template<typename>class Vector, typename Compare>
    bool operator>(const flat_map<Key, T, Vector, Compare>& lhs, const flat_map<Key, T, Vector, Compare>& rhs) {
        return rhs < lhs;
    }

    template <typename Key, typename T, template<typename>class Vector, typename Compare>
    bool operator<=(const flat_map<Key, T, Vector, Compare>& lhs, const flat_map<Key, T, Vector, Compare>& rhs) {
        return !(rhs < lhs);
    }

    template <typename Key, typename T, template<typename>class Vector, typename Compare>
    bool operator>=(const flat_map<Key, T, Vector, Compare>& lhs, const flat_map<Key, T, Vector, Compare>& rhs) {
        return !(lhs < rhs);
    }
}
