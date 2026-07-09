// This is the source code of PurrGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "purr/ui/settings/settings_general.h"

#include "lang_auto.h"
#include "purr/purr_settings.h"
#include "purr/ui/settings/purr_builder.h"
#include "purr/ui/settings/settings_purr_utils.h"
#include "purr/ui/settings/settings_main.h"
#include "base/platform/base_platform_info.h"
#include "core/application.h"
#include "lang/lang_text_entity.h"
#include "platform/platform_translate_provider.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/boxes/single_choice_box.h"
#include "ui/toast/toast.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_controller.h"
#include "window/window_session_controller.h"

namespace Settings {

using namespace Builder;
using namespace PurrBuilder;

namespace {

void BuildTranslator(SectionBuilder &builder, PurrSectionBuilder &purr) {
	builder.addSubsectionTitle(tr::lng_translate_settings_subtitle());

	auto *settings = &PurrSettings::getInstance();

	const auto options = std::vector{
		std::pair(TranslationProvider::Telegram, QString("Telegram")),
		std::pair(TranslationProvider::Google, QString("Google")),
		std::pair(TranslationProvider::Yandex, QString("Yandex")),
	};
	const auto nativeAvailable = Platform::IsTranslateProviderAvailable();
	auto availableOptions = options;
	if (nativeAvailable) {
		availableOptions.push_back(std::pair(
			TranslationProvider::Native,
			[] {
				if constexpr (Platform::IsMac()) {
					return QString("macOS");
				} else if constexpr (Platform::IsWindows()) {
					return QString("Windows");
				} else {
					return QString("Linux");
				}
			}()));
	}
	auto optionLabels = std::vector<QString>();
	optionLabels.reserve(availableOptions.size());
	for (const auto &option : availableOptions) {
		optionLabels.push_back(option.second);
	}

	const auto getIndex = [=](TranslationProvider val) {
		const auto i = ranges::find(
			availableOptions,
			val,
			&std::pair<TranslationProvider, QString>::first);
		return (i != end(availableOptions))
			? int(i - begin(availableOptions))
			: 0;
	};

	auto currentVal = PurrSettings::getInstance().translationProviderValue()
		| rpl::map(getIndex)
		| rpl::map([=](int val) { return availableOptions[val].second; });

	const auto button = builder.addButton({
		.id = u"purr/translationProvider"_q,
		.title = tr::purr_TranslationProvider(),
		.st = &st::settingsButtonNoIcon,
		.label = std::move(currentVal),
		.onClick = [=] {
			if (const auto controller = Core::App().activeWindow()->sessionController()) {
				controller->show(Box(
						[=](not_null<Ui::GenericBox*> box) {
							const auto save = [=](int index) {
								const auto option = availableOptions[index].first;
								PurrSettings::getInstance().setTranslationProvider(option);

								if constexpr (Platform::IsMac()) {
									if (option == TranslationProvider::Native) {
										controller->showToast(Ui::Toast::Config{
											.text = tr::lng_translate_settings_use_platform_mac_about(tr::now, tr::rich),
											.duration = 6 * crl::time(1000)
										});
									}
								}
							};
							SingleChoiceBox(box, {
								.title = tr::purr_TranslationProvider(),
								.options = optionLabels,
								.initialSelection = getIndex(settings->translationProvider()),
								.callback = save,
							});
						}));
			}
		},
	});
	if (button) {
		purr.addBetaBadge(button);
	}
}

void BuildShowPeerId(SectionBuilder &builder) {
	auto *settings = &PurrSettings::getInstance();

	const auto options = std::vector{
		QString(tr::purr_SettingsShowID_Hide(tr::now)),
		QString("Telegram API"),
		QString("Bot API")
	};

	auto currentVal = PurrSettings::getInstance().showPeerIdValue()
		| rpl::map([=](PeerIdDisplay val) {
			return options[static_cast<int>(val)];
		});

	const auto controller = builder.controller();
	builder.addButton({
		.id = u"purr/showPeerId"_q,
		.altIds = { u"purr/showIdAndDc"_q },
		.title = tr::purr_SettingsShowID(),
		.st = &st::settingsButtonNoIcon,
		.label = std::move(currentVal),
		.onClick = [=] {
			controller->show(Box(
				[=](not_null<Ui::GenericBox*> box) {
					const auto save = [=](int index) {
						PurrSettings::getInstance().setShowPeerId(
							static_cast<PeerIdDisplay>(index));
					};
					SingleChoiceBox(box, {
						.title = tr::purr_SettingsShowID(),
						.options = options,
						.initialSelection = static_cast<int>(settings->showPeerId()),
						.callback = save,
					});
				}));
		},
	});
}

void BuildQoLToggles(SectionBuilder &builder, PurrSectionBuilder &purr) {
	auto *settings = &PurrSettings::getInstance();

	BuildTranslator(builder, purr);
	purr.addSectionDivider();

	builder.addSubsectionTitle(tr::purr_CategoryGeneral());

	const auto controller = builder.controller();
	purr.addToggle({
		.id = u"purr/disableStories"_q,
		.altIds = { u"purr/hideStories"_q },
		.title = tr::purr_DisableStories(),
		.getter = [=] { return settings->disableStories(); },
		.setter = [=](bool enabled) {
			PurrSettings::getInstance().setDisableStories(enabled);
			ShowRestartPrompt(controller);
		},
	});

	purr.addSettingToggle({
		.id = u"purr/disableOpenLinkWarning"_q,
		.title = tr::purr_DisableOpenLinkWarning(),
		.getter = &PurrSettings::disableOpenLinkWarning,
		.setter = &PurrSettings::setDisableOpenLinkWarning,
	});

	purr.addCollapsibleToggle({
		.id = u"purr/similarChannels"_q,
		.title = tr::purr_DisableSimilarChannels(),
		.checkboxes = {
			NestedEntry{
				tr::purr_CollapseSimilarChannels(tr::now),
				[] { return PurrSettings::getInstance().collapseSimilarChannels(); },
				[](bool v) { PurrSettings::getInstance().setCollapseSimilarChannels(v); }
			},
			NestedEntry{
				tr::purr_HideSimilarChannelsTab(tr::now),
				[] { return PurrSettings::getInstance().hideSimilarChannels(); },
				[](bool v) { PurrSettings::getInstance().setHideSimilarChannels(v); }
			}
		},
		.toggledWhenAll = true,
	});

	purr.addSettingToggle({
		.id = u"purr/disableNotificationsDelay"_q,
		.title = tr::purr_DisableNotificationsDelay(),
		.getter = &PurrSettings::disableNotificationsDelay,
		.setter = &PurrSettings::setDisableNotificationsDelay,
	});

	purr.addSectionDivider();

	const auto zalgoButton = builder.addButton({
		.id = u"purr/filterZalgo"_q,
		.title = tr::purr_FilterZalgo(),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(settings->filterZalgo()),
	});
	if (zalgoButton) {
		zalgoButton->toggledValue(
		) | rpl::filter(
			[=](bool enabled) {
				return (enabled != settings->filterZalgo());
			}
		) | on_next(
			[=](bool enabled) {
				PurrSettings::getInstance().setFilterZalgo(enabled);
				ShowRestartPrompt(controller);
			},
			zalgoButton->lifetime());
		purr.addBetaBadge(zalgoButton);
	}

	purr.addSettingToggle({
		.id = u"purr/improveLinkPreviews"_q,
		.title = tr::purr_ImproveLinkPreviews(),
		.getter = &PurrSettings::improveLinkPreviews,
		.setter = &PurrSettings::setImproveLinkPreviews,
	});
	purr.addSettingToggle({
		.id = u"purr/showMessageSeconds"_q,
		.altIds = { u"purr/formatTimeWithSeconds"_q },
		.title = tr::purr_SettingsShowMessageSeconds(),
		.getter = &PurrSettings::showMessageSeconds,
		.setter = &PurrSettings::setShowMessageSeconds,
	});

	BuildShowPeerId(builder);

	purr.addSectionDivider();

	builder.addSubsectionTitle(rpl::single(QString("Webview")));

	purr.addSettingToggle({
		.id = u"purr/spoofWebviewAsAndroid"_q,
		.title = tr::purr_SettingsSpoofWebviewAsAndroid(),
		.getter = &PurrSettings::spoofWebviewAsAndroid,
		.setter = &PurrSettings::setSpoofWebviewAsAndroid,
	});

	purr.addCollapsibleToggle({
		.id = u"purr/biggerWindow"_q,
		.title = tr::purr_SettingsBiggerWindow(),
		.checkboxes = {
			NestedEntry{
				tr::purr_SettingsIncreaseWebviewHeight(tr::now),
				[] { return PurrSettings::getInstance().increaseWebviewHeight(); },
				[](bool v) { PurrSettings::getInstance().setIncreaseWebviewHeight(v); }
			},
			NestedEntry{
				tr::purr_SettingsIncreaseWebviewWidth(tr::now),
				[] { return PurrSettings::getInstance().increaseWebviewWidth(); },
				[](bool v) { PurrSettings::getInstance().setIncreaseWebviewWidth(v); }
			}
		},
		.toggledWhenAll = false,
	});

	purr.addSectionDivider();

	builder.addSubsectionTitle(tr::purr_ConfirmationsTitle());

	purr.addSettingToggle({
		.id = u"purr/stickerConfirmation"_q,
		.title = tr::purr_StickerConfirmation(),
		.getter = &PurrSettings::stickerConfirmation,
		.setter = &PurrSettings::setStickerConfirmation,
	});
	purr.addSettingToggle({
		.id = u"purr/gifConfirmation"_q,
		.title = tr::purr_GIFConfirmation(),
		.getter = &PurrSettings::gifConfirmation,
		.setter = &PurrSettings::setGifConfirmation,
	});
	purr.addSettingToggle({
		.id = u"purr/voiceConfirmation"_q,
		.title = tr::purr_VoiceConfirmation(),
		.getter = &PurrSettings::voiceConfirmation,
		.setter = &PurrSettings::setVoiceConfirmation,
	});
}

const auto kMeta = BuildHelper({
	.id = PurrGeneral::Id(),
	.parentId = PurrMain::Id(),
	.title = &tr::purr_CategoryGeneral,
	.icon = &st::menuIconShowAll,
}, [](SectionBuilder &builder) {
	auto purr = PurrSectionBuilder(builder);

	builder.addSkip();
	BuildQoLToggles(builder, purr);
	builder.addSkip();
});

} // namespace

rpl::producer<QString> PurrGeneral::title() {
	return tr::purr_CategoryGeneral();
}

PurrGeneral::PurrGeneral(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void PurrGeneral::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type PurrGeneralId() {
	return PurrGeneral::Id();
}

} // namespace Settings
