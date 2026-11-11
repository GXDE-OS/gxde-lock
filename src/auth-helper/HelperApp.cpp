/*
 * Main authentication application class
 * Copyright (C) 2013 Martin Bříza <mbriza@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "HelperApp.h"
#include "Backend.h"
#include "SafeDataStream.h"

#include "MessageHandler.h"

#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <QtNetwork/QLocalSocket>

namespace SDDM {
    HelperApp::HelperApp(int& argc, char** argv)
            : QCoreApplication(argc, argv)
            , m_backend(Backend::get(this))
            , m_socket(new QLocalSocket(this)) {
        qInstallMessageHandler(HelperMessageHandler);
        QTimer::singleShot(0, this, SLOT(setUp()));
    }

    void HelperApp::setUp() {
        const QStringList args = QCoreApplication::arguments();
        QString server;
        int pos;

        if ((pos = args.indexOf(QStringLiteral("--socket"))) >= 0) {
            if (pos >= args.length() - 1) {
                qCritical() << "This application is not supposed to be executed manually";
                exit(Auth::HELPER_OTHER_ERROR);
                return;
            }
            server = args[pos + 1];
        }

        if ((pos = args.indexOf(QStringLiteral("--id"))) >= 0) {
            if (pos >= args.length() - 1) {
                qCritical() << "This application is not supposed to be executed manually";
                exit(Auth::HELPER_OTHER_ERROR);
                return;
            }
            m_id = QString(args[pos + 1]).toLongLong();
        }

        if ((pos = args.indexOf(QStringLiteral("--user"))) >= 0) {
            if (pos >= args.length() - 1) {
                qCritical() << "This application is not supposed to be executed manually";
                exit(Auth::HELPER_OTHER_ERROR);
                return;
            }
            m_user = args[pos + 1];
        }

        if ((pos = args.indexOf(QStringLiteral("--fingerprint"))) >= 0) {
            m_backend->setFingerprint(true);
        }

        // gxde-lock only verifies the user of a running session. Opening a
        // session is the display manager's job, so this helper refuses to.
        if (args.contains(QStringLiteral("--start"))
                || args.contains(QStringLiteral("--display-server"))) {
            qCritical() << "This helper only authenticates; it cannot start sessions";
            exit(Auth::HELPER_OTHER_ERROR);
            return;
        }

        if (server.isEmpty() || m_id <= 0) {
            qCritical() << "This application is not supposed to be executed manually";
            exit(Auth::HELPER_OTHER_ERROR);
            return;
        }

        connect(m_socket, &QLocalSocket::connected, this, &HelperApp::doAuth);
        m_socket->connectToServer(server, QIODevice::ReadWrite | QIODevice::Unbuffered);
    }

    void HelperApp::doAuth() {
        SafeDataStream str(m_socket);
        str << Msg::HELLO << m_id;
        str.send();
        if (str.status() != QDataStream::Ok)
            qCritical() << "Couldn't write initial message:" << str.status();

        // Runs unprivileged: pam_unix delegates to the setuid unix_chkpwd.
        if (!m_backend->start(m_user) || !m_backend->authenticate()) {
            authenticated(QString());
            exit(Auth::HELPER_AUTH_ERROR);
            return;
        }

        m_user = m_backend->userName();
        authenticated(m_user);
        exit(Auth::HELPER_SUCCESS);
    }

    void HelperApp::info(const QString& message, Auth::Info type) {
        SafeDataStream str(m_socket);
        str << Msg::INFO << message << type;
        str.send();
        m_socket->waitForBytesWritten();
    }

    void HelperApp::error(const QString& message, Auth::Error type) {
        SafeDataStream str(m_socket);
        str << Msg::ERROR << message << type;
        str.send();
        m_socket->waitForBytesWritten();
    }

    Request HelperApp::request(const Request& request) {
        Msg m = Msg::MSG_UNKNOWN;
        Request response;
        SafeDataStream str(m_socket);
        str << Msg::REQUEST << request;
        str.send();
        str.receive();
        str >> m >> response;
        if (m != REQUEST) {
            response = Request();
            qCritical() << "Received a wrong opcode instead of REQUEST:" << m;
        }
        return response;
    }

    void HelperApp::authenticated(const QString &user) {
        Msg m = Msg::MSG_UNKNOWN;
        QProcessEnvironment env;
        QByteArray cookie;
        SafeDataStream str(m_socket);
        str << Msg::AUTHENTICATED << user;
        str.send();
        if (user.isEmpty())
            return;
        // The client still answers with an environment and a cookie; an
        // authentication-only helper has no use for either.
        str.receive();
        str >> m >> env >> cookie;
        if (m != AUTHENTICATED)
            qCritical() << "Received a wrong opcode instead of AUTHENTICATED:" << m;
    }

    const QString& HelperApp::user() const {
        return m_user;
    }
}

int main(int argc, char** argv) {
    SDDM::HelperApp app(argc, argv);
    return app.exec();
}
