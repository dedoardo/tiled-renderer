namespace camy
{
	namespace allocators
	{
		template <Size page_size>
		PagedLinearAllocator<page_size>::PagedLinearAllocator(u32 alignment) : 
			m_alignment{ alignment }
		{
			camy_test_if((m_alignment & 0x1), 
			{
				camy_warning("Alignment: ", m_alignment, "is not valid, has to be a power of two, defaulting to 2");
				m_alignment = 2;
			});

			m_current_page = new (_aligned_malloc(sizeof(Page<page_size>), m_alignment)) Page<page_size>;

			// Offset at compile time can indeed be done, but requires some little yet ugly hacks, if aligned is not 
			// respected it currently isnt a problem, just warning the use
			camy_test_if(((void*)m_current_page != (void*)&(m_current_page->buffer)),
			{
				camy_warning("Page buffer is not at offset 0 this will result in unaligned allocation");
			});
		}

		template <Size page_size>
		PagedLinearAllocator<page_size>::~PagedLinearAllocator()
		{
			// When freeing memory we have to be sure m_current_page points to the first page
			reset();

			auto current_page{ m_current_page };
			do
			{
				auto to_delete{ current_page };
		 
				if (current_page != nullptr)
					current_page = m_current_page->next;
				
				// Deleting current page
				_aligned_free(to_delete);
			} while (current_page != nullptr);
		}

		template <Size page_size>
		void* PagedLinearAllocator<page_size>::allocate(Size size)
		{
			camy_assert(size <= page_size, { return nullptr; }, "Can't allocate an item bigger than a page");
			camy_assert(m_current_page != nullptr, { return nullptr; }, "Invalid current page");

			// Do we still have space in the current page
			auto current_index{ m_current_page->next_free };
			auto next_index{ m_current_page->next_free + size };
			if (next_index < page_size)
			{
				m_current_page->next_free = next_index;
				return reinterpret_cast<void*>(&m_current_page->buffer[current_index]);
			}

			// We don't, now can we reuse a previously allocated page ? 
			if (m_current_page->next != nullptr)
			{
				m_current_page = m_current_page->next;
				m_current_page->next_free += size;
				return reinterpret_cast<void*>(&m_current_page->buffer[0]);
			}

			// Unfortunately we have to allocate a new page
			m_current_page->next = new (_aligned_malloc(sizeof(Page<page_size>), m_alignment)) Page<page_size>;
			m_current_page->next->previous = m_current_page; // back link
			m_current_page = m_current_page->next;
			m_current_page->next_free += size;
			return reinterpret_cast<void*>(&m_current_page->buffer[0]);
		}

		template <Size page_size>
		template <typename Type, typename ...CtorArgs>
		Type* PagedLinearAllocator<page_size>::allocate(CtorArgs&&... ctor_args)
		{
			auto size{ sizeof(Type) };

			camy_assert(size <= page_size, { return nullptr; }, "Can't allocate an item bigger than a page itself");
			camy_assert(m_current_page != nullptr, { return nullptr; } "Invalid current page");

			// Do we still have space in the current page
			auto current_index{ m_current_page->next_free };
			auto next_index{ m_current_page->next_free + size };
			if (next_index < page_size)
			{
				m_current_page->next_free = next_index;
				return new (reinterpret_cast<Type*>(&m_current_page->buffer[current_index])) Type(std::forward<CtorArgs>(ctor_args)...);
			}

			// We don't, now can we reuse a previously allocated page ? 
			if (m_current_page->next != nullptr)
			{
				m_current_page = m_current_page->next;
				m_current_page->next_free += size;
				return new (reinterpret_cast<Type*>(&m_current_page->buffer[0])) Type(std::forward<CtorArgs>(ctor_args)...);
			}

			// Unfortunately we have to allocate a new page
			m_current_page->next = new Page<page_size>;
			m_current_page->next->previous = m_current_page;
			m_current_page = m_current_page->next;
			m_current_page->next_free += size;
			return new (reinterpret_cast<Type*>(&m_current_page->buffer[0])) Type(std::forward<CtorArgs>(ctor_args)...);
		}

		template <Size page_size>
		void PagedLinearAllocator<page_size>::reset()
		{
			camy_assert(m_current_page != nullptr, { return; }, "Invalid current page");

			// Looping backwards from current one and setting all to 0 
			m_current_page->next_free = 0;
			auto current{ m_current_page };
			while (current->previous != nullptr)
			{
				current->previous->next_free = 0;
				current = current->previous;
			}

			m_current_page = current;
		}
	}
}