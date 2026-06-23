// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include <QtCore/QString>

namespace AyuUiSettings {

inline constexpr auto kMaxAvatarCorners = 23;

inline QString &MonoFontStorage() {
	static auto value = QString();
	return value;
}

inline double &WideMultiplierStorage() {
	static auto value = 1.;
	return value;
}

inline bool &MaterialSwitchesStorage() {
	static auto value = false;
	return value;
}

inline int &AvatarCornersStorage() {
	static auto value = kMaxAvatarCorners;
	return value;
}

inline void setMonoFont(const QString &value) {
	MonoFontStorage() = value;
}

[[nodiscard]] inline const QString &getMonoFont() {
	return MonoFontStorage();
}

inline void setWideMultiplier(double value) {
	WideMultiplierStorage() = value;
}

[[nodiscard]] inline double getWideMultiplier() {
	return WideMultiplierStorage();
}

inline void setMaterialSwitches(bool value) {
	MaterialSwitchesStorage() = value;
}

[[nodiscard]] inline bool getMaterialSwitches() {
	return MaterialSwitchesStorage();
}

inline void setAvatarCorners(int value) {
	AvatarCornersStorage() = (value < 0)
		? 0
		: (value > kMaxAvatarCorners)
		? kMaxAvatarCorners
		: value;
}

[[nodiscard]] inline int getAvatarCorners() {
	return AvatarCornersStorage();
}

} // namespace AyuUiSettings
