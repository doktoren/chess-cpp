#ifndef _SETTINGS_
#define _SETTINGS_

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <list>
#include <assert.h>

#include "typedefs.hxx"

using namespace std;

// All settings should work such that the default value means "do nothing"
#ifdef NDEBUG
#define TEST(setting) false
#else
#define TEST(setting) (*setting)
#endif

struct SettingValue {
  enum ValueType { BoolValue, IntValue, StringValue };

  SettingValue(string value, string comment = "");
  ~SettingValue();

  string toString();

  ValueType value_type;
  union {
    bool bool_value;
    int int_value;
    char *string_value;
  } u;

  string comment;
};


class SettingListener;


// Warning!  This class uses pointers to elements in a map.
// It only works because FIRST the map is filled with all the elements
// (during load_from_file), and THEN the addresses are taken
// (and the map is never again modified)
class Settings {
public:
  Settings(string filename);
  ~Settings();
  void clear();

  bool load_from_file(string _filename);

  void print(ostream &os, string only_with_prefix = "");
  void save();

  bool *get_bool_setting(string name, bool _default = false);
  int *get_int_setting(string name, int _default = 0);
  char *get_string_setting(string name, char *_default = 0);

  void define(string name, string value, string comment="", bool force_new = false);

  void add_listener(SettingListener *listener);
  void remove_listener(SettingListener *listener);
private:
  void send_all();
  string filename;
  map<string, SettingValue *> values;
  list<SettingListener *> listeners;
};

class SettingListener {
public:
  SettingListener(Settings *_settings, string name) : setting_name(name) {
    //set_settings(_settings); can't be called from here
    _settings->add_listener(this);
  }
  virtual ~SettingListener() {
    settings->remove_listener(this);
  }

  virtual void set_settings(Settings *_settings) = 0;

  void define(string name, string value, string comment="") {
    settings->define(setting_name + name, value, comment);
  }
  void print(ostream &os) {
    settings->print(os, setting_name);
  }
protected:
  bool *get_bool(string name, bool _default = false) {
    assert(settings);
    return settings->get_bool_setting(setting_name + name);
  }
  int *get_int(string name, int _default = 0) {
    assert(settings);
    return settings->get_int_setting(setting_name + name);
  }
  char *get_string(string name, char *_default = 0) {
    assert(settings);
    return settings->get_string_setting(setting_name + name);
  }

  void pbool(ostream &os, string name) {
    const string TF[2] = {"false","true"};
    os << "\t" << name << " = " << TF[*(settings->get_bool_setting(setting_name + name))] << "\n";
  }
  void pint(ostream &os, string name) {
    os << "\t" << name << " = " << *(settings->get_int_setting(setting_name + name)) << "\n";
  }
  
  string setting_name;
  Settings *settings;
};

#endif
