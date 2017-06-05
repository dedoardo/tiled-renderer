/* pages.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/system.hpp>

namespace camy
{
	struct CAMY_API Page
	{
		Page(rsize size, rsize alignment);
		virtual ~Page();

		Page(const Page& other) = delete; 
		Page(Page&&);

		Page& operator=(const Page& other) = delete;
		Page& operator=(Page&&) = delete;
		
		Page* previous;
		Page* next;
		byte* buffer;
		rsize byte_size;
	};

	CAMY_INLINE Page::Page(rsize size, rsize alignment) :
		previous(nullptr),
		next(nullptr),
		buffer(nullptr),
		byte_size(0)
	{
		buffer = (byte*)API::allocate(CAMY_ALLOC(size, alignment));
		byte_size = size;
	}

	CAMY_INLINE Page::~Page()
	{
		API::deallocate(buffer);
		previous = nullptr;
		next = nullptr;
		byte_size = 0;
	}

    CAMY_INLINE Page::Page(Page&& other)
    {
        byte_size = other.byte_size;
        buffer = other.buffer;
        previous = other.previous;
        next = other.next;
        other.byte_size = 0;
        other.buffer = nullptr;
        other.previous = nullptr;
        other.next = nullptr;
    }

	template <typename T>
	struct CAMY_API FreelistPage final : public Page
	{
	public:
		static const rsize ELEMENT_SIZE = sizeof(T);

		using TFreelistPage = FreelistPage<T>;
		using TValue = T;
		using TPtr = T*;
		using TConstPtr = const T*;
		using TRef = T&;
		using TConstRef = const T&;

	public:
		FreelistPage(rsize size, rsize alignment);
		~FreelistPage();

		FreelistPage(const TFreelistPage&) = delete;
		FreelistPage(TFreelistPage&& other);

		TFreelistPage& operator=(const TFreelistPage&) = delete;
		TFreelistPage& operator=(TFreelistPage&& other) = delete;

		template <typename ...Ts>
		TPtr allocate(Ts&& ...args);

		void deallocate(TPtr& ptr);
		void clear();

		bool is_in_range(TConstPtr ptr)const;

	private:
		void _reset();

		struct StoredType
		{
			T data;
			StoredType* next;
			uint8 flags;
		};
		static const rsize STORED_ELEMENT_SIZE = sizeof(StoredType);

		StoredType* next_free;
		rsize size;
	};

    template <typename T>
    CAMY_INLINE FreelistPage<T>::FreelistPage(rsize size, rsize alignment) :
        Page(size * STORED_ELEMENT_SIZE, alignment),
		next_free(nullptr),
		size(size)
    {
        CAMY_ASSERT(element_count > 0);
        next_free = (StoredType*)buffer;
        _reset();
    }

    template <typename T>
    CAMY_INLINE FreelistPage<T>::~FreelistPage()
    {
        next_free = (StoredType*)buffer;
        clear();
    }

    template <typename T>
    CAMY_INLINE FreelistPage<T>::FreelistPage(TFreelistPage&& other) : Page(other)
    {
        size = other.size;
        next_free = other.next_free;
        other.size = 0;
        other.next_free = nullptr;
    }

    template <typename T>
    template <typename... Ts>
    CAMY_INLINE typename FreelistPage<T>::TPtr FreelistPage<T>::allocate(Ts&&... args)
    {
        // Page is full
        if (next_free == nullptr) return nullptr;

        // Removing element from head of list
        StoredType* next = next_free;
        next_free = next->next;

        // Initializing new element
        new (&next->data) T(std::forward<Ts>(args)...);
        next->next = nullptr;
        next->flags = 1;

        return &next->data;
    }

    template <typename T>
    CAMY_INLINE void FreelistPage<T>::deallocate(TPtr& ptr)
    {
        // Calling destructor
        StoredType* old = (StoredType*)ptr;
        old->data.~T();
        old->flags = 0;
        ptr = nullptr;

        // Adding
        old->next = next_free;
        next_free = old;
    }

	template <typename T>
	CAMY_INLINE void FreelistPage<T>::clear()
	{
		// Destructing only allocated elements
		for (rsize i = 0; i < size; ++i)
		{
			StoredType* cur = ((StoredType*)buffer) + i;
			if (cur->flags == 1) cur->data.~T();
		}

		_reset();
	}

    template <typename T>
    CAMY_INLINE bool FreelistPage<T>::is_in_range(TConstPtr ptr) const
    {
        StoredType* cur_ptr = (StoredType*)ptr;
        StoredType* end = ((StoredType*)buffer) + size;
        if (cur_ptr >= (StoredType*)buffer && cur_ptr < end) return true;
        return false;
    }

    template <typename T>
    CAMY_INLINE void FreelistPage<T>::_reset()
    {
        StoredType* cur = (StoredType*)buffer;
        for (rsize i = 0; i < size - 1; ++i)
        {
            cur->next = cur + 1;
            cur->flags = 0;
            ++cur;
        }
        cur->next = nullptr;
        cur->flags = 0;
    }

	// Untyped, bumps the pointer until full
    template <typename T>
    struct CAMY_API LinearPage final : public Page
    {
      public:
		static const rsize ELEMENT_SIZE = sizeof(T);

		using TLinearPage = LinearPage<T>;
		using TValue = T;
		using TPtr = T*;
		using TConstPtr = const T*;
		using TRef = T&;
		using TConstRef = const T&;

      public:
        LinearPage(rsize size, rsize alignment);
        ~LinearPage();

		LinearPage(TLinearPage& other) = delete;
		LinearPage(TLinearPage&& other);

		TLinearPage& operator=(TLinearPage& other) = delete;
		TLinearPage& operator=(TLinearPage&& other) = delete;

        TPtr _next(); // TODO: Fix this name conflict
        TPtr next_array(rsize count);
        void reset();

      private:
        const rsize size;
        rsize counter;
    };

    template <typename T>
    CAMY_INLINE LinearPage<T>::LinearPage(rsize size, rsize alignment)
        : Page(size * ELEMENT_SIZE, alignment), 
		size(size), 
		counter(0)
    {
    }

    template <typename T>
    CAMY_INLINE LinearPage<T>::~LinearPage()
    {
        counter = 0;
    }

    template <typename T>
    CAMY_INLINE LinearPage<T>::LinearPage(TLinearPage&& other) : 
		Page(other)
    {
        counter = other.counter;
        other.counter = 0;
    }

    template <typename T>
    CAMY_INLINE typename LinearPage<T>::TPtr LinearPage<T>::_next()
    {
        if (counter == size) return nullptr;

        T* ret = &((T*)buffer)[counter++];
        new (ret) T();
        return ret;
    }

    template <typename T>
    CAMY_INLINE typename LinearPage<T>::TPtr LinearPage<T>::next_array(rsize count)
    {
        CAMY_ASSERT(count * ELEMENT_SIZE <= byte_size);

        if (counter + count >= size) return nullptr;

        TPtr ret = &((TPtr)buffer)[counter];
        rsize i = 0;
        while (i < count)
            new (&ret[i++]) T();
        counter += count;
        return ret;
    }

    template <typename T>
    CAMY_INLINE void LinearPage<T>::reset()
    {
        counter = 0;
    }
}