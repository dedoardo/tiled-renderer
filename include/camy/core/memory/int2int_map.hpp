/* int2int_map.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/base.hpp>
#include <camy/core/memory/alloc.hpp>

namespace camy
{
    /*
        Simple map that maps 64 bits integers to 64 bit integers. 
        Key can be an hashed string.
    */
    class camy_api Int2IntMap final
    {
    public:
        using KeyType = uint64;
        static const KeyType kInvalidKey = (KeyType)-1;
        using ValueType = uint64;

    public:
        Int2IntMap(rsize initial_capacity = 32); 
        ~Int2IntMap();

        Int2IntMap(Int2IntMap&& other);
        Int2IntMap(Int2IntMap& other);

        Int2IntMap& operator=(Int2IntMap&& other);
        Int2IntMap& operator=(Int2IntMap& other);

        ValueType*       operator[](KeyType key);
        const ValueType* operator[](KeyType key)const;
        ValueType*       operator[](const char8* name);
        const ValueType* operator[](const char8* name)const;

        ValueType*  insert(KeyType key, ValueType val);
        ValueType*  insert(const char8* name, ValueType val);
        void        remove(KeyType key);
        void        remove(const char8* name);
        void clear();

    private:
        void _realloc_if_needed();
        rsize to_idx(KeyType key)const;

        struct Entry
        {
            uint64 key = kInvalidKey;
            uint64 val;
        };
        static const rsize kEntrySize = sizeof(Entry);

        Entry* m_buffer;
        rsize m_capacity;
        rsize m_slots_occupied;
    };
}