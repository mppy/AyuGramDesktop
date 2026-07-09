/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "tray.h"
#include "tray_accounts_menu.h"

#include "core/application.h"
#include "core/core_settings.h"
#include "platform/platform_notifications_manager.h"
#include "platform/platform_specific.h"
#include "lang/lang_keys.h"

#include "purr/purr_settings.h"
#include "purr/features/streamer_mode/streamer_mode.h"
#include "lang_auto.h"

#include <QtWidgets/QApplication>

namespace Core {

Tray::Tray() {
}

void Tray::create() {
	rebuildMenu();
	using WorkMode = Settings::WorkMode;
	if (Core::App().settings().workMode() != WorkMode::WindowOnly) {
		_tray.createIcon();
	}

	Core::App().settings().workModeValue(
	) | rpl::combine_previous(
	) | rpl::on_next([=](WorkMode previous, WorkMode state) {
		const auto wasHasIcon = (previous != WorkMode::WindowOnly);
		const auto nowHasIcon = (state != WorkMode::WindowOnly);
		if (wasHasIcon != nowHasIcon) {
			if (nowHasIcon) {
				_tray.createIcon();
			} else {
				_tray.destroyIcon();
			}
		}
	}, _tray.lifetime());

	Core::App().settings().trayIconMonochromeChanges(
	) | rpl::on_next([=] {
		updateIconCounters();
	}, _tray.lifetime());

	Core::App().passcodeLockChanges(
	) | rpl::on_next([=] {
		rebuildMenu();
	}, _tray.lifetime());

	TrayAccountsMenu::SetupChangesSubscription(
		[=] { rebuildMenu(); },
		_tray.lifetime());

	_tray.iconClicks(
	) | rpl::on_next([=] {
		const auto skipTrayClick = (_lastTrayClickTime > 0)
			&& (crl::now() - _lastTrayClickTime
				< QApplication::doubleClickInterval());
		if (!skipTrayClick) {
			_activeForTrayIconAction = Core::App().isActiveForTrayMenu();
			_minimizeMenuItemClicks.fire({});
			_lastTrayClickTime = crl::now();
		}
	}, _tray.lifetime());
}

void Tray::rebuildMenu() {
	_tray.destroyMenu();
	_tray.createMenu();

	{
		auto minimizeText = _textUpdates.events(
		) | rpl::map([=] {
			_activeForTrayIconAction = Core::App().isActiveForTrayMenu();
			return _activeForTrayIconAction
				? tr::lng_minimize_to_tray(tr::now)
				: tr::lng_open_from_tray(tr::now);
		});

		_tray.addAction(
			std::move(minimizeText),
			[=] { _minimizeMenuItemClicks.fire({}); });
	}

	if (!Core::App().passcodeLocked()) {
		auto notificationsText = _textUpdates.events(
		) | rpl::map([=] {
			return Core::App().settings().desktopNotify()
				? tr::lng_disable_notifications_from_tray(tr::now)
				: tr::lng_enable_notifications_from_tray(tr::now);
		});

		_tray.addAction(
			std::move(notificationsText),
			[=] { toggleSoundNotifications(); });
	}

	const auto &settings = PurrSettings::getInstance();
	if (settings.showGhostToggleInTray()) {
		auto ghostActiveChanges = PurrSettings::getInstance().useGlobalGhostModeValue(
		) | rpl::map([](bool) {
			return PurrSettings::ghost().ghostModeActiveValue();
		}) | rpl::flatten_latest();

		auto text = rpl::combine(
			_textUpdates.events_starting_with({}),
			std::move(ghostActiveChanges)
		) | rpl::map([](auto, bool active) {
			return active
				? tr::purr_DisableGhostModeTray(tr::now)
				: tr::purr_EnableGhostModeTray(tr::now);
		});
		_tray.addAction(std::move(text), [=] {
			auto &ghost = PurrSettings::ghost();
			ghost.setGhostModeEnabled(!ghost.isGhostModeActive());
			updateMenuText();
		});
	}

	if (settings.showStreamerToggleInTray()) {
		auto text = _textUpdates.events_starting_with({}
		) | rpl::map([] {
			return PurrFeatures::StreamerMode::isEnabled()
				? tr::purr_DisableStreamerModeTray(tr::now)
				: tr::purr_EnableStreamerModeTray(tr::now);
		});
		_tray.addAction(std::move(text), [=] {
			if (PurrFeatures::StreamerMode::isEnabled()) {
				PurrFeatures::StreamerMode::disable();
			} else {
				PurrFeatures::StreamerMode::enable();
			}
			updateMenuText();
		});
	}

	_tray.addAction(tr::lng_quit_from_tray(), [] { Core::Quit(); });

	TrayAccountsMenu::Fill(_tray);

	updateMenuText();
}

void Tray::updateMenuText() {
	_textUpdates.fire({});
}

void Tray::updateIconCounters() {
	_tray.updateIcon();
}

rpl::producer<> Tray::aboutToShowRequests() const {
	return _tray.aboutToShowRequests();
}

rpl::producer<> Tray::showFromTrayRequests() const {
	return rpl::merge(
		_tray.showFromTrayRequests(),
		_minimizeMenuItemClicks.events() | rpl::filter([=] {
			return !_activeForTrayIconAction;
		})
	);
}

rpl::producer<> Tray::hideToTrayRequests() const {
	auto triggers = rpl::merge(
		_tray.hideToTrayRequests(),
		_minimizeMenuItemClicks.events() | rpl::filter([=] {
			return _activeForTrayIconAction;
		})
	);
	if (_tray.hasTrayMessageSupport()) {
		return std::move(triggers) | rpl::map([=]() -> rpl::empty_value {
			_tray.showTrayMessage();
			return {};
		});
	} else {
		return triggers;
	}
}

void Tray::toggleSoundNotifications() {
	auto soundNotifyChanged = false;
	auto flashBounceNotifyChanged = false;
	auto &settings = Core::App().settings();
	settings.setDesktopNotify(!settings.desktopNotify());
	if (settings.desktopNotify()) {
		if (settings.rememberedSoundNotifyFromTray()
			&& !settings.soundNotify()) {
			settings.setSoundNotify(true);
			settings.setRememberedSoundNotifyFromTray(false);
			soundNotifyChanged = true;
		}
		if (settings.rememberedFlashBounceNotifyFromTray()
			&& !settings.flashBounceNotify()) {
			settings.setFlashBounceNotify(true);
			settings.setRememberedFlashBounceNotifyFromTray(false);
			flashBounceNotifyChanged = true;
		}
	} else {
		if (settings.soundNotify()) {
			settings.setSoundNotify(false);
			settings.setRememberedSoundNotifyFromTray(true);
			soundNotifyChanged = true;
		} else {
			settings.setRememberedSoundNotifyFromTray(false);
		}
		if (settings.flashBounceNotify()) {
			settings.setFlashBounceNotify(false);
			settings.setRememberedFlashBounceNotifyFromTray(true);
			flashBounceNotifyChanged = true;
		} else {
			settings.setRememberedFlashBounceNotifyFromTray(false);
		}
	}
	Core::App().saveSettingsDelayed();
	using Change = Window::Notifications::ChangeType;
	auto &notifications = Core::App().notifications();
	notifications.notifySettingsChanged(Change::DesktopEnabled);
	if (soundNotifyChanged) {
		notifications.notifySettingsChanged(Change::SoundEnabled);
	}
	if (flashBounceNotifyChanged) {
		notifications.notifySettingsChanged(Change::FlashBounceEnabled);
	}
}

bool Tray::has() const {
	return _tray.hasIcon() && Platform::TrayIconSupported();
}

} // namespace Core
