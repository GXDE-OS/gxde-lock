#include "lockcontent.h"

#include "controlwidget.h"
#include "LockAppearance.h"
#include "LockTranslations.h"
#include "userinputwidget.h"
#include "sessionbasemodel.h"
#include "userframe.h"
#include "sessionwidget.h"
#include "shutdownwidget.h"
#include "virtualkbinstance.h"

#include <QDBusConnection>
#include <QMouseEvent>

namespace {

const QString kLockSystemService = QStringLiteral("top.gxde.Lock");
const QString kLockSystemPath = QStringLiteral("/top/gxde/Lock");
const QString kLockSystemInterface =
    QStringLiteral("top.gxde.Lock.System");

}  // namespace

LockContent::LockContent(SessionBaseModel * const model, QWidget *parent)
    : SessionBaseWindow(parent)
    , m_model(model)
    , m_virtualKB(nullptr)
{
    m_controlWidget = new ControlWidget;
    m_userInputWidget = new UserInputWidget;
    m_userFrame = new UserFrame;
    m_shutdownFrame = new ShutdownWidget;

    m_userInputWidget->updateAuthType(model->currentType());

    m_shutdownFrame->setModel(model);
    m_userFrame->setModel(model);

    // 锁屏从"中间头像"开始，点击头像后进入登录界面；
    // 登录界面在只有一个用户时直接显示登录框，多用户时与锁屏一致先从头像开始。
    model->setCurrentModeState(startFromAvatarMode()
        ? SessionBaseModel::ModeStatus::UserMode
        : SessionBaseModel::ModeStatus::PasswordMode);

    setRightBottomWidget(m_controlWidget);

    switch (model->currentType()) {
    case SessionBaseModel::AuthType::LightdmType:
        m_sessionFrame = new SessionWidget;
        m_sessionFrame->setModel(model);
        m_controlWidget->setSessionSwitchEnable(m_sessionFrame->sessionCount() > 1);

        connect(m_sessionFrame, &SessionWidget::hideFrame, this, &LockContent::restoreMode);
        connect(m_sessionFrame, &SessionWidget::sessionChanged, this, &LockContent::restoreMode);
        connect(m_controlWidget, &ControlWidget::requestSwitchSession, this, [=] {
            m_model->setCurrentModeState(SessionBaseModel::ModeStatus::SessionMode);
        });
        connect(m_model, &SessionBaseModel::onSessionKeyChanged, m_controlWidget, &ControlWidget::chooseToSession);
        connect(m_model, &SessionBaseModel::onSessionKeyChanged, this, &LockContent::restoreMode);
        m_controlWidget->chooseToSession(m_model->sessionKey());
        break;
    case SessionBaseModel::AuthType::LockType:
        m_controlWidget->setMPRISEnable(true);
        break;
    default:
        break;
    }

    // init connect
    connect(model, &SessionBaseModel::currentUserChanged, this, &LockContent::onCurrentUserChanged);
    connect(m_userInputWidget, &UserInputWidget::requestAuthUser, this, &LockContent::requestAuthUser);
    connect(m_userFrame, &UserFrame::requestSwitchUser, this,
        [this](std::shared_ptr<User> user) {
            // 锁屏点击当前用户头像：只进入登录界面；点击其他用户仍走原有切换逻辑
            const std::shared_ptr<User> currentUser = m_model->currentUser();
            if (user && currentUser && user->uid() != currentUser->uid()) {
                emit requestSwitchToUser(user);
            }
            restoreMode();
        });
    connect(m_controlWidget, &ControlWidget::requestSwitchUser, this, [=] {
        if (m_model->currentModeState() == SessionBaseModel::ModeStatus::UserMode) return;
        m_userFrame->setFixedSize(m_userInputWidget->size());
        m_model->setCurrentModeState(SessionBaseModel::ModeStatus::UserMode);
    });
    connect(m_controlWidget, &ControlWidget::requestShutdown, this, [=] {
        m_model->setCurrentModeState(SessionBaseModel::ModeStatus::PowerMode);
    });
    connect(m_controlWidget, &ControlWidget::requestSwitchVirtualKB, this, &LockContent::toggleVirtualKB);
    connect(m_userFrame, &UserFrame::hideFrame, this, &LockContent::restoreMode);
    connect(m_shutdownFrame, &ShutdownWidget::abortOperation, this, &LockContent::restoreMode);

    connect(model, &SessionBaseModel::authFinished, this, [=] (bool success) {
        if (success) {
            m_userInputWidget->resetAllState();
        }
    });
    connect(model, &SessionBaseModel::authFaildMessage, m_userInputWidget, &UserInputWidget::setFaildMessage);
    connect(model, &SessionBaseModel::authFaildTipsMessage, m_userInputWidget, &UserInputWidget::setFaildTipMessage);
    connect(model, &SessionBaseModel::onStatusChanged, this, &LockContent::onStatusChanged);
    connect(model, &SessionBaseModel::onPowerActionChanged, this, [=] (SessionBaseModel::PowerAction poweraction) {
        switch (poweraction) {
        case SessionBaseModel::RequireNormal:
            m_userInputWidget->normalMode();
            break;
        case SessionBaseModel::RequireRestart:
            if (model->currentType() == SessionBaseModel::AuthType::LockType) {
                m_userInputWidget->restartMode();
            }
            break;
        case SessionBaseModel::RequireShutdown:
            if (model->currentType() == SessionBaseModel::AuthType::LockType) {
                m_userInputWidget->shutdownMode();
            }
        default:
            break;
        }
    });

    connect(m_userInputWidget, &UserInputWidget::abortOperation, this, [=] {
        model->setPowerAction(SessionBaseModel::PowerAction::RequireNormal);
    });

    connect(m_userInputWidget, &UserInputWidget::requestUserKBLayoutChanged, this, [=] (const QString &value) {
        emit requestSetLayout(m_user, value);
    });

    auto initVirtualKB = [this] (bool hasvirtualkb) {
        if (hasvirtualkb && !m_virtualKB) {
            m_virtualKB = new VirtualKBInstance(this);
            m_virtualKB->hide();
            m_controlWidget->setVirtualKBVisible(true);
        }
    };

    connect(model, &SessionBaseModel::hasVirtualKBChanged, this, initVirtualKB);
    connect(model, &SessionBaseModel::onUserListChanged, this, &LockContent::onUserListChanged);

    QDBusConnection::systemBus().connect(kLockSystemService, kLockSystemPath,
        kLockSystemInterface, QStringLiteral("WallpaperChanged"), this,
        SLOT(onGlobalWallpaperChanged(QString)));

    onCurrentUserChanged(model->currentUser());
    initVirtualKB(model->hasVirtualKB());

    onUserListChanged(model->userList());
}

bool LockContent::sessionSwitcherEnabled() const
{
    return m_sessionFrame && m_sessionFrame->sessionCount() > 1;
}

void LockContent::onCurrentUserChanged(std::shared_ptr<User> user)
{
    if (user.get() == nullptr) return; // if dbus is async

    // Follow the locked user's language (AccountsService), falling back to
    // the session's.
    const QString locale { user->locale().split(".").first() };
    gxde::lock::InstallTranslations(locale.isEmpty() ? QLocale::system() : QLocale(locale));

    for (auto connect : m_currentUserConnects) {
        m_user.get()->disconnect(connect);
    }

    m_currentUserConnects.clear();

    m_user = user;

    if (m_model->currentType() == SessionBaseModel::AuthType::LockType) {
        m_currentUserConnects << connect(user.get(),
            &User::lockBackgroundPathChanged, this,
            &LockContent::updateBackground, Qt::UniqueConnection);
    }

    m_userInputWidget->setUser(user);

    if (m_model->currentType() == SessionBaseModel::AuthType::LightdmType) {
        m_sessionFrame->switchToUser(user->name());
    }

    //TODO: refresh blur image
    QTimer::singleShot(0, this, [=] {
        updateBackground(currentBackgroundPath());
    });
}

void LockContent::pushUserFrame()
{
    releaseAllKeyboard();
    setCenterContent(m_userFrame);
    m_userFrame->expansion(true);

    QTimer::singleShot(300, m_userFrame, &UserFrame::grabKeyboard);
}

void LockContent::pushSessionFrame()
{
    releaseAllKeyboard();
    setCenterContent(m_sessionFrame);
    m_sessionFrame->show();

    QTimer::singleShot(300, m_sessionFrame, &SessionWidget::grabKeyboard);
}

void LockContent::pushShutdownFrame()
{
    releaseAllKeyboard();
    setCenterContent(m_shutdownFrame);

    QTimer::singleShot(300, m_shutdownFrame, &ShutdownWidget::grabKeyboard);
}

void LockContent::onStatusChanged(SessionBaseModel::ModeStatus status)
{
    // 锁屏与登录在进入密码框界面时都启用全屏背景模糊
    emit requestBackgroundFocus(
        status == SessionBaseModel::ModeStatus::PasswordMode);

    switch (status) {
    case SessionBaseModel::ModeStatus::PasswordMode:
        restoreCenterContent();
        break;
    case SessionBaseModel::ModeStatus::UserMode:
        pushUserFrame();
        break;
    case SessionBaseModel::ModeStatus::SessionMode:
        pushSessionFrame();
        break;
    case SessionBaseModel::ModeStatus::PowerMode:
        pushShutdownFrame();
        break;
    default:
        break;
    }
}

void LockContent::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_model->currentModeState() != SessionBaseModel::ModeStatus::PasswordMode) {
        m_model->setCurrentModeState(SessionBaseModel::ModeStatus::PasswordMode);
        m_model->setPowerAction(SessionBaseModel::PowerAction::RequireNormal);
    }

    // hide keyboardlayout widget
    m_userInputWidget->hideKeyboard();

    restoreCenterContent();

    return SessionBaseWindow::mouseReleaseEvent(event);
}

void LockContent::showEvent(QShowEvent *event)
{
    if (m_user) {
        updateBackground(currentBackgroundPath());
    }

    // 每次显示都回到初始模式，避免解锁后再次锁屏仍停留在登录框界面
    m_model->setCurrentModeState(startFromAvatarMode()
        ? SessionBaseModel::ModeStatus::UserMode
        : SessionBaseModel::ModeStatus::PasswordMode);

    onStatusChanged(m_model->currentModeState());

    return QFrame::showEvent(event);
}

void LockContent::hideEvent(QHideEvent *event)
{
    releaseAllKeyboard();
    if (m_virtualKB)
        m_virtualKB->hide();

    return QFrame::hideEvent(event);
}

void LockContent::resizeEvent(QResizeEvent *event)
{
    QTimer::singleShot(0, this, [=] {
        if (m_virtualKB && m_virtualKB->isVisible()) {
            updateVirtualKBPosition();
        }
    });

    return QFrame::resizeEvent(event);
}

void LockContent::releaseAllKeyboard()
{
    m_userInputWidget->releaseKeyboard();
    m_userFrame->releaseKeyboard();
    m_shutdownFrame->releaseKeyboard();

    if (m_model->currentType() == SessionBaseModel::AuthType::LightdmType) {
        m_sessionFrame->releaseKeyboard();
    }
}

void LockContent::restoreCenterContent()
{
    releaseAllKeyboard();
    setCenterContent(m_userInputWidget);

    QTimer::singleShot(300, m_userInputWidget, &UserInputWidget::grabKeyboard);
}

void LockContent::restoreMode()
{
    m_model->setCurrentModeState(SessionBaseModel::ModeStatus::PasswordMode);
}

bool LockContent::startFromAvatarMode() const
{
    // 锁屏始终从头像开始；登录界面仅在存在多个用户时才从头像开始
    return m_model->currentType() == SessionBaseModel::AuthType::LockType
        || (m_model->currentType() == SessionBaseModel::AuthType::LightdmType
            && m_model->userList().size() > 1);
}

QString LockContent::currentBackgroundPath() const
{
    if (!m_user)
        return QString();

    if (m_model->currentType() == SessionBaseModel::AuthType::LockType)
        return m_user->lockBackgroundPath();

    // 登录界面：
    // - 显式配置了全局壁纸（state.conf）→ 使用全局壁纸；
    // - 未配置且单用户 → 与该用户的锁屏壁纸保持一致；
    // - 未配置且多用户 → 使用默认登录壁纸。
    if (LockAppearance::hasConfiguredWallpaper()
        || m_model->userList().size() == 1) {
        return m_user->greeterBackgroundPath();
    }
    return LockAppearance::wallpaper();
}

void LockContent::onGlobalWallpaperChanged(const QString &wallpaper)
{
    if (m_model->currentType() == SessionBaseModel::AuthType::LightdmType) {
        // 全局壁纸被清空时（wallpaper 为空）回退到默认的登录壁纸选择逻辑
        updateBackground(wallpaper.isEmpty() ? currentBackgroundPath()
                                             : wallpaper);
    } else if (m_user) {
        updateBackground(currentBackgroundPath());
    }
}


void LockContent::updateBackground(const QString &path)
{
    // No dde-daemon ImageBlur; use the wallpaper as-is.
    // 没有dde-daemon提供的ImageBlur；按原样显示壁纸
    emit requestBackground(path);
}

void LockContent::onBlurDone(const QString &source, const QString &blur, bool status)
{
    const QString &sourcePath = QUrl(source).isLocalFile() ? QUrl(source).toLocalFile() : source;

    if (status && currentBackgroundPath() == sourcePath)
        emit requestBackground(blur);
}

void LockContent::toggleVirtualKB()
{
    if (!m_virtualKB)
        return;

    m_virtualKB->setVisible(!m_virtualKB->isVisible());
    m_virtualKB->raise();

    updateVirtualKBPosition();
}

void LockContent::updateVirtualKBPosition()
{
    if (!m_virtualKB)
        return;

    constexpr int preferredWidth = 680;
    constexpr int edgeMargin = 16;
    const int keyboardWidth = qMin(preferredWidth, qMax(280, width() - edgeMargin * 2));
    m_virtualKB->resize(keyboardWidth, m_virtualKB->height());

    const int x = qMax(0, (width() - m_virtualKB->width()) / 2);
    const int y = qMax(0, height() - m_virtualKB->height() - 50);
    m_virtualKB->move(x, y);
}

void LockContent::onUserListChanged(QList<std::shared_ptr<User> > list)
{
    const bool allowShowUserSwitchButton = m_model->allowShowUserSwitchButton();
    const bool alwaysShowUserSwitchButton = m_model->alwaysShowUserSwitchButton();

    m_controlWidget->setUserSwitchEnable(alwaysShowUserSwitchButton || (allowShowUserSwitchButton && list.size() > 1));
}
