// This is the source code of PurrGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "purr/ui/settings/settings_appearance.h"

#include "lang_auto.h"
#include "purr/purr_settings.h"
#include "purr/purr_ui_settings.h"
#include "purr/ui/boxes/font_selector.h"
#include "purr/ui/components/avatar_corners_preview.h"
#include "purr/ui/components/icon_picker.h"
#include "purr/ui/settings/purr_builder.h"
#include "purr/ui/settings/settings_purr_utils.h"
#include "purr/ui/settings/settings_main.h"
#include "inline_bots/bot_attach_web_view.h"
#include "main/main_session.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_purr_icons.h"
#include "styles/style_purr_styles.h"
#include "styles/style_dialogs.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/painter.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/padding_wrap.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

namespace Settings {

using namespace Builder;
using namespace PurrBuilder;

namespace {

bool HasDrawerBots(not_null<Window::SessionController*> controller) {
	// todo: maybe iterate through all accounts
	const auto bots = &controller->session().attachWebView();
	for (const auto &bot : bots->attachBots()) {
		if (!bot.inMainMenu || !bot.media) {
			continue;
		}
		return true;
	}
	return false;
}

void BuildAppIcon(SectionBuilder &builder, PurrSectionBuilder &purr) {
	builder.addSubsectionTitle({
		.id = u"purr/appIcon"_q,
		.title = tr::purr_AppIconHeader(),
	});

	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		return {
			.widget = object_ptr<IconPicker>(ctx.container),
			.margin = st::settingsButtonNoIcon.padding,
		};
	});

#if defined Q_OS_WIN || defined Q_OS_MAC
	builder.addDivider();
	builder.addSkip();
	purr.addSettingToggle({
		.id = u"purr/hideNotificationBadge"_q,
		.title = tr::purr_HideNotificationBadge(),
		.getter = &PurrSettings::hideNotificationBadge,
		.setter = &PurrSettings::setHideNotificationBadge,
	});
	builder.addSkip();
	builder.addDividerText(tr::purr_HideNotificationBadgeDescription());
	builder.addSkip();
#else
    builder.addDivider();
    builder.addSkip();
#endif
}

void BuildAvatarCorners(SectionBuilder &builder, PurrSectionBuilder &purr) {
	auto *settings = &PurrSettings::getInstance();
	const auto controller = builder.controller();

	const auto mapRadius = [](int val)
	{
		if (val == 0) {
			return tr::purr_AvatarCornersSquare(tr::now).toUpper();
		} else if (val == PurrUiSettings::kMaxAvatarCorners) {
			return tr::purr_AvatarCornersCircle(tr::now).toUpper();
		}
		return QString::number(val);
	};

	builder.add([=](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		const auto container = ctx.container;
		auto title = object_ptr<Ui::FlatLabel>(
			container,
			tr::purr_AvatarCorners(),
			st::defaultSubsectionTitle);
		const auto titleRaw = title.data();

		const auto badge = Ui::CreateChild<Ui::PaddingWrap<Ui::FlatLabel>>(
			container,
			object_ptr<Ui::FlatLabel>(
				container,
				settings->avatarCornersValue() | rpl::map(mapRadius),
				st::settingsPremiumNewBadge),
			st::purrBetaBadgePadding);
		badge->show();
		badge->setAttribute(Qt::WA_TransparentForMouseEvents);
		badge->paintRequest() | rpl::on_next([=] {
			auto p = QPainter(badge);
			auto hq = PainterHighQualityEnabler(p);
			p.setPen(Qt::NoPen);
			p.setBrush(st::windowBgActive);
			const auto r = st::purrBetaBadgePadding.left();
			p.drawRoundedRect(badge->rect(), r, r);
		}, badge->lifetime());

		titleRaw->geometryValue() | rpl::on_next([=](QRect geometry) {
			badge->moveToLeft(
				geometry.x()
					+ titleRaw->textMaxWidth()
					+ st::settingsPremiumNewBadgePosition.x(),
				geometry.y()
					+ (geometry.height() - badge->height()) / 2);
		}, badge->lifetime());

		return {
			.widget = std::move(title),
			.margin = st::defaultSubsectionTitlePadding,
		};
	}, [] {
		return SearchEntry{
			.id = u"purr/avatarCorners"_q,
			.title = tr::purr_AvatarCorners(tr::now),
		};
	});

	auto *previewRaw = static_cast<AvatarCornersPreview*>(nullptr);
	builder.add([&](const Builder::WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		auto preview = object_ptr<AvatarCornersPreview>(
			ctx.container,
			controller);
		previewRaw = preview.data();
		const auto vMargin = st::settingsButtonNoIcon.padding
			- st::defaultDialogRow.padding;
		return {
			.widget = std::move(preview),
			.margin = QMargins(0, vMargin.top(), 0, vMargin.bottom()),
		};
	});

	purr.addSlider({
		.id = u"purr/avatarCornersSlider"_q,
		.title = rpl::single(QString()),
		.showTitle = false,
		.steps = PurrUiSettings::kMaxAvatarCorners + 1,
		.current = settings->avatarCorners(),
		.onChanged = [=](int val) {
			PurrSettings::getInstance().setAvatarCorners(val);
			if (previewRaw) {
				previewRaw->update();
			}
		},
		.onFinalChanged = [=](int val) {
			PurrSettings::getInstance().setAvatarCorners(val);
			ShowRestartPrompt(controller);
		},
	});

	purr.addSettingToggle({
		.id = u"purr/singleCornerRadius"_q,
		.title = tr::purr_SingleCornerRadius(),
		.getter = &PurrSettings::singleCornerRadius,
		.setter = &PurrSettings::setSingleCornerRadius,
	});

	builder.addSkip();
	builder.addDividerText(tr::purr_SingleCornerRadiusDescription());
	builder.addSkip();
}

void BuildAppearance(SectionBuilder &builder, PurrSectionBuilder &purr) {
	auto *settings = &PurrSettings::getInstance();

	builder.addSubsectionTitle(tr::purr_CategoryAppearance());

	purr.addSettingToggle({
		.id = u"purr/materialSwitches"_q,
		.altIds = { u"purr/newSwitchStyle"_q },
		.title = tr::purr_MaterialSwitches(),
		.getter = &PurrSettings::materialSwitches,
		.setter = &PurrSettings::setMaterialSwitches,
	});
	purr.addSettingToggle({
		.id = u"purr/disableCustomBackgrounds"_q,
		.altIds = { u"purr/customThemes"_q },
		.title = tr::purr_DisableCustomBackgrounds(),
		.getter = &PurrSettings::disableCustomBackgrounds,
		.setter = &PurrSettings::setDisableCustomBackgrounds,
	});

	purr.addSettingToggle({
		.id = u"purr/hidePremiumStatuses"_q,
		.title = tr::purr_HidePremiumStatuses(),
		.getter = &PurrSettings::hidePremiumStatuses,
		.setter = &PurrSettings::setHidePremiumStatuses,
	});

	const auto controller = builder.controller();
	builder.addButton({
		.id = u"purr/monoFont"_q,
		.title = tr::purr_MonospaceFont(),
		.st = &st::settingsButtonNoIcon,
		.label = rpl::single(
			settings->monoFont().isEmpty()
				? tr::purr_FontDefault(tr::now)
				: settings->monoFont()),
		.onClick = [=] {
			PurrUi::FontSelectorBox::Show(
				controller,
				[=](const QString &font) {
					PurrSettings::getInstance().setMonoFont(font);
				});
		},
	});

	purr.addSectionDivider();
}

void BuildChatFolders(SectionBuilder &builder, PurrSectionBuilder &purr) {
	builder.addSubsectionTitle(tr::purr_ChatFoldersHeader());

	purr.addSettingToggle({
		.id = u"purr/hideNotificationCounters"_q,
		.altIds = { u"purr/tabCounter"_q },
		.title = tr::purr_HideNotificationCounters(),
		.getter = &PurrSettings::hideNotificationCounters,
		.setter = &PurrSettings::setHideNotificationCounters,
	});
	purr.addSettingToggle({
		.id = u"purr/hideAllChatsFolder"_q,
		.altIds = { u"purr/hideAllChats"_q },
		.title = tr::purr_HideAllChats(),
		.getter = &PurrSettings::hideAllChatsFolder,
		.setter = &PurrSettings::setHideAllChatsFolder,
	});

	purr.addSectionDivider();
}

void BuildTrayElements(SectionBuilder &builder, PurrSectionBuilder &purr) {
	builder.addSubsectionTitle(tr::purr_TrayElementsHeader());

	purr.addSettingToggle({
		.id = u"purr/showGhostToggleInTray"_q,
		.title = tr::purr_EnableGhostModeTray(),
		.getter = &PurrSettings::showGhostToggleInTray,
		.setter = &PurrSettings::setShowGhostToggleInTray,
	});

#if defined Q_OS_WIN || defined Q_OS_MAC
	purr.addSettingToggle({
		.id = u"purr/showStreamerToggleInTray"_q,
		.title = tr::purr_EnableStreamerModeTray(),
		.getter = &PurrSettings::showStreamerToggleInTray,
		.setter = &PurrSettings::setShowStreamerToggleInTray,
	});
#endif

	purr.addSectionDivider();
}

void BuildDrawerElements(SectionBuilder &builder, PurrSectionBuilder &purr) {
	builder.addSubsectionTitle(tr::purr_DrawerElementsHeader());

	purr.addSettingToggle({
		.id = u"purr/showMyProfileInDrawer"_q,
		.title = tr::lng_menu_my_profile(),
		.getter = &PurrSettings::showMyProfileInDrawer,
		.setter = &PurrSettings::setShowMyProfileInDrawer,
		.icon = { &st::menuIconProfile },
	});

	const auto controller = builder.controller();
	if (controller && HasDrawerBots(controller)) {
		purr.addSettingToggle({
			.id = u"purr/showBotsInDrawer"_q,
			.title = tr::lng_filters_type_bots(),
			.getter = &PurrSettings::showBotsInDrawer,
			.setter = &PurrSettings::setShowBotsInDrawer,
			.icon = { &st::menuIconBot },
		});
	}

	purr.addSettingToggle({
		.id = u"purr/showNewGroupInDrawer"_q,
		.title = tr::lng_create_group_title(),
		.getter = &PurrSettings::showNewGroupInDrawer,
		.setter = &PurrSettings::setShowNewGroupInDrawer,
		.icon = { &st::menuIconGroups },
	});
	purr.addSettingToggle({
		.id = u"purr/showNewChannelInDrawer"_q,
		.title = tr::lng_create_channel_title(),
		.getter = &PurrSettings::showNewChannelInDrawer,
		.setter = &PurrSettings::setShowNewChannelInDrawer,
		.icon = { &st::menuIconChannel },
	});
	purr.addSettingToggle({
		.id = u"purr/showContactsInDrawer"_q,
		.title = tr::lng_menu_contacts(),
		.getter = &PurrSettings::showContactsInDrawer,
		.setter = &PurrSettings::setShowContactsInDrawer,
		.icon = { &st::menuIconUserShow },
	});
	purr.addSettingToggle({
		.id = u"purr/showCallsInDrawer"_q,
		.title = tr::lng_menu_calls(),
		.getter = &PurrSettings::showCallsInDrawer,
		.setter = &PurrSettings::setShowCallsInDrawer,
		.icon = { &st::menuIconPhone },
	});
	purr.addSettingToggle({
		.id = u"purr/showSavedMessagesInDrawer"_q,
		.title = tr::lng_saved_messages(),
		.getter = &PurrSettings::showSavedMessagesInDrawer,
		.setter = &PurrSettings::setShowSavedMessagesInDrawer,
		.icon = { &st::menuIconSavedMessages },
	});
	purr.addSettingToggle({
		.id = u"purr/showLReadToggleInDrawer"_q,
		.title = tr::purr_LReadMessages(),
		.getter = &PurrSettings::showLReadToggleInDrawer,
		.setter = &PurrSettings::setShowLReadToggleInDrawer,
		.icon = { &st::purrLReadMenuIcon },
	});
	purr.addSettingToggle({
		.id = u"purr/showSReadToggleInDrawer"_q,
		.title = tr::purr_SReadMessages(),
		.getter = &PurrSettings::showSReadToggleInDrawer,
		.setter = &PurrSettings::setShowSReadToggleInDrawer,
		.icon = { &st::purrSReadMenuIcon },
	});
	purr.addSettingToggle({
		.id = u"purr/showNightModeToggleInDrawer"_q,
		.title = tr::lng_menu_night_mode(),
		.getter = &PurrSettings::showNightModeToggleInDrawer,
		.setter = &PurrSettings::setShowNightModeToggleInDrawer,
		.icon = { &st::menuIconNightMode },
	});
	purr.addSettingToggle({
		.id = u"purr/showGhostToggleInDrawer"_q,
		.title = tr::purr_GhostModeToggle(),
		.getter = &PurrSettings::showGhostToggleInDrawer,
		.setter = &PurrSettings::setShowGhostToggleInDrawer,
		.icon = { &st::purrGhostIcon },
	});

#if defined Q_OS_WIN || defined Q_OS_MAC
	purr.addSettingToggle({
		.id = u"purr/showStreamerToggleInDrawer"_q,
		.title = tr::purr_StreamerModeToggle(),
		.getter = &PurrSettings::showStreamerToggleInDrawer,
		.setter = &PurrSettings::setShowStreamerToggleInDrawer,
		.icon = { &st::purrStreamerModeMenuIcon },
	});
#endif

	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = PurrAppearance::Id(),
	.parentId = PurrMain::Id(),
	.title = &tr::purr_CategoryAppearance,
	.icon = &st::menuIconPalette,
}, [](SectionBuilder &builder) {
	auto purr = PurrSectionBuilder(builder);

	builder.addSkip();
	BuildAppIcon(builder, purr);
	BuildAvatarCorners(builder, purr);
	BuildAppearance(builder, purr);
	BuildChatFolders(builder, purr);
	BuildTrayElements(builder, purr);
	BuildDrawerElements(builder, purr);
	builder.addSkip();
});

} // namespace

rpl::producer<QString> PurrAppearance::title() {
	return tr::purr_CategoryAppearance();
}

PurrAppearance::PurrAppearance(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void PurrAppearance::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type PurrAppearanceId() {
	return PurrAppearance::Id();
}

} // namespace Settings
