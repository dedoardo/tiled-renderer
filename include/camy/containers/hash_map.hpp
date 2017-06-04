/* int2int_map.hpp
 *
 * Copyright (C) 2017 Edoardo Dominici
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

 // camy
#include <camy/core.hpp>
#include <camy/containers/pool.hpp>

// C++ STL
#include <type_traits>

namespace camy
{
	namespace API
	{
		// Thanks: http://stackoverflow.com/questions/2111667/compile-time-string-hashing
		uint64 constexpr hash_str_ct(const char8* input)
		{
			return *input ? (uint64)(*input) + 33 * hash_str_ct(input + 1) : 5381;
		}
	}

#define CAMY_HM_STR(str) ::camy::API::hash_str_ct((str))

	// Gotta be honest, wanted to enable_if based on sizeof(T) <= sizeof(uint64) but didnt manage to do it
	// as overloading the constructor kept giving me internal compiler error when passing a default parameter to the constructor. 
	// The explicitly_convertible is more type-friendly though.
	// https://stackoverflow.com/questions/16893992/check-if-type-can-be-explicitly-converted
	template <typename T, typename U>
	struct is_explicitly_convertible
	{
		enum { value = std::is_constructible_v<T, U> && !std::is_convertible_v<U, T>};
	};

	template <typename T>
	using HMEnableExtTrue = typename std::enable_if<!std::is_convertible<T, uint64>::value>::type;

	template <typename T>
	using HMEnableExtFalse = typename std::enable_if<std::is_convertible<T, uint64>::value>::type;

	template <typename T, typename Enable = void>
	class BaseHashMap;

	template <typename T>
	struct BaseHashMap<T, HMEnableExtFalse<T>>
	{
		BaseHashMap(rsize capacity, rsize alignment) { } 
	};

	template <typename T>
	struct BaseHashMap<T, HMEnableExtTrue<T>>
	{ 
		BaseHashMap(rsize capacity, rsize alignment) : m_ext_buffer(capacity) { } // TODO: pass alignment
		Pool<T> m_ext_buffer;
	};

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
		template <typename U = T>
		explicit HashMap(rsize capacity = DEFAULT_CAPACITY, rsize alignment = DEFAULT_ALIGNMENT, HMEnableExtTrue<U>* = nullptr);

		template <typename U = T>
		explicit HashMap(rsize capacity = DEFAULT_CAPACITY, rsize alignment = DEFAULT_ALIGNMENT, HMEnableExtFalse<U>* = nullptr);

		~HashMap();

		HashMap(const THashMap& other);
		HashMap(THashMap&& other);

		THashMap& operator=(const THashMap& other);
		THashMap& operator=(THashMap&& other);

		TPtr find(TKey key);
		TConstPtr find(TKey key)const;
		
		TPtr find(const char8* str);
		TConstPtr find(const char8* str)const;

		TPtr insert(TKey key, TConstRef val);
		TPtr insert(const char8* str, TConstRef val);

		void remove(TKey key);
		void remove(const char8* str);

		void clear();

		rsize count();
		rsize capacity()const;
		bool empty()const;

	private:
		uint64 _hash(TKey key)const;
		void _make_space(rsize slots);

		template <typename U = T>
		TPtr _find(TKey key, HMEnableExtTrue<U>* = nullptr);

		template <typename U = T>
		TPtr _find(TKey key, HMEnableExtFalse<U>* = nullptr);

		template <typename U = T>
		TPtr _insert(TKey key, TConstRef val, HMEnableExtTrue<U>* = nullptr);

		template <typename U = T>
		TPtr _insert(TKey key, TConstRef val, HMEnableExtFalse<U>* = nullptr);

		template <typename U = T>
		void _remove(TKey key, HMEnableExtTrue<U>* = nullptr);

		template <typename U = T>
		void _remove(TKey key, HMEnableExtFalse<U>* = nullptr);

		template <typename U = T>
		void _clear(HMEnableExtTrue<U>* = nullptr);

		template <typename U = T>
		void _clear(HMEnableExtFalse<U>* = nullptr);

		struct Entry
		{
			TKey key;
			uint64 val;
		};
		static const rsize ENTRY_SIZE = sizeof(Entry);

		Entry*  m_entries;
		rsize	m_capacity;
		rsize	m_slots_occupied;
	};

	template <typename T>
	template <typename U>
	CAMY_INLINE HashMap<T>::HashMap(rsize capacity, rsize alignment, HMEnableExtTrue<U>*) :
		TBaseHashMap(capacity, alignment),
		m_entries(nullptr),
		m_capacity(0),
		m_slots_occupied(0)
	{
		m_entries = (Entry*)API::allocate(CAMY_ALLOC(ENTRY_SIZE * capacity, alignment));
		for (rsize i = 0; i < capacity; ++i)
			m_entries[i].key = INVALID_KEY;
		m_capacity = capacity;
	}

	template <typename T>
	template <typename U>
	CAMY_INLINE HashMap<T>::HashMap(rsize capacity, rsize alignment, HMEnableExtFalse<U>*) :
		TBaseHashMap(capacity, alignment),
		m_entries(nullptr),
		m_capacity(0),
		m_slots_occupied(0)
	{
		m_entries = (Entry*)API::allocate(CAMY_ALLOC(ENTRY_SIZE * capacity, alignment));
		for (rsize i = 0; i < capacity; ++i)
			m_entries[i].key = INVALID_KEY;
		m_capacity = capacity;
	}

	template <typename T>
	CAMY_INLINE HashMap<T>::~HashMap()
	{

	}

	template <typename T>
	CAMY_INLINE HashMap<T>::HashMap(const THashMap& other) :
		TBaseHashMap(other)
	{

	}

	template <typename T>
	CAMY_INLINE HashMap<T>::HashMap(THashMap&& other) :
		TBaseHashMap(other)
	{

	}

	template <typename T>
	CAMY_INLINE typename HashMap<T>::THashMap& HashMap<T>::operator=(const THashMap& other)
	{
		TBaseHashMap::operator=(other);
		return *this;
	}

	template <typename T>
	CAMY_INLINE typename HashMap<T>::THashMap& HashMap<T>::operator=(THashMap&& other)
	{
		TBaseHashMap::operator=(other);
		return *this;
	}

	template <typename T>
	CAMY_INLINE typename HashMap<T>::TPtr HashMap<T>::find(TKey key)
	{
		return _find(key);
	}

	template <typename T>
	CAMY_INLINE typename HashMap<T>::TConstPtr HashMap<T>::find(TKey key)const
	{
		return (TConstPtr)_find(key);
	}

	template <typename T>
	CAMY_INLINE typename HashMap<T>::TPtr HashMap<T>::find(const char8* str)
	{
		return _find(API::hash_str_ct(str));
	}

	template <typename T>
	CAMY_INLINE typename HashMap<T>::TConstPtr HashMap<T>::find(const char8* str)const
	{
		return (TConstPtr)_find(API::hash_str_ct(str));
	}

	template <typename T>
	CAMY_INLINE typename HashMap<T>::TPtr HashMap<T>::insert(TKey key, TConstRef val)
	{
		return _insert(key, val);
	}

	template <typename T>
	CAMY_INLINE typename HashMap<T>::TPtr HashMap<T>::insert(const char8* str, TConstRef val)
	{
		return _insert(API::hash_str_ct(str), val);
	}

	template <typename T>
	CAMY_INLINE void HashMap<T>::remove(TKey key)
	{
		_remove(key);
	}

	template <typename T>
	CAMY_INLINE void HashMap<T>::remove(const char8* str)
	{
		_remove(API::hash_str_ct(str));
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
	CAMY_INLINE rsize HashMap<T>::capacity()const
	{
		return m_capacity;
	}
	
	template <typename T>
	CAMY_INLINE bool HashMap<T>::empty()const
	{
		return m_slots_occupied == 0;
	}

	template <typename T>
	CAMY_INLINE uint64 HashMap<T>::_hash(TKey key) const
	{
		return key % m_capacity;
	}

	template <typename T>
	CAMY_INLINE void HashMap<T>::_make_space(rsize slots)
	{
		if (m_slots_occupied > m_capacity / 2)
		{
			Entry* old_buffer = m_entries;
			rsize  new_capacity = API::min(m_capacity * 2, DEFAULT_CAPACITY);
			m_entries = (Entry*)API::allocate(CAMY_ALLOC_SRC(ENTRY_SIZE * new_capacity, old_buffer));
			memcpy(m_entries, old_buffer, ENTRY_SIZE * m_capacity); // TODO: Copy constructor ?
			API::deallocate(old_buffer);
			m_capacity = new_capacity;
		}
	}

	// Enabled implementations
	// TODO: Can avoid some duplicated code in insert / find
	template <typename T>
	template <typename U = T>
	CAMY_INLINE typename HashMap<T>::TPtr HashMap<T>::_find(TKey key, HMEnableExtTrue<U>* = nullptr)
	{
		uint64 base = _hash(key);
		uint64* idx = nullptr;
		for (rsize i = 0; i < m_capacity; ++i)
		{
			Entry& cur = m_entries[(base + i) % m_capacity];
			if (cur.key == INVALID_KEY) break;
			if (cur.key == key)
			{
				idx = &cur.val;
				break;
			}
		}
		if (idx == nullptr) return nullptr;
		return &m_ext_buffer.get(*idx);
	}

	template <typename T>
	template <typename U = T>
	CAMY_INLINE typename HashMap<T>::TPtr HashMap<T>::_find(TKey key, HMEnableExtFalse<U>* = nullptr)
	{
		uint64 base = _hash(key);
		for (rsize i = 0; i < m_capacity; ++i)
		{
			Entry& cur = m_entries[(base + i) % m_capacity];
			if (cur.key == INVALID_KEY) break;
			if (cur.key == key) return (TPtr)&cur.val;
		}
		return nullptr;
	}

	template <typename T>
	template <typename U = T>
	CAMY_INLINE typename HashMap<T>::TPtr HashMap<T>::_insert(TKey key, TConstRef val, HMEnableExtTrue<U>* = nullptr)
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
				cur.val = (uint64)m_ext_buffer.allocate(val);
				++m_slots_occupied;
				return &m_ext_buffer.get((rsize)cur.val);
			}
		}

		return nullptr;
	}

	template <typename T>
	template <typename U = T>
	CAMY_INLINE typename HashMap<T>::TPtr HashMap<T>::_insert(TKey key, TConstRef val, HMEnableExtFalse<U>* = nullptr)
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
				cur.val = val;
				++m_slots_occupied;
				return (TPtr)&cur.val;
			}
		}

		return nullptr;
	}

	template <typename T>
	template <typename U = T>
	CAMY_INLINE void HashMap<T>::_remove(TKey key, HMEnableExtTrue<U>* = nullptr) 
	{
		rsize base = _hash(key);
		for (rsize i = 0; i < m_capacity; ++i)
		{
			Entry& cur = m_entries[(base + i) % m_capacity];
			if (cur.key == INVALID_KEY) break;

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
	template <typename U = T>
	CAMY_INLINE void HashMap<T>::_remove(TKey key, HMEnableExtFalse<U>* = nullptr)
	{
		rsize base = _hash(key);
		for (rsize i = 0; i < m_capacity; ++i)
		{
			Entry& cur = m_entries[(base + i) % m_capacity];
			if (cur.key == INVALID_KEY) break;

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
	template <typename U = T>
	CAMY_INLINE void HashMap<T>::_clear(HMEnableExtTrue<U>* = nullptr)
	{
		m_slots_occupied = 0;
		for (rsize i = 0; i < m_capacity; ++i)
			m_entries[i].key = INVALID_KEY;
		m_ext_buffer.clear();
	}

	template <typename T>
	template <typename U = T>
	CAMY_INLINE void HashMap<T>::_clear(HMEnableExtFalse<U>* = nullptr)
	{
		m_slots_occupied = 0;
		for (rsize i = 0; i < m_capacity; ++i)
			m_entries[i].key = INVALID_KEY;
	}
}