#include <qguiapplication.h>
#include <qlist.h>
#include <qlogging.h>
#include <qqml.h>
#include <qtenvironmentvariables.h>

#include "../core/plugin.hpp"

void installWlProxySafeDeref(); // NOLINT(misc-use-internal-linkage)
void installPlatformMenuHook(); // NOLINT(misc-use-internal-linkage)
void installPopupPositioner();  // NOLINT(misc-use-internal-linkage)

namespace {

class WaylandPlugin: public QsEnginePlugin {
	QList<QString> dependencies() override { return {"window"}; }

	bool applies() override {
		auto isWayland = QGuiApplication::platformName() == "wayland";

		if (!isWayland && !qEnvironmentVariable("WAYLAND_DISPLAY").isEmpty()) {
			qWarning() << "--- WARNING ---";
			qWarning() << "WAYLAND_DISPLAY is present but QT_QPA_PLATFORM is"
			           << QGuiApplication::platformName();
			qWarning() << "If you are actually running wayland, set QT_QPA_PLATFORM to \"wayland\" or "
			              "most functionality will be broken.";
		}

		return isWayland;
	}

	void preinit() override { installWlProxySafeDeref(); }

	void init() override {
		installPlatformMenuHook();
		installPopupPositioner();
	}
};

QS_REGISTER_PLUGIN(WaylandPlugin);

} // namespace
