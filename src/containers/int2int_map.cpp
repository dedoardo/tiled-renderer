/* int2int_map.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/containers/int2int_map.hpp>

// camy
#include <camy/system.hpp>

namespace camy
{
    // Thanks: http://stackoverflow.com/questions/2111667/compile-time-string-hashing
    uint64 constexpr key(const char8* input)
    {
        return *input ? (uint64)(*input) + 33 * key(input + 1) : 5381;
    }

    Int2IntMap::Int2IntMap(rsize initial_capacity)
        : m_buffer(nullptr), m_capacity(0), m_slots_occupied(0)
    {
        m_capacity = initial_capacity;
        m_buffer = (Entry*)API::allocate(CAMY_UALLOC(kEntrySize * m_capacity));
        for (rsize i = 0; i < m_capacity; ++i)
            m_buffer[i].key = kInvalidKey;
    }

    Int2IntMap::~Int2IntMap() { API::deallocate(m_buffer); }

    Int2IntMap::Int2IntMap(Int2IntMap&& other)
    {
        m_buffer = other.m_buffer;
        m_capacity = other.m_capacity;
        m_slots_occupied = other.m_slots_occupied;
        other.m_buffer = nullptr;
        other.m_capacity = 0;
        other.m_slots_occupied = 0;
    }

    Int2IntMap::Int2IntMap(Int2IntMap& other)
    {
        m_buffer = (Entry*)API::allocate(CAMY_UALLOC(kEntrySize * other.m_capacity));
        m_capacity = other.m_capacity;
        m_slots_occupied = 0;
        std::memcpy(m_buffer, other.m_buffer, kEntrySize * m_capacity);
    }

    Int2IntMap& Int2IntMap::operator=(Int2IntMap&& other)
    {
        m_buffer = other.m_buffer;
        m_capacity = other.m_capacity;
        m_slots_occupied = other.m_slots_occupied;
        other.m_buffer = nullptr;
        other.m_capacity = 0;
        other.m_slots_occupied = 0;
        return *this;
    }

    Int2IntMap& Int2IntMap::operator=(Int2IntMap& other)
    {
        m_buffer = (Entry*)API::allocate(CAMY_UALLOC(kEntrySize * other.m_capacity));
        m_capacity = other.m_capacity;
        m_slots_occupied = 0;
        std::memcpy(m_buffer, other.m_buffer, kEntrySize * m_capacity);
        return *this;
    }

    typename Int2IntMap::ValueType* Int2IntMap::operator[](KeyType key)
    {
        CAMY_ASSERT(m_buffer != nullptr);

        rsize base = to_idx(key);
        for (rsize i = 0; i < m_capacity; ++i)
        {
            Entry& cur = m_buffer[(base + i) & (m_capacity - 1)];
            if (cur.key == kInvalidKey) break;
            if (cur.key == key) return &cur.val;
        }
        return nullptr;
    }

    typename const Int2IntMap::ValueType* Int2IntMap::operator[](KeyType key) const
    {
        CAMY_ASSERT(m_buffer != nullptr);

        rsize base = to_idx(key);
        for (rsize i = 0; i < m_capacity; ++i)
        {
            Entry& cur = m_buffer[(base + i) & (m_capacity - 1)];
            if (cur.key == kInvalidKey) break;
            if (cur.key == key) return &cur.val;
        }
        return nullptr;
    }

    Int2IntMap::ValueType* Int2IntMap::operator[](const char8* name) { return (*this)[key(name)]; }

    const Int2IntMap::ValueType* Int2IntMap::operator[](const char8* name) const
    {
        return (*this)[key(name)];
    }

    typename Int2IntMap::ValueType* Int2IntMap::insert(KeyType key, ValueType val)
    {
        CAMY_ASSERT(m_buffer != nullptr);

        _realloc_if_needed();

        rsize base = to_idx(key);
        for (rsize i = 0; i < m_capacity; ++i)
        {
            Entry& cur = m_buffer[(base + i) & (m_capacity - 1)];
            if (cur.key == key)
            {
                CL_WARN("Inserting item: ", key, " already present");
                return nullptr;
            }

            if (cur.key == kInvalidKey)
            {
                cur.key = key;
                cur.val = val;
                ++m_slots_occupied;
                return &cur.val;
            }
        }

        return nullptr;
    }

    Int2IntMap::ValueType* Int2IntMap::insert(const char8* name, ValueType val)
    {
        return insert(key(name), val);
    }

    void Int2IntMap::remove(KeyType key)
    {
        CAMY_ASSERT(m_buffer != nullptr);

        rsize base = to_idx(key);
        for (rsize i = 0; i < m_capacity; ++i)
        {
            Entry& cur = m_buffer[(base + i) & (m_capacity - 1)];
            if (cur.key == kInvalidKey) break;

            if (cur.key == key)
            {
                cur.key = kInvalidKey;
                --m_slots_occupied;
                return;
            }
        }

        CL_ERR("Failed to remove: ", key, " from Int2IntMap as it doesn't exist");
    }

    void Int2IntMap::remove(const char8* name) { remove(key(name)); }

    void Int2IntMap::clear()
    {
        m_slots_occupied = 0;
        for (rsize i = 0; i < m_capacity; ++i)
            m_buffer[i].key = kInvalidKey;
    }

    void Int2IntMap::_realloc_if_needed()
    {
        if (m_slots_occupied >= m_capacity / 2)
        {
            Entry* old_buffer = m_buffer;
            rsize new_capacity = m_capacity * 2;
            m_buffer = (Entry*)API::allocate(CAMY_UALLOC(sizeof(Entry) * new_capacity));
            std::memcpy(m_buffer, old_buffer, sizeof(Entry) * m_capacity);

            API::deallocate(old_buffer);
            m_capacity = new_capacity;
        }
    }
    rsize Int2IntMap::to_idx(KeyType key) const
    {
        CAMY_ASSERT(API::is_power_of_two(m_capacity));
        return key & (m_capacity - 1);
    }
}