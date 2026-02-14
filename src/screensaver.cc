#include "screensaver.h"
#include <NickelHook.h>

#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QDesktopWidget>
#include <QScreen>

typedef void N3PowerWorkflowManager;
typedef void PowerViewController;
typedef QWidget BookCoverDragonPowerView;

void (*N3PowerWorkflowManager_handleSleep)(N3PowerWorkflowManager* self);
void (*N3PowerWorkflowManager_showSleepView)(N3PowerWorkflowManager* self);

void* (*MainWindowController_sharedInstance)();
QWidget* (*MainWindowController_currentView)(void*);
void (*BookCoverDragonPowerView_setInfoPanelVisible)(BookCoverDragonPowerView* self, bool visible);
void (*FullScreenDragonPowerView_setInfoPanelVisible)(QWidget* self, bool visible);

struct nh_info nickelscreensaver = {
    .name = "Nickel Screensaver",
    .desc = "Screen-freeze screensaver for Kobo",
    .uninstall_flag = NICKEL_SCREENSAVER_DELETE_FILE,
};

int ns_init() {
    return 0;
}

bool ns_uninstall() {
    return true;
}

struct nh_hook nickelscreensaverHook[] = {
    {
        .sym     = "_ZN22N3PowerWorkflowManager13showSleepViewEv", 
        .sym_new = "ns_show_sleep_view",
        .lib     = "libnickel.so.1.0.0",
        .out     = nh_symoutptr(N3PowerWorkflowManager_showSleepView),
        .desc    = "Show sleep view"
    },
    {
        .sym     = "_ZN22N3PowerWorkflowManager11handleSleepEv", 
        .sym_new = "ns_handle_sleep",
        .lib     = "libnickel.so.1.0.0",
        .out     = nh_symoutptr(N3PowerWorkflowManager_handleSleep),
        .desc    = "Handle sleep"
    },
    {0}
};

struct nh_dlsym nickelscreensaverDlsym[] = {
    {
		.name = "_ZN20MainWindowController14sharedInstanceEv",
		.out  = nh_symoutptr(MainWindowController_sharedInstance),
	},
	{
		.name = "_ZNK20MainWindowController11currentViewEv",
		.out  = nh_symoutptr(MainWindowController_currentView),
	},
    {
        .name = "_ZN24BookCoverDragonPowerView19setInfoPanelVisibleEb",
        .out  = nh_symoutptr(BookCoverDragonPowerView_setInfoPanelVisible),
        .desc = "",
        .optional = true,
    },
    {
        .name = "_ZN25FullScreenDragonPowerView19setInfoPanelVisibleEb",
        .out  = nh_symoutptr(FullScreenDragonPowerView_setInfoPanelVisible),
        .desc = "",
        .optional = true,
    },
	{0}
};

NickelHook(
    .init      = &ns_init,
    .info      = &nickelscreensaver,
    .hook      = nickelscreensaverHook,
    .dlsym     = nickelscreensaverDlsym,
    .uninstall = &ns_uninstall,
);

// Note: QPixmap is ref-counted, backing data is COW if more than one reference
QPixmap screensaver_pixmap;

extern "C" __attribute__((visibility("default")))
void ns_handle_sleep(N3PowerWorkflowManager* self) {
    screensaver_pixmap = QPixmap();

    void *mwc = MainWindowController_sharedInstance();
	if (!mwc) {
		nh_log("Invalid MainWindowController");
		return N3PowerWorkflowManager_handleSleep(self);
	}

    QWidget *current_view = MainWindowController_currentView(mwc);
	if (!current_view) {
		nh_log("Invalid currentView");
		return N3PowerWorkflowManager_handleSleep(self);
	}

    // Grab the current screen contents
    QDesktopWidget* desktop_widget = QApplication::desktop();
    QScreen* screen = QGuiApplication::primaryScreen();
    QSize screen_size = screen->size();

    QRect geometry = current_view->geometry();
    QPixmap screenshot = screen->grabWindow(
        desktop_widget->winId(),
        geometry.left(),
        geometry.top(),
        geometry.width(),
        geometry.height()
    );

    if (screenshot.isNull()) {
        nh_log("Failed to grab screenshot");
        return N3PowerWorkflowManager_handleSleep(self);
    }

    if (screenshot.size() != screen_size) {
        screensaver_pixmap = screenshot.scaled(screen_size, Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
    } else {
        screensaver_pixmap = screenshot;
    }

    N3PowerWorkflowManager_handleSleep(self);
}

extern "C" __attribute__((visibility("default")))
void ns_show_sleep_view(N3PowerWorkflowManager* self) {
    N3PowerWorkflowManager_showSleepView(self);

    if (screensaver_pixmap.isNull()) {
        return;
    }

    void *mwc = MainWindowController_sharedInstance();
    if (!mwc) {
        return;
    }

    QWidget *current_view = MainWindowController_currentView(mwc);
    if (!current_view) {
        return;
    }

    // Overlay the screenshot on top of whatever sleep view Kobo shows
    QLabel* overlay = new QLabel(current_view);
    overlay->setPixmap(screensaver_pixmap);
    overlay->setGeometry(current_view->rect());
    overlay->raise();
    overlay->show();
}