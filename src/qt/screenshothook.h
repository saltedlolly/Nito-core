// LOCAL TESTING ONLY - not part of any PR.
// When NITO_SCREENSHOT_DIR is set, wait for the file <dir>/go, then click
// through the main pages, open the console, and save each window as a PNG.
#ifndef NITO_QT_SCREENSHOTHOOK_H
#define NITO_QT_SCREENSHOTHOOK_H

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMainWindow>
#include <QMetaObject>
#include <QPixmap>
#include <QProgressBar>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>

#include <functional>
#include <memory>
#include <vector>

#include <util/chaintype.h>

//! LOCAL ONLY: show another network's theme and badge, for screenshots made with regtest data.
inline ChainType NitoScreenshotChain(ChainType real)
{
    const QString chain{qEnvironmentVariable("NITO_SCREENSHOT_CHAIN")};
    if (chain == "main") return ChainType::MAIN;
    if (chain == "test") return ChainType::TESTNET;
    if (chain == "signet") return ChainType::SIGNET;
    return real;
}

inline void NitoScreenshotHook(QMainWindow* window)
{
    const QString dir{qEnvironmentVariable("NITO_SCREENSHOT_DIR")};
    if (dir.isEmpty()) return;
    const QString prefix{qEnvironmentVariable("NITO_SCREENSHOT_PREFIX")};

    auto trigger = [window](const QString& text) {
        for (QAction* action : window->findChildren<QAction*>()) {
            QString t{action->text()};
            t.remove(QLatin1Char('&'));
            if (t == text) { action->trigger(); return; }
        }
    };
    auto save = [dir, prefix](QWidget* w, const QString& name) {
        w->grab().save(QDir(dir).filePath(prefix + name + ".png"));
    };
    auto top = [](const char* class_name) -> QWidget* {
        for (QWidget* w : QApplication::topLevelWidgets()) {
            if (w->isVisible() && w->inherits(class_name)) return w;
        }
        return nullptr;
    };

    using Step = std::function<void()>;
    auto steps = std::make_shared<std::vector<Step>>();
    auto overlay = [window]() -> QWidget* {
        for (QWidget* w : window->findChildren<QWidget*>()) if (w->inherits("ModalOverlay")) return w;
        return nullptr;
    };
    steps->push_back([window, overlay] {
        window->resize(1000, 660); window->raise(); window->activateWindow();
        if (QWidget* o = overlay(); o && o->y() == 0) QMetaObject::invokeMethod(o, "toggleVisibility");
    });
    for (const char* page : {"Overview", "Send", "Receive", "Transactions"}) {
        const QString p{QString::fromLatin1(page)};
        steps->push_back([trigger, p] { trigger(p); });
        steps->push_back([save, window, p] { save(window, "main-" + p.toLower()); });
    }
    steps->push_back([overlay] { if (QWidget* o = overlay(); o && o->y() != 0) QMetaObject::invokeMethod(o, "toggleVisibility"); });
    steps->push_back([save, window] { save(window, "overlay"); });
    steps->push_back([overlay] { if (QWidget* o = overlay(); o && o->y() == 0) QMetaObject::invokeMethod(o, "toggleVisibility"); });
    steps->push_back([save, window] {
        if (auto* bar = window->findChild<QProgressBar*>()) {
            save(bar, "bar-full");
            bar->setValue(bar->minimum() + (bar->maximum() - bar->minimum()) * 4 / 10);
            save(bar, "bar-part");
        }
    });
    steps->push_back([trigger] { trigger(QStringLiteral("Console")); });
    steps->push_back([top] { if (QWidget* c = top("RPCConsole")) c->resize(820, 560); });
    steps->push_back([save, top] { if (QWidget* c = top("RPCConsole")) save(c, "console"); });
    steps->push_back([top] {
        if (QWidget* c = top("RPCConsole")) {
            if (auto* tabs = c->findChild<QTabWidget*>()) tabs->setCurrentIndex(0);
        }
    });
    steps->push_back([save, top] { if (QWidget* c = top("RPCConsole")) save(c, "information"); });
    steps->push_back([dir] { QFile f(QDir(dir).filePath("done")); f.open(QIODevice::WriteOnly); });

    auto run = std::make_shared<std::function<void(size_t)>>();
    *run = [steps, run](size_t i) {
        if (i >= steps->size()) return;
        (*steps)[i]();
        QTimer::singleShot(900, [run, i] { (*run)(i + 1); });
    };
    auto* poll = new QTimer(window);
    QObject::connect(poll, &QTimer::timeout, window, [poll, dir, run] {
        if (!QFile::exists(QDir(dir).filePath("go"))) return;
        QFile::remove(QDir(dir).filePath("go"));
        poll->stop();
        (*run)(0);
    });
    poll->start(500);
}

#endif // NITO_QT_SCREENSHOTHOOK_H
