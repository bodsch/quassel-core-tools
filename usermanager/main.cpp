
#include <cstdlib>
#include <iostream>
#include <getopt.h>
#include <sstream>
#include <string>
#include <random>
#include <vector>

#include <QString>

#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QCryptographicHash>

#include <QDebug>

#include <QuasselUser.h>


const char *progname = "quasselcore-usermanager";
const char *version = "1.0.1";
const char *copyright = "2019";
const char *email = "Bodo Schulz <bodo@boone-schulz.de>";

void print_help (void);
void print_usage (void);
QString hashPasswordSha2_512(const QString& password);
QString sha2_512(const QString& input);

enum Mode {
  list_user,
  add_user,
  delete_user,
  update_user,
  rename_user,
  validate_user
};

// ------------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {

  QString database_file = "";
  QString quassel_user = "";
  QString quassel_password = "";

  int opt = 0;
  const char* const short_opts = "hVladrvuU:P:f:";
  const option long_opts[] = {
    {"help"    , no_argument      , nullptr, 'h'},
    {"version" , no_argument      , nullptr, 'V'},

    {"list"    , no_argument      , nullptr, 'l'},
    {"add"     , no_argument      , nullptr, 'a'},
    {"delete"  , no_argument      , nullptr, 'd'},
    {"rename"  , no_argument      , nullptr, 'r'},
    {"validate", no_argument      , nullptr, 'v'},
    {"update"  , no_argument      , nullptr, 'u'},

    {"user"    , required_argument, nullptr, 'U'},
    {"password", required_argument, nullptr, 'P'},
    {"file"    , required_argument, nullptr, 'f'},
    {nullptr   , 0, nullptr, 0}
  };

  if(argc < 2)
    return 1;

  Mode mode = list_user;

  int long_index = 0;
  while((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1) {

    switch(opt) {
      case 'h':
        print_help();
        return 0;
      case 'V':
        std::cout << progname << " v" << version << std::endl;
        return 0;
      case 'l':
        mode = list_user;
        break;
      case 'a':
        mode = add_user;
        break;
      case 'd':
        mode = delete_user;
        break;
      case 'u':
        mode = update_user;
        break;
      case 'r':
        mode = rename_user;
        break;
      case 'v':
        mode = validate_user;
        break;
      case 'f':
        database_file = optarg;
        break;
      case 'U':
        quassel_user = optarg;
        break;
      case 'P':
        quassel_password = optarg;
        break;
      default:
        print_usage();

        std::cout
          << "unknown opt: "
          << opt
          << std::endl;

        return 1;
    }
  }

  /**
   * validate it
   */
  if(database_file.isEmpty()) {
    print_usage();
    std::cerr
      << "we need an database file.\n"
      << std::endl;
    return 1;
  }

  if( mode != list_user && quassel_user.isEmpty() ) {
    print_usage();
    std::cerr
      << "missing user.\n"
      << std::endl;
    return 1;
  }

  if( ( mode != list_user && mode != delete_user ) && quassel_password.isEmpty() ) {
    print_usage();
    std::cerr
      << "missing password.\n"
      << std::endl;
    return 1;
  }


  if( QFile(database_file).exists() == false ) {
    print_usage();
    std::cerr
      << "The database file " << database_file.toStdString() << " does not exist.\n"
      << std::endl;
    return 1;
  }

  /**
   *
   */

  QuasselUser qu(database_file);

  if( mode == add_user ) {

    if( qu.addUser(quassel_user, quassel_password) != 0 ) {
      std::cout
        << "user "
        << quassel_user.toStdString()
        << " successfuly added"
        << std::endl;
      return 0;
    } else {
      return 1;
    }

  } else
  if( mode == delete_user ) {

    std::cout
      << "delete user "
      << quassel_user.toStdString()
      << std::endl;

    qu.deleteUser(quassel_user);

  } else
  if( mode == update_user ) {

    if( qu.updateUser(quassel_user, quassel_password) == true ) {

      std::cout
        << "user "
        << quassel_user.toStdString()
        << " successfuly updated"
        << std::endl;
      return 0;

    } else {

      std::cout
        << "user "
        << quassel_user.toStdString()
        << " update failed"
        << std::endl;

      return 1;
    }
  } else
  if( mode == rename_user ) {

    // renameUser(const QString& username, const QString& newName)

    std::cout
      << "rename_user is currently not supported"
      << std::endl;
    return 0;
  } else
  if( mode == validate_user ) {

    if( qu.validateUser(quassel_user, quassel_password) != 0 ) {

      std::cout
        << "username and password are valid"
        << std::endl;
      return 0;
    } else {

      std::cout
        << "username and password are not valid"
        << std::endl;
      return 1;
    }
  } else {

    QMap<uint, QString> users = qu.getAllAuthUserNames();

    for(auto e : users.keys()) {
      std::cout
        << "uid: "
        << e
        << ", username: " << users.value(e).toStdString()
        << std::endl;
    }

  }

  return 0;
}

/**
 *
 */
void print_help (void) {

  std::cout
    << std::endl
    << progname << " v" << version << std::endl
    << "  Copyright (c) " << copyright << " " << email << std::endl
    << std::endl
    << "manage quassel core users" << std::endl;

  print_usage();

  std::cout
    << "Options:" << std::endl
    << " -h, --help" << std::endl
    << "    Print detailed help screen" << std::endl
    << " -V, --version" << std::endl
    << "    Print version information" << std::endl
    << " -f, --file <database file>" << std::endl
    << "    sqlite database file." << std::endl
    << " -a, --add" << std::endl
    << "    add an quassel core user (requires --user and --password)." << std::endl
    << " -d, --delete" << std::endl
    << "    delete an quassel core user (requires --user)." << std::endl
    << " -r, --rename" << std::endl
    << "    rename an quassel core user (requires --user) (NOT SUPPORTED YET!)." << std::endl
    << " -v, --validate" << std::endl
    << "    validate credentials of an quassel core user (requires --user and --password)." << std::endl
    << " -u, --update" << std::endl
    << "    set an new password of an existing quassel core user (requires --user and --password) (INSECURE, NO DOUBLE CHECK YET)" << std::endl
    << " -l, --list" << std::endl
    << "    list all quassel core users." << std::endl
    << " -U, --user <username>" << std::endl
    << "    the quassel core username." << std::endl
    << " -P, --password <password>" << std::endl
    << "    the password for the quassel core user." << std::endl
    << std::endl;
}

/**
 *
 */
void print_usage (void) {
  std::cout
    << std::endl
    << "Usage:"
    << " " << progname
    << " [--help]"
    << " [--version]"
    << " [--file]"
    << " [--user]"
    << " [--password]"
    << " [--add]"
    << " [--delete]"
    << " [--rename]"
    << " [--validate]"
    << " [--update]"
    << " [--list]"
    << std::endl;
}
