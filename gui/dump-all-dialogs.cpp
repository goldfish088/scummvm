/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "gui/dump-all-dialogs.h"

#include "common/file.h"
#include "common/fs.h"
#include "common/language.h"
#include "common/translation.h"

#include "gui/ThemeEngine.h"
#include "gui/about.h"
#include "gui/gui-manager.h"
#include "gui/launcher.h"
#include "gui/message.h"
#include "gui/browser.h"
#include "gui/downloaddialog.h"
#include "gui/remotebrowser.h"
#include "gui/chooser.h"
#include "gui/cloudconnectionwizard.h"
#include "gui/dialog.h"
#include "gui/downloaddialog.h"
#include "gui/downloadpacksdialog.h"
#include "gui/fluidsynth-dialog.h"
#include "gui/themebrowser.h"
#include "gui/massadd.h"
#include "gui/options.h"
#include "gui/widgets/tab.h"
#include "gui/launcher.h"

#include "image/png.h"

namespace GUI {

void saveGUISnapshot(const Graphics::ManagedSurface &surf, const Common::String &filename) {
	Common::DumpFile outFile;
	Common::String outName = Common::String::format("snapshots/%s", filename.c_str());

	if (outFile.open(Common::Path(outName, '/'))) {
		Image::writePNG(outFile, surf);
		outFile.finalize();
		outFile.close();

		warning("Dumped %s", filename.c_str());
	}
}

void handleSimpleDialog(GUI::Dialog &dialog, const Common::String &filename, const Graphics::ManagedSurface &surf) {
	dialog.open();         // For rendering
	dialog.reflowLayout(); // For updating surface
	g_gui.redrawFull();
	saveGUISnapshot(surf, filename);
	dialog.close();
}

void loopThroughTabs(GUI::Dialog &dialog, const Common::String &lang, const Graphics::ManagedSurface &surf, const Common::String &name) {
	dialog.open();
	GUI::Widget *widget = nullptr;
	widget = dialog.findWidget((uint32)kTabWidget);

	if (widget) {
		TabWidget *tabWidget = (TabWidget *)widget;
		for (int tabNo = 0; tabNo < tabWidget->getTabCount(); tabNo++) {
			Common::String suffix = Common::String::format("-%d-%dx%d-%s.png", tabNo + 1, g_system->getOverlayWidth(), g_system->getOverlayHeight(), lang.c_str());
			tabWidget->setActiveTab(tabNo);
			handleSimpleDialog(dialog, name + suffix, surf);
		}
	}
	dialog.close();
}

void dumpDialogs(const Common::String &lang, const Common::String &message, int width, int height) {
#ifdef USE_TRANSLATION
	// Update GUI language
	TransMan.setLanguage(lang);
#endif

	Graphics::ManagedSurface &surf = g_gui.theme()->getScreenSurface();

	Common::String suffix = Common::String::format("-%dx%d-%s.png", width, height, lang.c_str());

	// Skipping Tooltips as not required

	// MessageDialog
	GUI::MessageDialog messageDialog(message);
	handleSimpleDialog(messageDialog, "messageDialog" + suffix, surf);
	// AboutDialog
	GUI::AboutDialog aboutDialog;
	handleSimpleDialog(aboutDialog, "aboutDialog" + suffix, surf);

#ifdef USE_CLOUD
	// CloudConnectingWizard
	GUI::CloudConnectionWizard cloudConnectingWizard;
	handleSimpleDialog(cloudConnectingWizard, "cloudConnectingWizard" + suffix, surf);

	// RemoteBrowserDialog
	GUI::RemoteBrowserDialog remoteBrowserDialog(_("Select directory with game data"));
	handleSimpleDialog(remoteBrowserDialog, "remoteBrowserDialog" + suffix, surf);
#endif

#ifdef USE_HTTP
	// DownloadIconPacksDialog
	GUI::DownloadPacksDialog downloadIconPacksDialog(_("icon packs"), "LIST", "gui-icons*.dat");
	handleSimpleDialog(downloadIconPacksDialog, "downloadIconPacksDialog" + suffix, surf);

	// DownloadShaderPacksDialog
	GUI::DownloadPacksDialog downloadShaderPacksDialog(_("shader packs"), "LIST-SHADERS", "shaders*.dat");
	handleSimpleDialog(downloadShaderPacksDialog, "downloadShaderPacksDialog" + suffix, surf);
#endif

#ifdef USE_FLUIDSYNTH
	// FluidSynthSettingsDialog
	GUI::FluidSynthSettingsDialog fluidSynthSettingsDialog;
	handleSimpleDialog(fluidSynthSettingsDialog, "fluidSynthSettings-" + suffix, surf);
#endif

	// ThemeBrowserDialog
	GUI::ThemeBrowser themeBrowser;
	handleSimpleDialog(themeBrowser, "themeBrowser-" + suffix, surf);

	// BrowserDialog
	GUI::BrowserDialog browserDialog(_("Select directory with game data"), true);
	handleSimpleDialog(browserDialog, "browserDialog-" + suffix, surf);

	// ChooserDialog
	GUI::ChooserDialog chooserDialog(_("Pick the game:"));
	handleSimpleDialog(chooserDialog, "chooserDialog-" + suffix, surf);

	// MassAddDialog
	GUI::MassAddDialog massAddDialog(Common::FSNode("."));
	handleSimpleDialog(massAddDialog, "massAddDialog-" + suffix, surf);

	// GlobalOptionsDialog
	LauncherSimple launcherDialog("Launcher");
	GUI::GlobalOptionsDialog globalOptionsDialog(&launcherDialog);
	loopThroughTabs(globalOptionsDialog, lang, surf, "GlobalOptionDialog");

	// LauncherDialog
#if 0
	GUI::LauncherChooser chooser;
	chooser.selectLauncher();
	chooser.open();
	saveGUISnapshot(surf, "launcher-" + filename);
	chooser.close();
#endif
}

void dumpAllDialogs(const Common::String &message) {
	Common::List<Common::String> list = Common::getLanguageList();
	// Common::List<Common::String> list;
	// list.push_back("en");
	Common::Pair<int, int> resolutions[] {
		{320, 200},
		{320, 240},
		{640, 400},
		{640, 480},
		{800, 600},
	};

	Common::FSNode dumpDir("snapshots");

	if (!dumpDir.isDirectory())
		dumpDir.createDirectory();

	Graphics::PixelFormat pixelFormat = g_system->getOverlayFormat();

	for (const auto &res : resolutions) {
		int w = res.first, h = res.second;
		g_gui.theme()->updateSurfaceDimensions(w, h, pixelFormat);
		for (const auto &lang : list) dumpDialogs(message, lang, w, h);
	}

	g_system->quit();
}

} // End of namespace GUI
