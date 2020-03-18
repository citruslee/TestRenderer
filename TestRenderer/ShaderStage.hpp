#pragma once

#include <cstdint>
#include <type_traits>

enum class EShaderStage : uint8_t
{
	NONE = 0x00,
	VERTEX = 0x01,
	PIXEL = 0x02,
	COMPUTE = 0x04
};

inline EShaderStage operator | (const EShaderStage Lhs, const EShaderStage Rhs)
{
	using TType = std::underlying_type_t<EShaderStage>;
	return static_cast<EShaderStage>(static_cast<TType>(Lhs) | static_cast<TType>(Rhs));
}

inline EShaderStage& operator |= (EShaderStage& Lhs, const EShaderStage Rhs)
{
	Lhs = Lhs | Rhs;
	return Lhs;
}

inline EShaderStage operator & (const EShaderStage Lhs, const EShaderStage Rhs)
{
	using TType = std::underlying_type_t<EShaderStage>;
	return static_cast<EShaderStage>(static_cast<TType>(Lhs) & static_cast<TType>(Rhs));
}

inline EShaderStage& operator &= (EShaderStage& Lhs, const EShaderStage Rhs)
{
	Lhs = Lhs & Rhs;
	return Lhs;
}
