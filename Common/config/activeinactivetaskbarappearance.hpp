#pragma once
#include <string_view>
#include <optional>


#include "rapidjsonhelper.hpp"
#include "taskbarappearance.hpp"

struct ActiveInactiveTaskbarAppearance : TaskbarAppearance {
	std::optional<TaskbarAppearance> Inactive;

	constexpr ActiveInactiveTaskbarAppearance() noexcept = default;
	constexpr ActiveInactiveTaskbarAppearance(std::optional<TaskbarAppearance> inactive, ACCENT_STATE accent, Util::Color color, bool showPeek, bool showLine, float blurRadius) noexcept :
		TaskbarAppearance(accent, color, showPeek, showLine, blurRadius),
		Inactive(std::move(inactive))
	{ }

	template<typename Writer>
	inline void Serialize(Writer &writer) const
	{
		TaskbarAppearance::Serialize(writer);
		rjh::Serialize(writer, Inactive, INACTIVE_KEY);
	}

	inline void Deserialize(const rjh::value_t &obj, void (*unknownKeyCallback)(std::wstring_view))
	{
		rjh::EnsureType(rj::Type::kObjectType, obj.GetType(), L"root node");

		for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
		{
			rjh::EnsureType(rj::Type::kStringType, it->name.GetType(), L"member name");

			const auto key = rjh::ValueToStringView(it->name);
			if (key == INACTIVE_KEY)
			{
				rjh::Deserialize(it->value, Inactive, key, unknownKeyCallback);
			}
			else
			{
				InnerDeserialize(key, it->value, unknownKeyCallback);
			}
		}
	}

private:
	static constexpr std::wstring_view INACTIVE_KEY = L"inactive";
};
