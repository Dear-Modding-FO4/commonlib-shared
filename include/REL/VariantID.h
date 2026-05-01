#pragma once

#include "REL/ID.h"
#include "REL/Offset.h"

namespace REL
{
	class VariantID
	{
		static_assert(COMMONLIB_RUNTIMECOUNT > 0, "COMMONLIB_RUNTIMECOUNT must be at least 1.");

	public:
		constexpr VariantID() noexcept = default;

		explicit constexpr VariantID(std::uint64_t a_id) noexcept
		{
			for (auto& id : m_offs)
				id = a_id;
		}

		template <typename... Args>
		explicit constexpr VariantID(Args&&... args) noexcept
		{
			auto size = sizeof...(args);
			if (!size)
				return;

			std::size_t   i = 0;
			std::uint64_t lastValue = 0;

			inserter(i, lastValue, std::forward<Args>(args)...);

			while (i < COMMONLIB_RUNTIMECOUNT)
				m_offs[i++] = lastValue;
		}

		constexpr VariantID& operator=(std::uint64_t a_id) noexcept
		{
			for (auto& id : m_offs)
				id = a_id;
			return *this;
		}

		[[nodiscard]] std::uintptr_t address() const
		{
			const auto mod = REX::FModule::GetExecutingModule();
			return mod.GetBaseAddress() + offset();
		}

		[[nodiscard]] std::size_t offset() const
		{
			auto index = static_cast<std::uint8_t>(REX::FModule::GetRuntimeIndex());

			if (index >= COMMONLIB_RUNTIMECOUNT)
				index = COMMONLIB_RUNTIMECOUNT - 1;

			return m_offs[index];
		}

	private:
		template <typename T>
		void inserter(std::size_t& i, std::uint64_t& last, T item) noexcept
		{
			if constexpr (std::is_same_v<T, REL::ID>) {
				m_offs[i] = item.offset();
				last = m_offs[i++];
			} else if constexpr (std::is_same_v<T, REL::Offset>) {
				m_offs[i] = item.offset();
				last = m_offs[i++];
			} else if (std::is_integral_v<T>) {
				m_offs[i] = REL::ID{ *(uint64_t*)&item }.offset();
				last = m_offs[i++];
			}
		}

		template <typename T, typename... Args>
		void inserter(std::size_t& i, std::uint64_t& last, T item, Args... args) noexcept
		{
			inserter(i, last, item);
			inserter(i, last, args...);
		}

		std::uint64_t m_offs[COMMONLIB_RUNTIMECOUNT]{ 0 };
	};
}
