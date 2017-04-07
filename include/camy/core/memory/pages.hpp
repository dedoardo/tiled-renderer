/* pages.hpp
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
		Struct: Page
			General purpose untyped CPU memory page that allow for chaining 
			in double linked list.
	*/
	template <uint16 kAlignment = kDefaultAlignment>
	struct Page
	{
	public:
		
		Page(rsize size);
		virtual ~Page();

		Page(Page&& other);
		Page(Page& other) = delete; // Paged non-move copyctor / assignment operators
									// are explicitly deleted.

		Page& operator=(Page&& other);
		Page& operator=(Page& other) = delete;
				
		rsize byte_size;
		byte* buffer;
		Page* previous;
		Page* next;
	};

	template <uint16 kAlignment>
	inline Page<kAlignment>::Page(rsize size) :
		byte_size(size),
		buffer(nullptr),
		previous(nullptr),
		next(nullptr)
	{
		buffer = (byte*)allocate(camy_loc, size, kAlignment);
		byte_size = size;
		camy_assert(buffer != nullptr);
	}

	template <uint16 kAlignment>
	inline Page<kAlignment>::~Page()
	{
		deallocate(buffer);
		previous = nullptr;
		next = nullptr;
		byte_size = 0;
	}

	template<uint16 kAlignment>
	inline Page<kAlignment>::Page(Page && other)
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

	template<uint16 kAlignment>
	inline Page<kAlignment>& Page<kAlignment>::operator=(Page && other)
	{
		byte_size = other.byte_size;
		buffer = other.buffer;
		previous = other.previous;
		next = other.next;
		other.byte_size = 0;
		other.buffer = nullptr;
		other.previous = nullptr;
		other.next = nullptr;
		return *this;
	}

	/*
		Struct: FreelistPage
			Typed page that acts as a pool. Used as base for pool containers.
			It calls destructor upon release
	*/
	template <typename ElementType, uint16 kAlignment = kDefaultAlignment>
	struct camy_api FreelistPage final : public Page<kAlignment>
	{
	public:
		static const rsize kElementSize = sizeof(ElementType);

	public:
		FreelistPage(rsize element_count);
		~FreelistPage();

		FreelistPage(FreelistPage&& other);
		FreelistPage(FreelistPage& other) = delete;

		FreelistPage& operator=(FreelistPage&& other);
		FreelistPage& operator=(FreelistPage& other) = delete;

		template <typename ...CtorArgs>
		ElementType* allocate(CtorArgs&& ...ctor_args);

		void deallocate(ElementType*& ptr);
		void clear();

		bool is_in_range(const ElementType* ptr)const;

	private:
		void _reset();

		struct _StoredType
		{
			ElementType data;
			_StoredType* next;
			uint8 flags; // TODO: Might switch to indices
		};

		rsize element_count;
		_StoredType* next_free;
	};

	template<typename ElementType, uint16 kAlignment>
	inline FreelistPage<ElementType, kAlignment>::FreelistPage(rsize element_count) :
		Page(element_count * sizeof(_StoredType)),
		next_free(nullptr),
		element_count(element_count)
	{
		camy_assert(element_count > 0);
		next_free = (_StoredType*)buffer;

		_reset();
	}

	template<typename ElementType, uint16 kAlignment>
	inline FreelistPage<ElementType, kAlignment>::~FreelistPage()
	{
		next_free = (_StoredType*)buffer;
		clear();
	}

	template<typename ElementType, uint16 kAlignment>
	inline FreelistPage<ElementType, kAlignment>::FreelistPage(FreelistPage && other) :
		Page(other)
	{
		element_count = other.element_count;
		next_free = other.next_free;
		other.element_count = 0;
		other.next_free = nullptr;
	}

	template<typename ElementType, uint16 kAlignment>
	inline FreelistPage<ElementType, kAlignment>& FreelistPage<ElementType, kAlignment>::operator=(FreelistPage && other)
	{
		Page::operator=(other);
		element_count = other.element_count;
		next_free = other.next_free;
		other.element_count = 0;
		other.next_free = nullptr;
		return *this;
	}

	template<typename ElementType, uint16 kAlignment>
	template<typename ...CtorArgs>
	inline ElementType * FreelistPage<ElementType, kAlignment>::allocate(CtorArgs && ...ctor_args)
	{
		// Page is full
		if (next_free == nullptr)
			return nullptr;

		// Removing element from head of list
		_StoredType* next = next_free;
		next_free = next->next;

		// Initializing new element
		new (&next->data) ElementType(std::forward<CtorArgs>(ctor_args)...);
		next->next = nullptr;
		next->flags = 1;

		return &next->data;
	}

	template<typename ElementType, uint16 kAlignment>
	inline void FreelistPage<ElementType, kAlignment>::deallocate(ElementType *& ptr)
	{
		// Calling destructor
		_StoredType* old = (_StoredType*)ptr;
		old->data.~ElementType();
		old->flags = 0;
		ptr = nullptr;

		// Adding 
		old->next = next_free;
		next_free = old;
	}

	template<typename ElementType, uint16 kAlignment>
	inline bool FreelistPage<ElementType, kAlignment>::is_in_range(const ElementType * ptr) const
	{
		_StoredType* cur_ptr = (_StoredType*)ptr;
		_StoredType* end = ((_StoredType*)buffer) + element_count;
		if (cur_ptr >= (_StoredType*)buffer && cur_ptr < end)
			return true;
		return false;
	}

	template<typename ElementType, uint16 kAlignment>
	inline void FreelistPage<ElementType, kAlignment>::clear()
	{
		// Destructing only allocated elements
		for (rsize i = 0; i < element_count; ++i)
		{
			_StoredType* cur = ((_StoredType*)buffer) + i;
			if (cur->flags == 1)
				cur->data.~ElementType();
		}

		_reset();
	}

	template<typename ElementType, uint16 kAlignment>
	inline void FreelistPage<ElementType, kAlignment>::_reset()
	{
		_StoredType* cur = (_StoredType*)buffer;
		for (rsize i = 0; i < element_count - 1; ++i)
		{
			cur->next = cur + 1;
			cur->flags = 0;
			++cur;
		}
		cur->next = nullptr;
		cur->flags = 0;
	}

	/*
		Struct: LinearPage
			Typed paged that simply bumps the pointer until full, then can 
			be reset. Used as base for arena containers. Does not call
			constructors / destructors!!
	*/
	template <typename ElementType, uint16 kAlignment = kDefaultAlignment>
	struct LinearPage final : public Page<kAlignment>
	{
	public:
		static const rsize kElementSize = sizeof(ElementType);

	public:
		LinearPage(rsize element_count);
		~LinearPage();

		LinearPage(LinearPage&& other);
		LinearPage(LinearPage& other) = delete;

		LinearPage& operator=(LinearPage&& other);
		LinearPage& operator=(LinearPage& other) = delete;

		ElementType* _next(); // TODO: Fix this name conflict
		ElementType* next_array(rsize count);
		void         reset();

	private:
		const rsize element_count;
		rsize counter;
	};

	template<typename ElementType, uint16 kAlignment>
	inline LinearPage<ElementType, kAlignment>::LinearPage(rsize element_count) :
		Page(element_count * sizeof(ElementType)),
		element_count(element_count),
		counter(0)
	{

	}

	template<typename ElementType, uint16 kAlignment>
	inline LinearPage<ElementType, kAlignment>::~LinearPage()
	{
		counter = 0;
	}

	template<typename ElementType, uint16 kAlignment>
	inline LinearPage<ElementType, kAlignment>::LinearPage(LinearPage && other) :
		Page(other)
	{
		counter = other.counter;
		other.counter = 0;
	}

	template<typename ElementType, uint16 kAlignment>
	inline LinearPage<ElementType, kAlignment>& LinearPage<ElementType, kAlignment>::operator=(LinearPage && other)
	{
		Page::operator=(other);
		count = other.counter;
		other.counter = 0;
		return *this;
	}

	template<typename ElementType, uint16 kAlignment>
	inline ElementType * LinearPage<ElementType, kAlignment>::_next()
	{
		if (counter == element_count)
			return nullptr;
		
		ElementType* ret = &((ElementType*)buffer)[counter++];
		new (ret) ElementType();
		return ret;
	}

	template<typename ElementType, uint16 kAlignment>
	inline ElementType * LinearPage<ElementType, kAlignment>::next_array(rsize count)
	{
		camy_assert(count * kElementSize <= byte_size);
		
		if (counter + count >= element_count)
			return nullptr;

		ElementType* ret = &((ElementType*)buffer)[counter];
		rsize i = 0;
		while (i < count)
			new (&ret[i++]) ElementType();
		counter += count;
		return ret;
	}

	template<typename ElementType, uint16 kAlignment>
	inline void LinearPage<ElementType, kAlignment>::reset()
	{
		counter = 0;
	}
}