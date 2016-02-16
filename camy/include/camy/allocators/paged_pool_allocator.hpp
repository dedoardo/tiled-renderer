#pragma once

#include "page.hpp"

namespace camy
{
	namespace allocators
	{
		template <typename Type, u32 count>
		struct TypedReusablePage : public Page<sizeof(Type) * count>
		{
			static_assert(count > 0, "Can't created a page with size 0");
			static_assert(sizeof(Type) >= sizeof(u32), "Typed pages element size has to be > 32 bit");

			union StoredType
			{
				~StoredType() { data.~Type() }
				Type data;
				u32  next_free;
			};

			static_assert(sizeof(StoredType) == sizeof(Type), "Segmentation faults will occur if this condition is not verified");

			TypedReusablePage();

			template <typename ...CtorArgs>
			Type* allocate(CtorArgs&&... ctor_args);

			void deallocate(Type* ptr);

			u32 m_next_free{ 0 };
			u32 m_free_count{ count };
		};

		/*
			Class: PagedPoolAllocator
				Typed pool allocator that allows to reuse pointers ( it is not index-based ), it uses
				reusable pages. 
				
				Type is the type of the pool ( since it's tpe
		*/
		template <typename Type, u32 count = 10>
		class PagedPoolAllocator final
		{
			static_assert(count > 0, "Can't created a pool allocator with page size 0");
		public:
			/*
				Constructor: PagedPoolAllocator
					Constructs a new instance preallocating one page
			*/
			PagedPoolAllocator(u32 alignment = 16);

			/*
				Destuctor: ~PagedPoolAllocator
					Destructs the current instance releasing all the associated memory
			*/
			~PagedPoolAllocator();

			/*
				Function: allocate
					Allocates a new instance  *without* calling the constructor, it has to be manually
					placed new
			*/
			template <typename ...CtorArgs>
			Type* allocate(CtorArgs&&... ctor_args);

			/*
				Function: deallocate
					Deallocates a previously allocated pointer. Memory is not freed, but will be reused
			*/
			void deallocate(Type* ptr);

		private:
			u32 m_alignment;

			TypedReusablePage<Type, count>* m_current_page;
			TypedReusablePage<Type, count>* m_first;
		};
	}
}

#include "paged_pool_allocator.inl"