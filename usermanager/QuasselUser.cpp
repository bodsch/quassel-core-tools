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

#include "QuasselUser.h"

QuasselUser::QuasselUser(const QString& file){

  database_file = file;
}

bool QuasselUser::isAvailable() const {

  if (!QSqlDatabase::isDriverAvailable("QSQLITE"))
    return false;
  return true;
}

QString QuasselUser::backendId() const {
  return QString("SQLite");
}

QString QuasselUser::displayName() const {
  // Note: Pre-0.13 clients use the displayName property for backend idenfication
  // We identify the backend to use for the monolithic core by its displayname.
  // so only change this string if you _really_ have to and make sure the core
  // setup for the mono client still works ;)
  return backendId();
}

QString QuasselUser::description() const {
  return ("SQLite is a file-based database engine that does not require any setup. It is suitable for small and medium-sized "
            "databases that do not require access via network. Use SQLite if your Quassel Core should store its data on the same machine "
            "it is running on, and if you only expect a few users to use your core.");
}

QSqlDatabase QuasselUser::logDb() {

  QSqlDatabase db;

  if(QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
    db = QSqlDatabase::database();
  } else {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(database_file);
  }

  if( !db.isOpen() ) {
    dbConnect(db);
  }

  return db;
}

void QuasselUser::dbConnect(QSqlDatabase& db) {

  if( !db.open() ) {
    std::cerr
      << std::endl
      << "ERROR: "
      << "Unable to open database" << displayName().toStdString()
      << "file "
      << database_file.toStdString()
      << std::endl
      << "-"
      << db.lastError().text().toStdString();
  }
}

uint QuasselUser::addUser(const QString& user, const QString& password, const QString& authenticator) {

  QSqlDatabase db = logDb();
  uint uid = 0;

  db.transaction();

  QSqlQuery query(db);
  query.prepare("INSERT INTO quasseluser (username, password, hashversion, authenticator) VALUES (:username, :password, :hashversion, :authenticator)");
  query.bindValue(":username", user);
  query.bindValue(":password", hashPasswordSha2_512(password));
  query.bindValue(":hashversion", HashVersion::Latest);
  query.bindValue(":authenticator", authenticator);
  query.exec();

  // user already exists - sadly 19 seems to be the general constraint violation error...
  // QSqlError("19", "Unable to fetch row", "UNIQUE constraint failed: quasseluser.username")
  if( query.lastError().isValid() && query.lastError().nativeErrorCode().toInt() == 19 ) {
    std::cerr
      << std::endl
      << "ERROR: "
      << "The User "
      << user.toStdString()
      << " already exists"
      << std::endl;

    db.rollback();
  }
  else {
    uid = query.lastInsertId().toInt();
    db.commit();
  }

  return uid;
}

bool QuasselUser::updateUser(uint user, const QString& password) {

  QSqlDatabase db = logDb();
  bool success = false;

  db.transaction();

  QSqlQuery query(db);
  query.prepare("UPDATE quasseluser SET password = :password, hashversion = :hashversion WHERE userid = :userid");
  query.bindValue(":userid", user);
  query.bindValue(":password", hashPasswordSha2_512(password));
  query.bindValue(":hashversion", HashVersion::Latest);

  query.exec();

  success = query.numRowsAffected() != 0;

  db.commit();

  return success;
}

bool QuasselUser::updateUser(const QString& username, const QString& password) {

  uint user_id = getUserId(username);

  if( user_id != 0 )
    return updateUser(user_id, password);
  else
    return false;
}

void QuasselUser::renameUser(uint user, const QString& newName) {

  QSqlDatabase db = logDb();
  db.transaction();

  QSqlQuery query(db);
  query.prepare("UPDATE quasseluser SET username = :username WHERE userid = :userid");
  query.bindValue(":userid", user);
  query.bindValue(":username", newName);

  query.exec();

  db.commit();
}

void QuasselUser::renameUser(const QString& username, const QString& newName) {

  uint user_id = getUserId(username);

  if( user_id != 0 )
    renameUser(user_id, newName);
}

uint QuasselUser::validateUser(const QString& user, const QString& password) {

  uint userId = 0;
  QString hashedPassword;

  QSqlQuery query(logDb());
  query.prepare("SELECT userid, password, hashversion, authenticator FROM quasseluser WHERE username = :username");
  query.bindValue(":username", user);
  query.exec();

  if( query.first() ) {
    userId = query.value("userid").toInt();
    hashedPassword = query.value("password").toString();
  }

  uint returnUserId = 0;
  if( userId != 0 && checkHashedPassword(password, hashedPassword) ) {
    returnUserId = userId;
  }
  return returnUserId;
}

uint QuasselUser::getUserId(const QString& username) {

  uint userId = 0;

  QSqlQuery query(logDb());

  query.prepare("SELECT userid FROM quasseluser WHERE username = :username");
  query.bindValue(":username", username);
  query.exec();

  if(query.first()) {
    userId = query.value("userid").toInt();
  }
  return userId;
}

QString QuasselUser::getUserAuthenticator(uint userid) {

  QString authenticator = QString("");

  QSqlQuery query(logDb());
  query.prepare("SELECT authenticator FROM quasseluser WHERE userid = :userid");
  query.bindValue(":userid", userid);
  query.exec();

  if( query.first() ) {
    authenticator = query.value("authenticator").toString();
  }

  return authenticator;
}

void QuasselUser::deleteUser(uint user) {

  QSqlDatabase db = logDb();
  db.transaction();

  QSqlQuery query(db);
  query.prepare("DELETE FROM backlog WHERE bufferid IN (SELECT DISTINCT bufferid FROM buffer WHERE userid = :userid)");
  query.bindValue(":userid", user);
  query.exec();

  query.prepare("DELETE FROM buffer WHERE userid = :userid");
  query.bindValue(":userid", user);
  query.exec();

  query.prepare("DELETE FROM network WHERE userid = :userid");
  query.bindValue(":userid", user);
  query.exec();

  query.prepare("DELETE FROM quasseluser WHERE userid = :userid");
  query.bindValue(":userid", user);
  query.exec();

  // I hate the lack of foreign keys and on delete cascade... :(
  db.commit();
}

void QuasselUser::deleteUser(const QString& username) {

  uint user_id = getUserId(username);

  if( user_id != 0 )
    deleteUser(user_id);
}

QMap<uint, QString> QuasselUser::getAllAuthUserNames() {

  QMap<uint, QString> authusernames;

  QSqlDatabase db = logDb();
  db.transaction();

  QSqlQuery query(db);
  query.prepare("SELECT userid, username FROM quasseluser;");
  query.exec();

  while( query.next() ) {
    authusernames[query.value("userid").toInt()] = query.value("username").toString();
  }

  db.commit();

  return authusernames;
}

bool QuasselUser::checkHashedPassword(const QString& password, const QString& hashedPassword) {

  QRegExp colonSplitter("\\:");
  QStringList hashedPasswordAndSalt = hashedPassword.split(colonSplitter);

  if( hashedPasswordAndSalt.size() == 2 ) {
    return sha2_512(password + hashedPasswordAndSalt[1]) == hashedPasswordAndSalt[0];
  }
  else {
    qWarning() << "Password hash and salt were not in the correct format";
    return false;
  }
}

QString QuasselUser::hashPasswordSha2_512(const QString& password) {

    // Generate a salt of 512 bits (64 bytes) using the Mersenne Twister
    std::random_device seed;
    std::mt19937 generator(seed());
    std::uniform_int_distribution<int> distribution(0, 255);
    QByteArray saltBytes;
    saltBytes.resize(64);

    for (int i = 0; i < 64; i++) {
      saltBytes[i] = (unsigned char)distribution(generator);
    }
    QString salt(saltBytes.toHex());

    // Append the salt to the password, hash the result, and append the salt value
    return sha2_512(password + salt) + ":" + salt;
}

QString QuasselUser::sha2_512(const QString& input) {
  return QString(QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Sha512).toHex());
}
