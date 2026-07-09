// This is the source code of PurrGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "purr/purr_infra.h"

#include "purr/purr_lang.h"
#include "purr/purr_settings.h"
#include "purr/purr_ui_settings.h"
#include "purr/purr_worker.h"
#include "purr/data/purr_database.h"
#include "purr/ui/purr_logo.h"
#include "features/translator/purr_translator.h"
#include "lang/lang_instance.h"
#include "ui/chat/chat_style_radius.h"
#include "utils/rc_manager.h"

#ifdef Q_OS_WIN
#include "purr/utils/windows_utils.h"
#endif

namespace PurrInfra {

void initLang() {
	QString id = Lang::GetInstance().id();
	QString baseId = Lang::GetInstance().baseId();
	if (id.isEmpty()) {
		LOG(("Language is not loaded"));
		return;
	}
	PurrLanguage::init();
	PurrLanguage::currentInstance()->fetchLanguage(id, baseId);
}

void initUiSettings() {
	const auto &settings = PurrSettings::getInstance();

	PurrUiSettings::setMonoFont(settings.monoFont());
	PurrUiSettings::setWideMultiplier(settings.wideMultiplier());
	PurrUiSettings::setMaterialSwitches(settings.materialSwitches());
	PurrUiSettings::setAvatarCorners(settings.avatarCorners());
	Ui::SetAppliedBubbleRadius(settings.messageBubbleRadius());
}

void initDatabase() {
	PurrDatabase::initialize();
}

void initWorker() {
	PurrWorker::initialize();
}

void initRCManager() {
	RCManager::getInstance().start();
}

void initTranslator() {
	Purr::Translator::TranslateManager::init();
}

void initIcon() {
#ifdef Q_OS_WIN
	PurrAssets::loadAppIco();
	reloadAppIconFromTaskBar();
#endif
}

void init() {
	PurrSettings::load();
	initLang();
	initDatabase();
	initUiSettings();
	initIcon();
	initWorker();
	initRCManager();
	initTranslator();
}

}
