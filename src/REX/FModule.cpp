#include "REX/FModule.h"

#include "REL/Utility.h"
#include "REX/CAST.h"
#include "REX/LOG.h"
#include "REX/W32/KERNEL32.h"

namespace
{
	constexpr std::array SUPPORTED_OG_VERSIONS{
		REL::Version{ 1, 10, 163, 0 }
	};
	constexpr std::array SUPPORTED_NG_VERSIONS{
		REL::Version{ 1, 10, 980, 0 },
		REL::Version{ 1, 10, 984, 0 }
	};
	constexpr std::array SUPPORTED_AE_VERSIONS{
		REL::Version{ 1, 11, 137, 0 },
		REL::Version{ 1, 11, 159, 0 },
		REL::Version{ 1, 11, 169, 0 },
		REL::Version{ 1, 11, 191, 0 },
		REL::Version{ 1, 11, 221, 0 },
		REL::Version{ 1, 11, 240, 0 }
	};

	template <std::size_t N>
	[[nodiscard]] constexpr bool Contains(const std::array<REL::Version, N>& a_versions, const REL::Version& a_version) noexcept
	{
		return std::ranges::find(a_versions, a_version) != a_versions.end();
	}

	[[nodiscard]] constexpr std::optional<REX::FModule::Runtime> GetRuntimeForVersion(const REL::Version& a_version) noexcept
	{
		if (Contains(SUPPORTED_OG_VERSIONS, a_version)) {
			return REX::FModule::Runtime::kOG;
		}
		if (Contains(SUPPORTED_NG_VERSIONS, a_version)) {
			return REX::FModule::Runtime::kNG;
		}
		if (Contains(SUPPORTED_AE_VERSIONS, a_version)) {
			return REX::FModule::Runtime::kAE;
		}

		return std::nullopt;
	}

	static_assert(GetRuntimeForVersion(REL::Version{ 1, 10, 163 }) == REX::FModule::Runtime::kOG);
	static_assert(GetRuntimeForVersion(REL::Version{ 1, 10, 980 }) == REX::FModule::Runtime::kNG);
	static_assert(GetRuntimeForVersion(REL::Version{ 1, 10, 984 }) == REX::FModule::Runtime::kNG);
	static_assert(GetRuntimeForVersion(REL::Version{ 1, 11, 137 }) == REX::FModule::Runtime::kAE);
	static_assert(GetRuntimeForVersion(REL::Version{ 1, 11, 240 }) == REX::FModule::Runtime::kAE);
	static_assert(!GetRuntimeForVersion(REL::Version{ 1, 10, 162 }));
	static_assert(!GetRuntimeForVersion(REL::Version{ 1, 10, 164 }));
	static_assert(!GetRuntimeForVersion(REL::Version{ 1, 10, 985 }));
	static_assert(!GetRuntimeForVersion(REL::Version{ 1, 11, 241 }));
}

namespace REX
{
	[[nodiscard]] FModule::Runtime FModule::GetRuntimeIndex() noexcept
	{
		static const auto runtime = []() {
			const auto mod = REX::FModule::GetExecutingModule();
			const auto version = mod.GetFileVersion();
			if (const auto result = GetRuntimeForVersion(version)) {
				return *result;
			}

			REX::FAIL(
				"Unsupported Fallout 4 runtime!\n"
				"Game Version: {}",
				version);
			std::terminate();
		}();

		return runtime;
	}

	FModule FModule::GetCurrentModule()
	{
		return FModule{ W32::GetCurrentModule() };
	}

	FModule FModule::GetExecutingModule()
	{
		return FModule{ W32::GetModuleHandleA(nullptr) };
	}

	FModule FModule::GetLoadedModule(std::string_view a_name)
	{
		return FModule{ W32::GetModuleHandleA(a_name.data()) };
	}

	std::string FModule::GetFileName() const noexcept
	{
		char path[W32::MAX_PATH];
		if (!W32::GetModuleFileNameA(reinterpret_cast<W32::HMODULE>(m_base), path, W32::MAX_PATH)) {
			REX::FAIL("Failed to obtain module file name."sv);
		}

		return std::filesystem::path(path).string();
	}

	REL::Version FModule::GetFileVersion() const noexcept
	{
		const auto filename = GetFileName();
		if (const auto version = REL::GetFileVersion(filename)) {
			return *version;
		}

		REX::FAIL(
			"Failed to obtain module file version!\n"
			"Module: {}",
			filename);
		std::terminate();
	}

	void* FModule::GetExportFunctionPointer(std::string_view a_function) const
	{
		if (m_base) {
			return W32::GetProcAddress(reinterpret_cast<W32::HMODULE>(m_base), a_function.data());
		}

		return nullptr;
	}

	void* FModule::GetImportFunctionPointer(std::string_view a_function, std::string_view a_library) const
	{
		const auto dosHeader = reinterpret_cast<W32::IMAGE_DOS_HEADER*>(m_base);
		if (dosHeader->magic != W32::IMAGE_DOS_SIGNATURE) {
			REX::ERROR("Invalid IMAGE_DOS_HEADER"sv);
			return nullptr;
		}

		const auto  ntHeader = ADJUST_POINTER<W32::IMAGE_NT_HEADERS64>(dosHeader, dosHeader->lfanew);
		const auto& dataDir = ntHeader->optionalHeader.dataDirectory[REX::W32::IMAGE_DIRECTORY_ENTRY_IMPORT];
		const auto  importDesc = ADJUST_POINTER<W32::IMAGE_IMPORT_DESCRIPTOR>(dosHeader, dataDir.virtualAddress);

		for (auto import = importDesc; import->characteristics != 0; ++import) {
			auto name = ADJUST_POINTER<const char>(dosHeader, import->name);
			if (a_library.size() == strlen(name) && _strnicmp(a_library.data(), name, a_library.size()) != 0) {
				continue;
			}

			const auto thunk = ADJUST_POINTER<W32::IMAGE_THUNK_DATA64>(dosHeader, import->firstThunkOriginal);
			for (std::size_t i = 0; thunk[i].ordinal; ++i) {
				if (W32::IMAGE_SNAP_BY_ORDINAL64(thunk[i].ordinal)) {
					continue;
				}

				const auto importByName = ADJUST_POINTER<W32::IMAGE_IMPORT_BY_NAME>(dosHeader, thunk[i].address);
				if (a_function.size() == strlen(importByName->name) &&
					_strnicmp(a_function.data(), importByName->name, a_function.size()) == 0) {
					return ADJUST_POINTER<W32::IMAGE_THUNK_DATA64>(dosHeader, import->firstThunk) + i;
				}
			}
		}

		REX::ERROR("Failed to get {} ({})", a_function, a_library);

		return nullptr;
	}

	void* FModule::SetImportFunctionPointer(std::string_view a_function, std::string_view a_library, void* a_pointer) const
	{
		auto original = GetImportFunctionPointer(a_function, a_library);
		if (original) {
			REL::WriteSafeData(original, a_pointer);
		} else {
			REX::ERROR("Failed to set {} ({})", a_function, a_library);
		}

		return original;
	}

	FModuleSection FModule::GetSection(std::string_view a_section) const
	{
		const auto dosHeader = reinterpret_cast<const W32::IMAGE_DOS_HEADER*>(m_base);
		const auto ntHeader = ADJUST_POINTER<W32::IMAGE_NT_HEADERS64>(dosHeader, dosHeader->lfanew);
		const auto sections = W32::IMAGE_FIRST_SECTION(ntHeader);
		for (std::size_t i = 0; i < ntHeader->fileHeader.sectionCount; ++i) {
			const auto&    section = sections[i];
			constexpr auto size = std::extent_v<decltype(section.name)>;
			const auto     len = std::min(a_section.size(), size);

			if (std::memcmp(a_section.data(), section.name, len) == 0) {
				return FModuleSection{ m_base, m_base + section.virtualAddress, section.virtualSize };
			}
		}

		return {};
	}
}
