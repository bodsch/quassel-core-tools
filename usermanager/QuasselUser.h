/***************************************************************************
 *   Copyright (C) 2019 by Bodo Schulz                                     *
 *   bodo@boone-schulz.de                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <iostream>

#include <QSqlDatabase>
#include <QVariantList>
#include <QtSql>

class QuasselUser {

public:
    QuasselUser(const QString& database_file);

    bool isAvailable() const ;
    QString backendId() const ;
    QString displayName() const ;
    QVariantList setupData() const  { return {}; }
    QString description() const ;

    // TODO: Add functions for configuring the backlog handling, i.e. defining auto-cleanup settings etc

    /* User handling */
    uint addUser(const QString& user, const QString& password, const QString& authenticator = "Database") ;

    bool updateUser(uint user, const QString& password) ;
    bool updateUser(const QString& username, const QString& password);

    void renameUser(uint user, const QString& newName) ;
    void renameUser(const QString& username, const QString& newName) ;

    uint validateUser(const QString& user, const QString& password) ;

    uint getUserId(const QString& username) ;

    void deleteUser(uint user);
    void deleteUser(const QString& user);

    QString getUserAuthenticator(uint userid);

    // Sysident handling
    QMap<uint, QString> getAllAuthUserNames() ;

protected:

    QSqlDatabase logDb();
    void dbConnect(QSqlDatabase& db);
    bool initDbSession(QSqlDatabase& /* db */) { return true; }

    bool checkHashedPassword(const QString& password, const QString& hashedPassword);

private:

    QString database_file;
    QString hashPasswordSha2_512(const QString& password);
    QString sha2_512(const QString& input);

    enum HashVersion {
      Sha1,
      Sha2_512,
      Latest = Sha2_512
    };
};
