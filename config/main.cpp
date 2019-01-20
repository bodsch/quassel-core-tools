
#include <cstdlib>
#include <iostream>
#include <getopt.h>
#include <sstream>
#include <string>
#include <vector>

#include <QString>
#include <QSettings>
#include <QByteArray>
#include <QVariant>
#include <QVariantMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

const char *progname = "quasselcore-config";
const char *version = "1.0.1";
const char *copyright = "2019";
const char *email = "Bodo Schulz <bodo@boone-schulz.de>";

void print_help (void);
void print_usage (void);

// ------------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {

  bool dump_config_file = false;
  QString config_file;

  int opt = 0;
  const char* const short_opts = "hVdf:";
  const option long_opts[] = {
    {"help"   , no_argument      , nullptr, 'h'},
    {"version", no_argument      , nullptr, 'V'},
    {"dump"   , no_argument      , nullptr, 'd'},
    {"file"   , required_argument, nullptr, 'f'},
    {nullptr  , 0, nullptr, 0}
  };

  if(argc < 2)
    return 1;

  int long_index =0;
  while((opt = getopt_long(argc, argv, short_opts, long_opts, &long_index)) != -1) {

    switch(opt) {
      case 'h':
        print_help();
        return 0;
      case 'V':
        std::cout << progname << " v" << version << std::endl;
        return 0;
      case 'd':
        dump_config_file = true;
        break;
      case 'f':
        config_file = optarg;
        break;
      default:
        print_usage();
        return 1;
    }
  }

  /**
   * validate it
   */
  if(config_file.isEmpty()) {
    print_usage();
    std::cerr
      << "we need an config file.\n"
      << std::endl;
    return 1;
  }

  if( QFile(config_file).exists() == false ) {
    print_usage();
    std::cerr
      << "The configuration file " << config_file.toStdString() << " does not exist.\n"
      << std::endl;
    return 1;
  }

  /**
   *
   */
  QSettings settings( config_file,  QSettings::IniFormat );

  if(dump_config_file) {

    //
    //  dump configuration
    //

    QVariant authSettings = settings.value("Core/AuthSettings");
    QJsonObject authJson = authSettings.toJsonObject();
    QVariant storageSettings = settings.value("Core/StorageSettings");
    QJsonObject storageJson = storageSettings.toJsonObject();

    QJsonDocument doc;
    doc.setObject(authJson);
    QString authStrJson( doc.toJson(QJsonDocument::Compact) );

    doc.setObject(storageJson);
    QString storageStrJson( doc.toJson(QJsonDocument::Compact) );

    std::cout
      << std::endl
      << "config file : " << config_file.toStdString()
      << std::endl
      << std::endl
      << "config version: " << settings.value("Config/Version").toUInt()
      << std::endl
      << "Core AuthSettings: " << authStrJson.toStdString()
      << std::endl
      << "Core StorageSettings: " << storageStrJson.toStdString()
      << std::endl
      << std::endl;

    return 0;
  }

  bool ldpa_config_valid = true;
  QByteArray config_dir         = qgetenv("QUASSEL_CONFIG_DIR");
  QByteArray ldap_base_dn       = qgetenv("LDAP_BASE_DN");
  QByteArray ldap_bind_dn       = qgetenv("LDAP_BIND_DN");
  QByteArray ldap_bind_password = qgetenv("LDAP_BIND_PASSWORD");
  QByteArray ldap_filter        = qgetenv("LDAP_FILTER");
  QByteArray ldap_hostname      = qgetenv("LDAP_HOSTNAME");
  QByteArray ldap_port          = qgetenv("LDAP_PORT");
  QByteArray ldap_uid_attribute = qgetenv("LDAP_UID_ATTR");

  std::ostringstream ss;

  // set Config Version
  if( settings.value("Config/Version").toUInt() == 0 )
    settings.setValue("Config/Version", 1);

  // ----------------

  if(ldap_bind_dn.isEmpty()) {
    ldpa_config_valid = false;
    ss
      << " - LDAP_BASE_DN missing"
      << std::endl;
  }

  if(ldap_bind_password.isEmpty()) {
    ldpa_config_valid = false;
    ss
      << " - LDAP_BIND_PASSWORD missing"
      << std::endl;
  }

  if(ldap_filter.isEmpty()) {
    ldpa_config_valid = false;
    ss
      << " - LDAP_FILTER missing"
      << std::endl;
  }

  if(ldap_hostname.isEmpty()) {
    ldpa_config_valid = false;
    ss
      << " - LDAP_HOSTNAME missing"
      << std::endl;
  }

  if(ldap_port.isEmpty()) {
    ldpa_config_valid = false;
    ss
      << " - LDAP_PORT missing"
      << std::endl;
  }

  if(ldap_uid_attribute.isEmpty()) {
    ldpa_config_valid = false;
    ss
      << " - LDAP_UID_ATTR missing"
      << std::endl;
  }

  if( ldpa_config_valid == false ) {

    std::cout
      << std::endl
      << "WARNING:"
      << std::endl
      << "The LDAP configuration was not written because not all required environment variables were set."
      << std::endl
      << ss.str()
      << std::endl;

    return 0;
  }

  QVariantMap map;
  QVariantMap map2;

  map2.insert("BaseDN"       , ldap_base_dn);
  map2.insert("BindDN"       , ldap_bind_dn);
  map2.insert("BindPassword" , ldap_bind_password);
  map2.insert("Filter"       , ldap_filter);
  map2.insert("Hostname"     , ldap_hostname);
  map2.insert("Port"         , ldap_port);
  map2.insert("UidAttribute" , ldap_uid_attribute);

  map.insert("Authenticator", "LDAP");
  map.insert("AuthProperties", map2);

  QJsonObject json = QJsonObject::fromVariantMap(map);
  settings.setValue("Core/AuthSettings", json.toVariantMap());

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
    << "write an quassel core config file" << std::endl
    << "read following environment variablen to  write an Ldap configuration:" << std::endl
    << "  - LDAP_BASE_DN, " << std::endl
    << "  - LDAP_BIND_DN " << std::endl
    << "  - LDAP_BIND_PASSWORD" << std::endl
    << "  - LDAP_FILTER, " << std::endl
    << "  - LDAP_HOSTNAME " << std::endl
    << "  - LDAP_PORT" << std::endl
    << "  - LDAP_UID_ATTR" << std::endl;

  print_usage();

  std::cout
    << "Options:" << std::endl
    << " -h, --help" << std::endl
    << "    Print detailed help screen" << std::endl
    << " -V, --version" << std::endl
    << "    Print version information" << std::endl
    << " -f, --file" << std::endl
    << "    config file." << std::endl
    << " -d, --dump" << std::endl
    << "    dump content config file" << std::endl;
}

/**
 *
 */
void print_usage (void) {
  std::cout << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << " " << progname << " [-file <config file>] --dump"  << std::endl;
  std::cout << std::endl;
}
