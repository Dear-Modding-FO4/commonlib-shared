#pragma once

#ifdef COMMONLIB_OPTION_RANDOM

#	include "REX/BASE.h"

namespace REX
{
	// Non-owning standard engine view; the generator must outlive the view and its copies.
	template <typename T>
	class TRandomEngine
	{
	public:
		using result_type = std::uint64_t;

		explicit constexpr TRandomEngine(T& a_owner) noexcept :
			m_owner(std::addressof(a_owner))
		{}

		static constexpr result_type min() noexcept { return std::numeric_limits<result_type>::min(); }
		static constexpr result_type max() noexcept { return std::numeric_limits<result_type>::max(); }
		result_type                  operator()() noexcept { return m_owner->GenerateEngineValue(); }

	private:
		T* m_owner;
	};

	template <typename T>
		requires std::is_arithmetic_v<T>
	class TRandom
	{
	public:
		using engine_type = TRandomEngine<TRandom>;

		TRandom();
		TRandom(std::uint32_t a_seed);
		TRandom(std::uint64_t a_seed);

		static constexpr T min() { return std::numeric_limits<T>::min(); }
		static constexpr T max() { return std::numeric_limits<T>::max(); }

		T Generate(T a_min = min(), T a_max = max());

		// Generate uses typed bounds; this lvalue-only view supports standard random algorithms.
		[[nodiscard]] engine_type GetEngine() & noexcept { return engine_type(*this); }
		engine_type               GetEngine() && = delete;

	protected:
		std::byte m_rng[32];

	private:
		friend engine_type;

		std::uint64_t GenerateEngineValue() noexcept;
	};

	template <typename T>
		requires std::is_integral_v<T>
	class TRandomDistribution
	{
	public:
		using engine_type = TRandomEngine<TRandomDistribution>;

		TRandomDistribution() = delete;
		TRandomDistribution(std::vector<std::uint32_t>& a_weights);
		TRandomDistribution(std::uint32_t a_seed, std::vector<std::uint32_t>& a_weights);
		TRandomDistribution(std::uint64_t a_seed, std::vector<std::uint32_t>& a_weights);

		static constexpr T min() { return std::numeric_limits<T>::min(); }
		static constexpr T max() { return std::numeric_limits<T>::max(); }

		T Generate();

		// Generate uses the configured weights; this lvalue-only view exposes the same engine state.
		[[nodiscard]] engine_type GetEngine() & noexcept { return engine_type(*this); }
		engine_type               GetEngine() && = delete;

	private:
		friend engine_type;

		std::uint64_t GenerateEngineValue() noexcept;

		std::byte                     m_rng[32];
		std::discrete_distribution<T> m_dist;
	};
}

namespace REX::RNG
{
	using F32 = TRandom<float>;
	using F64 = TRandom<double>;
	using I32 = TRandom<std::int32_t>;
	using I64 = TRandom<std::int64_t>;
	using U32 = TRandom<std::uint32_t>;
	using U64 = TRandom<std::uint64_t>;
	using I32D = TRandomDistribution<std::int32_t>;
	using I64D = TRandomDistribution<std::int64_t>;
	using U32D = TRandomDistribution<std::uint32_t>;
	using U64D = TRandomDistribution<std::uint64_t>;
}

#endif
