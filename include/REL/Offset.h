#pragma once

#include "REX/BASE.h"

#include "REL/Module.h"

namespace REL
{
	class Offset
	{
	public:
		static_assert(COMMONLIB_RUNTIMECOUNT > 0, "COMMONLIB_RUNTIMECOUNT must be at least 1.");

		constexpr Offset() noexcept = default;

		explicit constexpr Offset(std::size_t a_offset) noexcept
		{
			for (auto& offset : _offsets)
			{
				offset = a_offset;
			}
		}

		explicit constexpr Offset(std::initializer_list<std::size_t> a_list) noexcept
		{
			if (a_list.size() == 0)
			{
				return;
			}

			std::size_t i = 0;
			std::size_t lastValue = 0;

			for (auto val : a_list)
			{
				if (i >= COMMONLIB_RUNTIMECOUNT)
				{
					break;
				}

				_offsets[i++] = val;
				lastValue = val;
			}

			while (i < COMMONLIB_RUNTIMECOUNT)
			{
				_offsets[i++] = lastValue;
			}
		}

		constexpr Offset& operator=(std::size_t a_offset) noexcept
		{
			for (auto& offset : _offsets)
			{
				offset = a_offset;
			}
			return *this;
		}

		[[nodiscard]] std::uintptr_t address() const
		{
			const auto mod = Module::GetSingleton();
			return mod->base() + offset();
		}

		[[nodiscard]] std::size_t offset() const noexcept
		{
			auto index = static_cast<std::uint8_t>(Module::GetRuntimeIndex());

			if (index >= COMMONLIB_RUNTIMECOUNT)
				index = COMMONLIB_RUNTIMECOUNT - 1;

			return _offsets[index];
		}

	private:
		std::size_t _offsets[COMMONLIB_RUNTIMECOUNT]{ 0 };
	};
}
