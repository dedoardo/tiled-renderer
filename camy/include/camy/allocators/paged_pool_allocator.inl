namespace camy
{
	namespace allocators
	{
		template <typename Type, u32 count>
		TypedReusablePage<Type, count>::TypedReusablePage() 
		{
			for (u32 i{ 0u }; i < count; ++i)
				camy_to_type_ref(StoredType, &buffer[sizeof(StoredType) * i]).next_free = i + 1;
		}

		template <typename Type, u32 count>
		template <typename ...CtorArgs>
		Type* TypedReusablePage<Type, count>::allocate(CtorArgs&&... ctor_args)
		{
			if (m_free_count == 0)
				return nullptr;

			auto next_free{ m_next_free };
			m_next_free = camy_to_type_ref(StoredType, &buffer[sizeof(StoredType) * m_next_free]).next_free;
			--m_free_count;

			return new (reinterpret_cast<Type*>(&buffer[sizeof(StoredType) * next_free])) Type(std::forward(ctor_args)...);
		}

		template <typename Type, u32 count>
		void TypedReusablePage<Type, count>::deallocate(Type* ptr_t)
		{
			auto ptr{ reinterpret_cast<Byte*>(ptr_t) };
			camy_assert(ptr > &buffer[0], { return; }, "Trying to deallocate invalid pointer");

			// Calculating index from pointer offset
			auto free_index{ static_cast<u32>(reinterpret_cast<std::uintptr_t>(ptr) - reinterpret_cast<std::uintptr_t>(&buffer[0])) };
			camy_assert(free_index % sizeof(Type) == 0, { return; }, "Failed to calcuate index fom pointer");
			free_index /= sizeof(Type);

			camy_to_type_ref(StoredType, &buffer[sizeof(StoredType) * free_index]).next_free = m_next_free;

			m_next_free = free_index;
			++m_free_count;
		}

		template <typename Type, u32 count>
		PagedPoolAllocator<Type, count>::PagedPoolAllocator(u32 alignment) :
			m_current_page{ nullptr },
			m_alignment{ alignment }
		{
			camy_test_if((m_alignment & 0x1),
			{
				camy_warning("Alignment: ", m_alignment, "is not valid, has to be a power of two, defaulting to 2");
				m_alignment = 2;
			});

			m_first = new (_aligned_malloc(sizeof(TypedReusablePage<Type, count>), m_alignment)) TypedReusablePage<Type, count>;
			m_current_page = m_first;

			camy_test_if(((void*)m_current_page != (void*)&(m_current_page->buffer)),
			{
				camy_warning("Page buffer is not at offset 0 this will result in unaligned allocation");
			});
		}

		template <typename Type, u32 count>
		PagedPoolAllocator<Type, count>::~PagedPoolAllocator()
		{
			auto current_page{ m_first };
			do
			{
				auto to_delete{ current_page };

				if (current_page != nullptr)
					current_page = static_cast<TypedReusablePage<Type, count>*>(current_page->next);

				_aligned_free(to_delete);
			} while (current_page != nullptr);
		}

		template <typename Type, u32 count>
		template <typename ...CtorArgs>
		Type* PagedPoolAllocator<Type, count>::allocate(CtorArgs&&... ctor_args)
		{
			Type* result{ nullptr };
			do
			{
				// We try to allocate from the current page ( does not necessarily have to be the last one )
				result = m_current_page->allocate(std::forward(ctor_args)...);
				if (result != nullptr)
					return result;

				// If we are at the last page, let's allocate the next one, allocation won't fail at the next iteration
				if (m_current_page->next == nullptr)
					m_current_page->next = new (_aligned_malloc(sizeof(TypedReusablePage<Type, count>), m_alignment)) TypedReusablePage<Type, count>;

				// Let's go to the next page
				m_current_page = static_cast<TypedReusablePage<Type, count>*>(m_current_page->next);
			} while (m_current_page != nullptr);

			return result;
		}

		template <typename Type, u32 count>
		void PagedPoolAllocator<Type, count>::deallocate(Type* ptr)
		{
			// Calling destructor
			ptr->~Type();

			// We have to find the page the ptr was allocated from
			auto current_page{ m_first };
			do
			{
				// If the ptr is in this page we call deallocate() and then leave
				if (ptr > reinterpret_cast<Type*>(&current_page->buffer[0]) &&
					ptr < reinterpret_cast<Type*>(&current_page->buffer[sizeof(Type) * count]))
				{
					current_page->deallocate(ptr);
					return;
				}

				// Going to the next page
				current_page = static_cast<TypedReusablePage<Type, count>*>(m_current_page->next);
			} while (current_page != nullptr);

			// Nothing happens if the pointer has not been allocated with this page allocator
		}
	}
}