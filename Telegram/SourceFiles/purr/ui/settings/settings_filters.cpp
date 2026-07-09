// This is the source code of PurrGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "purr/ui/settings/settings_filters.h"

#include "lang_auto.h"
#include "purr/purr_settings.h"
#include "purr/data/purr_database.h"
#include "purr/features/filters/filters_cache_controller.h"
#include "purr/ui/boxes/import_filters_box.h"
#include "purr/ui/settings/purr_builder.h"
#include "purr/ui/settings/settings_main.h"
#include "purr/utils/telegram_helpers.h"
#include "boxes/abstract_box.h"
#include "boxes/peer_list_box.h"
#include "core/application.h"
#include "filters/per_dialog_filter.h"
#include "filters/settings_filters_list.h"
#include "inline_bots/bot_attach_web_view.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_purr_icons.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/vertical_list.h"
#include "ui/boxes/confirm_box.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/menu/menu_add_action_callback.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_controller.h"
#include "window/window_peer_menu.h"
#include "window/window_session_controller.h"

namespace Settings {

using namespace Builder;
using namespace PurrBuilder;

namespace {

void BuildFiltersSettings(SectionBuilder &builder) {
	auto *settings = &PurrSettings::getInstance();

	builder.addSkip();
	builder.addSubsectionTitle(tr::purr_RegexFilters());

	const auto enabledButton = builder.addButton({
		.id = u"purr/filtersEnabled"_q,
		.title = tr::purr_RegexFiltersEnable(),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(settings->filtersEnabled()),
	});
	if (enabledButton) {
		enabledButton->toggledValue(
		) | rpl::filter([=](bool enabled) {
			return (enabled != settings->filtersEnabled());
		}) | on_next([=](bool enabled) {
			PurrSettings::getInstance().setFiltersEnabled(enabled);
			FiltersCacheController::rebuildCache();
			FiltersCacheController::fireUpdate();
		}, enabledButton->lifetime());
	}

	const auto sharedButton = builder.addButton({
		.id = u"purr/filtersEnabledInChats"_q,
		.altIds = { u"purr/filtersInChats"_q },
		.title = tr::purr_RegexFiltersEnableSharedInChats(),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(settings->filtersEnabledInChats()),
	});
	if (sharedButton) {
		sharedButton->toggledValue(
		) | rpl::filter([=](bool enabled) {
			return (enabled != settings->filtersEnabledInChats());
		}) | on_next([=](bool enabled) {
			PurrSettings::getInstance().setFiltersEnabledInChats(enabled);
			FiltersCacheController::rebuildCache();
			FiltersCacheController::fireUpdate();
		}, sharedButton->lifetime());
	}

	const auto blockedButton = builder.addButton({
		.id = u"purr/hideFromBlocked"_q,
		.title = tr::purr_FiltersHideFromBlocked(),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(settings->hideFromBlocked()),
	});
	if (blockedButton) {
		blockedButton->toggledValue(
		) | rpl::filter([=](bool enabled) {
			return (enabled != settings->hideFromBlocked());
		}) | on_next([=](bool enabled) {
			PurrSettings::getInstance().setHideFromBlocked(enabled);
			FiltersCacheController::rebuildCache();
			FiltersCacheController::fireUpdate();
		}, blockedButton->lifetime());
	}

	builder.addSkip();
}

void BuildShared(SectionBuilder &builder) {
	builder.addDivider();
	builder.addSkip();

	const auto controller = builder.controller();
	builder.addButton({
		.id = u"purr/sharedFilters"_q,
		.title = tr::purr_RegexFiltersShared(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			controller->dialogId = std::nullopt;
			controller->showExclude = false;
			controller->showSettings(PurrFiltersList::Id());
		},
	});
}

void BuildShadowBan(SectionBuilder &builder) {
	const auto controller = builder.controller();

	builder.addButton({
		.id = u"purr/shadowBanIds"_q,
		.altIds = { u"purr/shadowBanList"_q },
		.title = tr::purr_FiltersShadowBan(),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			controller->dialogId = std::nullopt;
			controller->showExclude = false;
			controller->shadowBan = true;
			controller->showSettings(PurrFiltersList::Id());
		},
	});
}

void BuildPerDialog(SectionBuilder &builder) {
	builder.add([](const BuildContext &ctx) {
		v::match(ctx, [&](const WidgetContext &wctx) {
			if (!PurrDatabase::hasPerDialogFilters()) {
				return;
			}

			const auto container = wctx.container;
			const auto controller = wctx.controller;

			AddSkip(container);
			AddDivider(container);

			auto ctrl = container->lifetime().make_state<PerDialogFiltersListController>(
				&controller->session(),
				controller);

			auto list = object_ptr<Ui::PaddingWrap<PeerListContent>>(
				container,
				object_ptr<PeerListContent>(
					container,
					ctrl),
				QMargins(0, -st::peerListBox.padding.top(), 0, -st::peerListBox.padding.bottom()));
			AddSkip(container);
			const auto content = container->add(std::move(list));
			AddSkip(container);
			auto delegate = container->lifetime().make_state<PeerListContentDelegateSimple>();
			delegate->setContent(content->entity());
			ctrl->setDelegate(delegate);
		}, [&](const SearchContext &) {
		});
	});
}

const auto kMeta = BuildHelper({
	.id = PurrFilters::Id(),
	.parentId = PurrMain::Id(),
	.title = &tr::purr_CategoryFilters,
	.icon = &st::menuIconTagFilter,
}, [](SectionBuilder &builder) {
	BuildFiltersSettings(builder);
	BuildShared(builder);
	BuildShadowBan(builder);
	BuildPerDialog(builder);
});

} // namespace

rpl::producer<QString> PurrFilters::title() {
	return tr::purr_CategoryFilters();
}

void PurrFilters::fillTopBarMenu(const Ui::Menu::MenuCallback &addAction) {
	addAction(
		tr::purr_FiltersMenuSelectChat(tr::now),
		[=] {
			if (const auto window = Core::App().activeWindow()) {
				if (const auto controller = window->sessionController()) {
					auto types = InlineBots::PeerTypes();
					types |= InlineBots::PeerType::Bot;
					types |= InlineBots::PeerType::Group;
					types |= InlineBots::PeerType::Broadcast;

					Window::ShowChooseRecipientBox(
						controller,
						[=](not_null<Data::Thread*> thread) {
							const auto peer = thread->peer();
							controller->dialogId = getDialogIdFromPeer(peer);
							controller->showExclude = true;
							controller->showSettings(PurrFiltersList::Id());
							return true;
						},
						tr::purr_FiltersMenuSelectChat(),
						nullptr,
						types);
				}
			}
		},
		&st::menuIconSearch);
	addAction({ .isSeparator = true });
	addAction(
		tr::purr_FiltersMenuImport(tr::now),
		[=] {
			auto box = Box(Ui::FillImportFiltersBox, true);
			Ui::show(std::move(box));
		},
		&st::menuIconArchive);
	if (PurrDatabase::hasFilters()) {
		addAction(
			tr::purr_FiltersMenuExport(tr::now),
			[=] {
				auto box = Box(Ui::FillImportFiltersBox, false);
				Ui::show(std::move(box));
			},
			&st::menuIconUnarchive);
	}
	addAction({ .isSeparator = true });
	addAction({
		.text = tr::purr_FiltersMenuClear(tr::now),
		.handler = [=] {
			auto callback = [=](Fn<void()> &&close) {
				PurrDatabase::deleteAllFilters();
				PurrDatabase::deleteAllExclusions();
				FiltersCacheController::rebuildCache();
				FiltersCacheController::fireUpdate();
				close();
			};
			auto box = Ui::MakeConfirmBox({
				.text = tr::purr_FiltersClearPopupText(),
				.confirmed = callback,
				.confirmText = tr::purr_FiltersClearPopupActionText(),
				.confirmStyle = &st::attentionBoxButton,
			});
			Ui::show(std::move(box));
		},
		.icon = &st::menuIconClearAttention,
		.isAttention = true,
	});
}

PurrFilters::PurrFilters(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void PurrFilters::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type PurrFiltersId() {
	return PurrFilters::Id();
}

} // namespace Settings
