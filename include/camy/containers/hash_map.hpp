/* int2int_map.hpp
 *
 * Copyright (C) 2017 Edoardo Dominici
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

// camy
#include <camy/containers/pool.hpp>
#include <camy/core.hpp>

// C++ STL
#include <type_traits>

namespace camy
{
    // In order to avoid duplicating code and to keep the implementation hidden from user C++11
    // SFINAE support is used to only enable certain functions.
    // This is used in 2 contexts:
    // - Different base class to have an additional member
    // - _*** internal HashMap functions to provide specific overloads base on the type size (this
    // is done using and additional default parameter, but it's not exposed to the user, instead
    // wrapped by another function (hopefully assuming the compiler flattens it)
    template <typename T>
    using HMEnableExtTrue = typename std::enable_if<!std::is_convertible<T, uint64>::value>::type;

    template <typename T>
    using HMEnableExtFalse = typename std::enable_if<std::is_convertible<T, uint64>::value>::type;

    template <typename T, typename Enable = void>
    class BaseHashMap;

    template <typename T>
    struct BaseHashMap<T, HMEnableExtFalse<T>>
    {
        BaseHashMap(rsize capacity, rsize alignment) {}
    };

    template <typename T>
    struct BaseHashMap<T, HMEnableExtTrue<T>>
    {
        BaseHashMap(rsize capacity, rsize alignment)
            : m_ext_buffer(capacity)
        {
        } // TODO: pass alignment
        Pool<T> m_ext_buffer;
    };

    // Hashmap indexable by a 64-bit integer, strings can also be used and hashed into keys
    // at compile time (see API::hash_str). There is not really the need for other type of keys.
    // if T is_convertible to a uint64 no external buffer is used and the val is stored with the key
    // in the internal buffer. Otherwise an external buffer is used and an index is stored ( +1
    // level of indirection)
    template <typename T>
    class CAMY_API HashMap final : protected BaseHashMap<T>
    {
    public:
        using TKey = uint64;
        static const TKey INVALID_KEY = (TKey)-1;

        static const rsize ELEMENT_SIZE = sizeof(T);
        static const rsize DEFAULT_CAPACITY = 128;

        using TBaseHashMap = BaseHashMap<T>;
        using THashMap = HashMap<T>;
        using TValue = T;
        using TPtr = T*;
        using TConstPtr = const T*;
        using TRef = T&;
        using TConstRef = const T&;

    public:
        explicit HashMap(rsize capacity = DEFAULT_CAPACITY, rsize alignment = DEFAULT_ALIGNMENT);
        ~HashMap();

        HashMap(const THashMap& other);
        HashMap(THashMap&& other);

        THashMap& operator=(const THashMap& other);
        THashMap& operator=(THashMap&& other);

        // Looks for a specific key, if non is found nullptr is returned
        TPtr find(TKey key);
        TConstPtr find(TKey key) const;

        // Adds a new <Key, T(args)|val> pair to the hashmap
        template <typename... Ts>
        TPtr emplace(TKey key, Ts&&... args);
        TPtr insert(TKey key, TConstRef val);

        void remove(TKey key);
        void clear();

        rsize count();
        rsize capacity() const;
        bool empty() const;

    private:
        struct Entry
        {
            TKey key;
            uint64 val;
        };
        static const rsize ENTRY_SIZE = sizeof(Entry);

        Entry* _allocate_align_explicit(rsize n, rsize alignment);
        Entry* _allocate_align_same(rsize n, void* src_alignment);
        void _deallocate(Entry* ptr);
        void _reset_entries(Entry* beg, rsize n);

        rsize _hash(TKey key) const;
        void _make_space(rsize slots);

        // see HMEnableExt*** for more info
        template <typename U = T>
        TPtr _find(TKey key, HMEnableExtTrue<U>* = nullptr);

        template <typename U = T>
        TPtr _find(TKey key, HMEnableExtFalse<U>* = nullptr);

        template <typename U = T, typename... Ts>
        TPtr _insert(TKey key, HMEnableExtTrue<U>* = nullptr, Ts&&... args);

        template <typename U = T, typename... Ts>
        TPtr _insert(TKey key, HMEnableExtFalse<U>* = nullptr, Ts&&... args);

        template <typename U = T>
        void _remove(TKey key, HMEnableExtTrue<U>* = nullptr);

        template <typename U = T>
        void _remove(TKey key, HMEnableExtFalse<U>* = nullptr);

        template <typename U = T>
        void _clear(HMEnableExtTrue<U>* = nullptr);

        template <typename U = T>
        void _clear(HMEnableExtFalse<U>* = nullptr);

        Entry* m_entries;
        rsize m_capacity;
        rsize m_slots_occupied;
    };

    template <typename T>
    CAMY_INLINE HashMap<T>::HashMap(rsize capacity, rsize alignment)
        : TBaseHashMap(capacity, alignment)
        , m_entries(nullptr)
        , m_capacity(0)
        , m_slots_occupied(0)
    {
        m_entries = _allocate_align_explicit(capacity, alignment);
        _reset_entries(m_entries, capacity);
        m_capacity = capacity;
    }

    template <typename T>
    CAMY_INLINE HashMap<T>::~HashMap()
    {
        _deallocate(m_entries);
    }

    template <typename T>
    CAMY_INLINE HashMap<T>::HashMap(const THashMap& other)
        : TBaseHashMap(other)
    {
        m_entries = _allocate_align_same(other.m_capacity, other.m_entries);
        memcpy(m_entries, other.m_entries, ENTRY_SIZE * other.m_capacity);
        m_capacity = other.m_capacity;
        m_slots_occupied = other.m_slots_occupied;
    }

    template <typename T>
    CAMY_INLINE HashMap<T>::HashMap(THashMap&& other)
        : TBaseHashMap(other)
        , m_entries(nullptr)
        , m_capacity(0)
        , m_slots_occupied(0)
    {
        API::swap(m_entries, other.m_entries);
        API::swap(m_capacity, other.m_capacity);
        API::swap(m_slots_occupied, other.m_slots_occupied);
    }

    template <typename T>
    CAMY_INLINE typename HashMap<T>::THashMap& HashMap<T>::operator=(const THashMap& other)
    {
        TBaseHashMap::operator=(other);
        _deallocate(m_entries);
        m_capacity = m_slots_occupied = 0;

        m_entries = _allocate_align_same(other.m_capacity, other.m_entries);
        memcpy(m_entries, other.m_entries, ENTRY_SIZE * other.m_capacity);
        m_capacity = other.m_capacity;
        m_slots_occupied = other.m_slots_occupied;
        return *this;
    }

    template <typename T>
    CAMY_INLINE typename HashMap<T>::THashMap& HashMap<T>::operator=(THashMap&& other)
    {
        TBaseHashMap::operator=(other);
        _deallocate(m_entries);
        m_capacity = m_slots_occupied = 0;

        API::swap(m_entries, other.m_entries);
        API::swap(m_capacity, other.m_capacity);
        API::swap(m_slots_occupied, other.m_slots_occupied);
        return *this;
    }

    template <typename T>
    CAMY_INLINE typename HashMap<T>::TPtr HashMap<T>::find(TKey key)
    {
        return _find(key);
    }

    template <typename T>
    CAMY_INLINE typename HashMap<T>::TConstPtr HashMap<T>::find(TKey key) const
    {
        return (TConstPtr)_find(key);
    }

    template <typename T>
    template <typename... Ts>
    CAMY_INLINE typename HashMap<T>::TPtr HashMap<T>::emplace(TKey key, Ts&&... args)
    {
        return _insert(key, nullptr, std::forward<Ts>(args)...);
    }

    template <typename T>
    CAMY_INLINE typename HashMap<T>::TPtr HashMap<T>::insert(TKey key, TConstRef val)
    {
        return _insert(key, nullptr, val);
    }

    template <typename T>
    CAMY_INLINE void HashMap<T>::remove(TKey key)
    {
        _remove(key);
    }

    template <typename T>
    CAMY_INLINE void HashMap<T>::clear()
    {
        _clear();
    }

    template <typename T>
    CAMY_INLINE rsize HashMap<T>::count()
    {
        return m_slots_occupied;
    }

    template <typename T>
    CAMY_INLINE rsize HashMap<T>::capacity() const
    {
        return m_capacity;
    }

    template <typename T>
    CAMY_INLINE bool HashMap<T>::empty() const
    {
        return m_slots_occupied == 0;
    }

    template <typename T>
    CAMY_INLINE typename HashMap<T>::Entry* HashMap<T>::_allocate_align_explicit(rsize n,
                                                                                 rsize alignment)
    {
        return (Entry*)API::allocate(CAMY_ALLOC(ENTRY_SIZE * n, alignment));
    }

    template <typename T>
    CAMY_INLINE typename HashMap<T>::Entry* HashMap<T>::_allocate_align_same(rsize n,
                                                                             void* src_alignment)
    {
        return (Entry*)API::allocate(CAMY_ALLOC_SRC(ENTRY_SIZE * n, src_alignment));
    }

    template <typename T>
    CAMY_INLINE void HashMap<T>::_deallocate(Entry* ptr)
    {
        API::deallocate(ptr);
    }

    template <typename T>
    CAMY_INLINE void HashMap<T>::_reset_entries(Entry* beg, rsize n)
    {
        for (rsize i = 0; i < n; ++i)
            beg[i].key = INVALID_KEY;
    }

    template <typename T>
    CAMY_INLINE rsize HashMap<T>::_hash(TKey key) const
    {
        return key % m_capacity;
    }

    template <typename T>
    CAMY_INLINE void HashMap<T>::_make_space(rsize slots)
    {
        if (m_slots_occupied > (m_capacity / 2))
        {
            Entry* old_buffer = m_entries;
            rsize new_capacity = API::max(m_capacity * 2, DEFAULT_CAPACITY);
            m_entries = _allocate_align_same(new_capacity, old_buffer);

            memcpy(m_entries, old_buffer, ENTRY_SIZE * m_capacity); // TODO: Copy constructor ?
            _reset_entries(m_entries + m_capacity, new_capacity - m_capacity);

            _deallocate(old_buffer);
            m_capacity = new_capacity;
        }
    }

    // Enabled implementations
    // TODO: Can avoid some duplicated code in insert / find
    template <typename T>
    template <typename U>
    CAMY_INLINE typename HashMap<T>::TPtr HashMap<T>::_find(TKey key, HMEnableExtTrue<U>*)
    {
        uint64 base = _hash(key);
        uint64* idx = nullptr;
        for (rsize i = 0; i < m_capacity; ++i)
        {
            Entry& cur = m_entries[(base + i) % m_capacity];
            if (cur.key == INVALID_KEY)
                break;
            if (cur.key == key)
            {
                idx = &cur.val;
                break;
            }
        }
        if (idx == nullptr)
            return nullptr;
        return &m_ext_buffer.get(*idx);
    }

    template <typename T>
    template <typename U>
    CAMY_INLINE typename HashMap<T>::TPtr HashMap<T>::_find(TKey key, HMEnableExtFalse<U>*)
    {
        uint64 base = _hash(key);
        for (rsize i = 0; i < m_capacity; ++i)
        {
            Entry& cur = m_entries[(base + i) % m_capacity];
            if (cur.key == INVALID_KEY)
                break;
            if (cur.key == key)
                return (TPtr)&cur.val;
        }
        return nullptr;
    }

    template <typename T>
    template <typename U, typename... Ts>
    CAMY_INLINE typename HashMap<T>::TPtr
    HashMap<T>::_insert(TKey key, HMEnableExtTrue<U>*, Ts&&... args)
    {
        _make_space(m_slots_occupied + 1);

        rsize base = _hash(key);
        for (rsize i = 0; i < m_capacity; ++i)
        {
            Entry& cur = m_entries[(base + i) % m_capacity];
            if (cur.key == key)
            {
                CL_WARN("Error inserting duplicated item: ", key);
                return nullptr;
            }

            if (cur.key == INVALID_KEY)
            {
                cur.key = key;
                cur.val = (uint64)m_ext_buffer.allocate(std::forward<Ts>(args)...);
                ++m_slots_occupied;
                return &m_ext_buffer.get((rsize)cur.val);
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename U, typename... Ts>
    CAMY_INLINE typename HashMap<T>::TPtr
    HashMap<T>::_insert(TKey key, HMEnableExtFalse<U>*, Ts&&... args)
    {
        _make_space(m_slots_occupied + 1);

        rsize base = _hash(key);
        for (rsize i = 0; i < m_capacity; ++i)
        {
            Entry& cur = m_entries[(base + i) % m_capacity];
            if (cur.key == key)
            {
                CL_WARN("Error inserting duplicated item: ", key);
                return nullptr;
            }

            if (cur.key == INVALID_KEY)
            {
                cur.key = key;
                new (&cur.val) T(std::forward<Ts>(args)...);
                ++m_slots_occupied;
                return (TPtr)&cur.val;
            }
        }

        return nullptr;
    }

    template <typename T>
    template <typename U>
    CAMY_INLINE void HashMap<T>::_remove(TKey key, HMEnableExtTrue<U>*)
    {
        rsize base = _hash(key);
        for (rsize i = 0; i < m_capacity; ++i)
        {
            Entry& cur = m_entries[(base + i) % m_capacity];
            if (cur.key == INVALID_KEY)
                break;

            if (cur.key == key)
            {
                cur.key = INVALID_KEY;
                m_ext_buffer.deallocate((rsize)cur.val);
                --m_slots_occupied;
                return;
            }
        }

        CL_ERR("Error removing key: ", key, " not found");
    }

    template <typename T>
    template <typename U>
    CAMY_INLINE void HashMap<T>::_remove(TKey key, HMEnableExtFalse<U>*)
    {
        rsize base = _hash(key);
        for (rsize i = 0; i < m_capacity; ++i)
        {
            Entry& cur = m_entries[(base + i) % m_capacity];
            if (cur.key == INVALID_KEY)
                break;

            if (cur.key == key)
            {
                cur.key = INVALID_KEY;
                --m_slots_occupied;
                return;
            }
        }

        CL_ERR("Error removing key: ", key, " not found");
    }

    template <typename T>
    template <typename U>
    CAMY_INLINE void HashMap<T>::_clear(HMEnableExtTrue<U>*)
    {
        m_slots_occupied = 0;
        for (rsize i = 0; i < m_capacity; ++i)
            m_entries[i].key = INVALID_KEY;
        m_ext_buffer.clear();
    }

    template <typename T>
    template <typename U>
    CAMY_INLINE void HashMap<T>::_clear(HMEnableExtFalse<U>*)
    {
        m_slots_occupied = 0;
        for (rsize i = 0; i < m_capacity; ++i)
            m_entries[i].key = INVALID_KEY;
    }
}