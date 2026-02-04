#pragma once

#include "REX/BASE.h"

#include "REL/IDDB.h"
#include "REL/Module.h"

namespace REL
{
	class ID
	{
	public:
		static_assert(COMMONLIB_RUNTIMECOUNT > 0, "COMMONLIB_RUNTIMECOUNT must be at least 1.");

		constexpr ID() noexcept = default;

		explicit constexpr ID(std::uint64_t a_id) noexcept
		{
			for (auto& id : m_ids)
			{
				id = a_id;
			}
		}

		explicit constexpr ID(std::initializer_list<std::uint64_t> a_list) noexcept
		{
			if (a_list.size() == 0)
			{
				return;
			}

			std::size_t i = 0;
			std::uint64_t lastValue = 0;

			for (auto val : a_list)
			{
				if (i >= COMMONLIB_RUNTIMECOUNT)
				{
					break;
				}

				m_ids[i++] = val;
				lastValue = val;
			}

			while (i < COMMONLIB_RUNTIMECOUNT)
			{
				m_ids[i++] = lastValue;
			}
		}

		constexpr ID& operator=(std::uint64_t a_id) noexcept
		{
			for (auto& id : m_ids)
			{
				id = a_id;
			}
			return *this;
		}

		[[nodiscard]] std::uintptr_t address() const
		{
			const auto mod = Module::GetSingleton();
			return mod->base() + offset();
		}

		[[nodiscard]] std::uint64_t id() const noexcept
		{
			auto index = static_cast<std::uint8_t>(Module::GetRuntimeIndex());

			if (index >= COMMONLIB_RUNTIMECOUNT)
				index = COMMONLIB_RUNTIMECOUNT - 1;

			return m_ids[index];
		}

		[[nodiscard]] std::size_t offset() const
		{
			const auto iddb = IDDB::GetSingleton();
			return iddb->offset(id());
		}

	private:
		std::uint64_t m_ids[COMMONLIB_RUNTIMECOUNT]{ 0 };
	};
}
