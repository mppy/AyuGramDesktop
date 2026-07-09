// This is the source code of PurrGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "purr/ui/settings/settings_chats.h"

#include "lang_auto.h"
#include "purr/purr_settings.h"
#include "purr/ui/boxes/edit_mark_box.h"
#include "purr/ui/components/message_preview.h"
#include "purr/ui/settings/purr_builder.h"
#include "purr/ui/settings/settings_purr_utils.h"
#include "purr/ui/settings/settings_main.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_purr_icons.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

#include <memory>

namespace Settings {

using namespace Builder;
using namespace PurrBuilder;

namespace {

struct PreviewState {
	MessagePreview *widget = nullptr;
};

void BuildStickersAndEmoji(SectionBuilder &builder, PurrSectionBuilder &purr) {
	builder.addSubsectionTitle(tr::lng_settings_stickers_emoji());

	purr.addSettingToggle({
		.id = u"purr/showOnlyAddedEmojisAndStickers"_q,
		.title = tr::purr_ShowOnlyAddedEmojisAndStickers(),
		.getter = &PurrSettings::showOnlyAddedEmojisAndStickers,
		.setter = &PurrSettings::setShowOnlyAddedEmojisAndStickers,
	});

	purr.addCollapsibleToggle({
		.id = u"purr/hideReactions"_q,
		.title = tr::purr_HideReactions(),
		.checkboxes = {
			NestedEntry{
				tr::purr_HideReactionsInChannels(tr::now),
				[] { return !PurrSettings::getInstance().showChannelReactions(); },
				[](bool v) { PurrSettings::getInstance().setShowChannelReactions(!v); }
			},
			NestedEntry{
				tr::purr_HideReactionsInGroups(tr::now),
				[] { return !PurrSettings::getInstance().showGroupReactions(); },
				[](bool v) { PurrSettings::getInstance().setShowGroupReactions(!v); }
			},
			NestedEntry{
				tr::purr_HideReactionsInPrivateChats(tr::now),
				[] { return !PurrSettings::getInstance().showPrivateChatReactions(); },
				[](bool v) { PurrSettings::getInstance().setShowPrivateChatReactions(!v); }
			}
		},
		.toggledWhenAll = false,
	});

	purr.addSectionDivider();
}

void BuildRecentStickersLimit(SectionBuilder &builder, PurrSectionBuilder &purr) {
	auto *settings = &PurrSettings::getInstance();

	purr.addSlider({
		.id = u"purr/recentStickersCount"_q,
		.title = tr::purr_SettingsRecentStickersCount(),
		.steps = 200 + 1,
		.current = settings->recentStickersCount(),
		.indexToValue = [](int index) { return index; },
		.onChanged = nullptr,
		.onFinalChanged = [](int amount) {
			PurrSettings::getInstance().setRecentStickersCount(amount);
		},
		.formatLabel = [](int amount) { return QString::number(amount); },
	});

	purr.addSectionDivider();
}

void BuildGroupsAndChannels(SectionBuilder &builder, PurrSectionBuilder &purr) {
	auto *settings = &PurrSettings::getInstance();

	builder.addSubsectionTitle(tr::lng_premium_double_limits_subtitle_channels());

	purr.addChooseButton({
		.id = u"purr/channelBottomButton"_q,
		.altIds = { u"purr/bottomButton"_q },
		.title = tr::purr_ChannelBottomButton(),
		.boxTitle = tr::purr_ChannelBottomButton(),
		.initialSelection = static_cast<int>(settings->channelBottomButton()),
		.options = {
			tr::purr_ChannelBottomButtonHide(tr::now),
			tr::purr_ChannelBottomButtonMute(tr::now),
			tr::purr_ChannelBottomButtonDiscuss(tr::now),
		},
		.setter = [](int index) {
			PurrSettings::getInstance().setChannelBottomButton(
				static_cast<ChannelBottomButton>(index));
		},
	});

	purr.addSettingToggle({
		.id = u"purr/quickAdminShortcuts"_q,
		.title = tr::purr_QuickAdminShortcuts(),
		.getter = &PurrSettings::quickAdminShortcuts,
		.setter = &PurrSettings::setQuickAdminShortcuts,
	});
	purr.addSettingToggle({
		.id = u"purr/showMessageShot"_q,
		.title = tr::purr_SettingsShowMessageShot(),
		.getter = &PurrSettings::showMessageShot,
		.setter = &PurrSettings::setShowMessageShot,
	});

	builder.addSkip();
	builder.addDividerText(tr::purr_SettingsShowMessageShotDescription());
	builder.addSkip();
}

void BuildMarks(
		SectionBuilder &builder,
		PurrSectionBuilder &purr,
		std::shared_ptr<PreviewState> previewState) {
	auto *settings = &PurrSettings::getInstance();
	const auto controller = builder.controller();

	builder.addSubsectionTitle(tr::lng_settings_messages());

	builder.add([=](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		auto preview = object_ptr<MessagePreview>(ctx.container, controller);
		previewState->widget = preview.data();
		return {
			.widget = std::move(preview),
			.margin = style::margins(
				0,
				st::defaultVerticalListSkip,
				0,
				st::settingsPrivacySkipTop),
		};
	});

	purr.addSettingToggle({
		.id = u"purr/replaceBottomInfoWithIcons"_q,
		.altIds = { u"purr/replaceEditedWithIcon"_q },
		.title = tr::purr_ReplaceMarksWithIcons(),
		.getter = &PurrSettings::replaceBottomInfoWithIcons,
		.setter = &PurrSettings::setReplaceBottomInfoWithIcons,
	});

	builder.scope([&] {
		builder.addButton({
			.id = u"purr/deletedMark"_q,
			.title = tr::purr_DeletedMarkText(),
			.st = &st::settingsButtonNoIcon,
			.label = PurrSettings::getInstance().deletedMarkValue(),
			.onClick = [=] {
				auto box = Box<EditMarkBox>(
					tr::purr_DeletedMarkText(),
					settings->deletedMark(),
					QString("🧹"),
					[=](const QString &value) {
						PurrSettings::getInstance().setDeletedMark(value);
					});
				Ui::show(std::move(box));
			},
		});

		builder.addButton({
			.id = u"purr/editedMark"_q,
			.title = tr::purr_EditedMarkText(),
			.st = &st::settingsButtonNoIcon,
			.label = PurrSettings::getInstance().editedMarkValue(),
			.onClick = [=] {
				auto box = Box<EditMarkBox>(
					tr::purr_EditedMarkText(),
					settings->editedMark(),
					tr::lng_edited(tr::now),
					[=](const QString &value) {
						PurrSettings::getInstance().setEditedMark(value);
					});
				Ui::show(std::move(box));
			},
		});
	}, PurrSettings::getInstance().replaceBottomInfoWithIconsValue()
		| rpl::map([](bool v) { return !v; }));

	purr.addSettingToggle({
		.id = u"purr/removeMessageTail"_q,
		.title = tr::purr_RemoveMessageTail(),
		.getter = &PurrSettings::removeMessageTail,
		.setter = &PurrSettings::setRemoveMessageTail,
	});

	purr.addSettingToggle({
		.id = u"purr/hideFastShare"_q,
		.altIds = { u"purr/hideShareButton"_q },
		.title = tr::purr_HideShareButton(),
		.getter = &PurrSettings::hideFastShare,
		.setter = &PurrSettings::setHideFastShare,
	});
	purr.addSettingToggle({
		.id = u"purr/simpleQuotesAndReplies"_q,
		.altIds = { u"purr/disableColorfulReplies"_q, u"purr/replyElements"_q },
		.title = tr::purr_SimpleQuotesAndReplies(),
		.getter = &PurrSettings::simpleQuotesAndReplies,
		.setter = &PurrSettings::setSimpleQuotesAndReplies,
	});

	const auto semiTransparent = purr.addSettingToggle({
		.id = u"purr/semiTransparentDeletedMessages"_q,
		.altIds = { u"purr/translucentDeletedMessages"_q },
		.title = tr::purr_SemiTransparentDeletedMessages(),
		.getter = &PurrSettings::semiTransparentDeletedMessages,
		.setter = &PurrSettings::setSemiTransparentDeletedMessages,
	});
	if (semiTransparent) {
		purr.addBetaBadge(semiTransparent);
	}

	purr.addSectionDivider();
}

void BuildWideMessagesMultiplier(
		SectionBuilder &builder,
		PurrSectionBuilder &purr,
		std::shared_ptr<PreviewState> previewState) {
	auto *settings = &PurrSettings::getInstance();

	constexpr auto kMinSize = 1.00;
	constexpr auto kStep = 0.05;

	const auto valueToIndex = [=](double value) {
		return static_cast<int>(std::round((value - kMinSize) / kStep));
	};

	const auto controller = builder.controller();
	purr.addSlider({
		.id = u"purr/messageBubbleRadius"_q,
		.title = tr::purr_MessageBubbleRadius(),
		.steps = 17,
		.current = settings->messageBubbleRadius(),
		.indexToValue = [](int index) { return index; },
		.onChanged = [=](int index) {
			if (previewState->widget) {
				previewState->widget->setBubbleRadius(index);
			}
		},
		.onFinalChanged = [=](int index) {
			if (previewState->widget) {
				previewState->widget->setBubbleRadius(index);
			}
			PurrSettings::getInstance().setMessageBubbleRadius(index);
			ShowRestartPrompt(controller);
		},
		.formatLabel = [](int index) {
			return QString::number(index);
		},
	});

	purr.addSectionDivider();

	purr.addSlider({
		.id = u"purr/wideMultiplier"_q,
		.title = tr::purr_SettingsWideMultiplier(),
		.steps = 61, // (4.00 - 1.00) / 0.05 + 1
		.current = valueToIndex(settings->wideMultiplier()),
		.indexToValue = [](int index) { return index; },
		.onChanged = nullptr,
		.onFinalChanged = [=](int index) {
			PurrSettings::getInstance().setWideMultiplier(
				kMinSize + index * kStep);
			ShowRestartPrompt(controller);
		},
		.formatLabel = [=](int index) {
			return QString::number(kMinSize + index * kStep, 'f', 2);
		},
	});

	builder.addSkip();
	builder.addDividerText(tr::purr_SettingsWideMultiplierDescription());
	builder.addSkip();
}

void BuildContextMenuElements(SectionBuilder &builder, PurrSectionBuilder &purr) {
	auto *settings = &PurrSettings::getInstance();

	builder.addSubsectionTitle(tr::purr_ContextMenuElementsHeader());

	const auto options = std::vector{
		tr::purr_SettingsContextMenuItemHidden(tr::now),
		tr::purr_SettingsContextMenuItemShown(tr::now),
		tr::purr_SettingsContextMenuItemExtended(tr::now),
	};

	purr.addChooseButton({
		.id = u"purr/showReactionsPanelInContextMenu"_q,
		.title = tr::purr_SettingsContextMenuReactionsPanel(),
		.boxTitle = tr::purr_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showReactionsPanelInContextMenu()),
		.options = options,
		.setter = [](int i) { PurrSettings::getInstance().setShowReactionsPanelInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::menuIconReactions },
	});
	purr.addChooseButton({
		.id = u"purr/showViewsPanelInContextMenu"_q,
		.title = tr::purr_SettingsContextMenuViewsPanel(),
		.boxTitle = tr::purr_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showViewsPanelInContextMenu()),
		.options = options,
		.setter = [](int i) { PurrSettings::getInstance().setShowViewsPanelInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::menuIconShowInChat },
	});
	purr.addChooseButton({
		.id = u"purr/showHideMessageInContextMenu"_q,
		.title = tr::purr_ContextHideMessage(),
		.boxTitle = tr::purr_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showHideMessageInContextMenu()),
		.options = options,
		.setter = [](int i) { PurrSettings::getInstance().setShowHideMessageInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::menuIconClear },
	});
	purr.addChooseButton({
		.id = u"purr/showUserMessagesInContextMenu"_q,
		.title = tr::purr_UserMessagesMenuText(),
		.boxTitle = tr::purr_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showUserMessagesInContextMenu()),
		.options = options,
		.setter = [](int i) { PurrSettings::getInstance().setShowUserMessagesInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::menuIconTTL },
	});
	purr.addChooseButton({
		.id = u"purr/showMessageDetailsInContextMenu"_q,
		.title = tr::purr_MessageDetailsPC(),
		.boxTitle = tr::purr_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showMessageDetailsInContextMenu()),
		.options = options,
		.setter = [](int i) { PurrSettings::getInstance().setShowMessageDetailsInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::menuIconInfo },
	});
	purr.addChooseButton({
		.id = u"purr/showRepeatMessageInContextMenu"_q,
		.title = tr::purr_RepeatMessage(),
		.boxTitle = tr::purr_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showRepeatMessageInContextMenu()),
		.options = options,
		.setter = [](int i) { PurrSettings::getInstance().setShowRepeatMessageInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::purrRepeatMenuIcon },
	});
	if (settings->filtersEnabled()) {
		purr.addChooseButton({
			.id = u"purr/showAddFilterInContextMenu"_q,
			.title = tr::purr_RegexFilterQuickAdd(),
			.boxTitle = tr::purr_SettingsContextMenuTitle(),
			.initialSelection = static_cast<int>(settings->showAddFilterInContextMenu()),
			.options = options,
			.setter = [](int i) { PurrSettings::getInstance().setShowAddFilterInContextMenu(static_cast<ContextMenuVisibility>(i)); },
			.icon = { &st::menuIconAddToFolder },
		});
	}

	builder.addSkip();
	builder.addDividerText(tr::purr_SettingsContextMenuDescription());
	builder.addSkip();
}

void BuildMessageFieldElements(SectionBuilder &builder, PurrSectionBuilder &purr) {
	builder.addSubsectionTitle(tr::purr_MessageFieldElementsHeader());

	purr.addSettingToggle({
		.id = u"purr/showAttachButtonInMessageField"_q,
		.title = tr::purr_MessageFieldElementAttach(),
		.getter = &PurrSettings::showAttachButtonInMessageField,
		.setter = &PurrSettings::setShowAttachButtonInMessageField,
		.icon = { &st::messageFieldAttachIcon },
	});
	purr.addSettingToggle({
		.id = u"purr/showCommandsButtonInMessageField"_q,
		.title = tr::purr_MessageFieldElementCommands(),
		.getter = &PurrSettings::showCommandsButtonInMessageField,
		.setter = &PurrSettings::setShowCommandsButtonInMessageField,
		.icon = { &st::messageFieldCommandsIcon },
	});
	purr.addSettingToggle({
		.id = u"purr/showAutoDeleteButtonInMessageField"_q,
		.title = tr::purr_MessageFieldElementTTL(),
		.getter = &PurrSettings::showAutoDeleteButtonInMessageField,
		.setter = &PurrSettings::setShowAutoDeleteButtonInMessageField,
		.icon = { &st::messageFieldTTLIcon },
	});
	purr.addSettingToggle({
		.id = u"purr/showEmojiButtonInMessageField"_q,
		.title = tr::purr_MessageFieldElementEmoji(),
		.getter = &PurrSettings::showEmojiButtonInMessageField,
		.setter = &PurrSettings::setShowEmojiButtonInMessageField,
		.icon = { &st::messageFieldEmojiIcon },
	});
	purr.addSettingToggle({
		.id = u"purr/showMicrophoneButtonInMessageField"_q,
		.title = tr::purr_MessageFieldElementVoice(),
		.getter = &PurrSettings::showMicrophoneButtonInMessageField,
		.setter = &PurrSettings::setShowMicrophoneButtonInMessageField,
		.icon = { &st::messageFieldVoiceIcon },
	});
	purr.addSettingToggle({
		.id = u"purr/showGiftButtonInMessageField"_q,
		.title = tr::lng_profile_action_short_gift(),
		.getter = &PurrSettings::showGiftButtonInMessageField,
		.setter = &PurrSettings::setShowGiftButtonInMessageField,
		.icon = { &st::settingsButtonIconGift },
	});
	purr.addSettingToggle({
		.id = u"purr/showAiEditorButtonInMessageField"_q,
		.title = tr::lng_ai_compose_title(),
		.getter = &PurrSettings::showAiEditorButtonInMessageField,
		.setter = &PurrSettings::setShowAiEditorButtonInMessageField,
		.icon = { &st::messageFieldCocoonAiIcon },
	});

	purr.addSectionDivider();
}

void BuildMessageFieldPopups(SectionBuilder &builder, PurrSectionBuilder &purr) {
	builder.addSubsectionTitle(tr::purr_MessageFieldPopupsHeader());

	purr.addSettingToggle({
		.id = u"purr/showAttachPopup"_q,
		.title = tr::purr_MessageFieldElementAttach(),
		.getter = &PurrSettings::showAttachPopup,
		.setter = &PurrSettings::setShowAttachPopup,
		.icon = { &st::messageFieldAttachIcon },
	});
	purr.addSettingToggle({
		.id = u"purr/showEmojiPopup"_q,
		.title = tr::purr_MessageFieldElementEmoji(),
		.getter = &PurrSettings::showEmojiPopup,
		.setter = &PurrSettings::setShowEmojiPopup,
		.icon = { &st::messageFieldEmojiIcon },
	});
}

const auto kMeta = BuildHelper({
	.id = PurrChats::Id(),
	.parentId = PurrMain::Id(),
	.title = &tr::purr_CategoryChats,
	.icon = &st::menuIconChatBubble,
}, [](SectionBuilder &builder) {
	auto purr = PurrSectionBuilder(builder);
	const auto previewState = std::make_shared<PreviewState>();

	builder.addSkip();
	BuildStickersAndEmoji(builder, purr);
	BuildRecentStickersLimit(builder, purr);
	BuildGroupsAndChannels(builder, purr);
	BuildMarks(builder, purr, previewState);
	BuildWideMessagesMultiplier(builder, purr, previewState);
	BuildContextMenuElements(builder, purr);
	BuildMessageFieldElements(builder, purr);
	BuildMessageFieldPopups(builder, purr);
	builder.addSkip();
});

} // namespace

rpl::producer<QString> PurrChats::title() {
	return tr::purr_CategoryChats();
}

PurrChats::PurrChats(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void PurrChats::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type PurrChatsId() {
	return PurrChats::Id();
}

} // namespace Settings
