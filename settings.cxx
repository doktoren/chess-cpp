#include "settings.hxx"

#include <stdio.h>
#include <fstream>

#include "help_functions.hxx"

int string_count = 0;
char *new_string(int allocated_size = 256) {
  ++string_count;
#ifndef NDEBUG
  if (string_count > 1000) {
    cerr << "ERROR: (Assuming) memory leak in settings.cxx\n";
    assert(0);
  }
#endif
  return (char *)malloc(allocated_size);
}


SettingValue::SettingValue(string value, string comment) : comment(comment) {
  if (value=="true"  ||  value=="false") {
    value_type = BoolValue;
    u.bool_value = value=="true";
  } else if ('-'==value[0]  ||  ('0'<=value[0]  &&  value[0]<='9')) {
    value_type = IntValue;
    sscanf(value.c_str(), "%d", &u.int_value);

  } else if (strstr(value.c_str(), "true")) {
    value_type = BoolValue;
    u.bool_value = true;
  } else if (strstr(value.c_str(), "false")) {
    value_type = BoolValue;
    u.bool_value = false;
    
  } else if (value[0]=='\"') {
    value_type = StringValue;
    u.string_value = new_string();
    memcpy(u.string_value, &(value[1]), value.size()-2);
    u.string_value[value.size()-2] = 0;

  } else {
    cerr << "\nSettingValue " << value << " not recognized!\n";
  }
}

SettingValue::~SettingValue() {
  if (value_type == StringValue) {
    delete u.string_value;
    --string_count;
  }
}

string SettingValue::toString() {
  switch (value_type) {
  case BoolValue:
    return u.bool_value ? "true" : "false";
  case IntValue:
    {
      char tmp[12];
      sprintf(tmp, "%d", u.int_value);
      return string(tmp);
    }
  case StringValue:
    { // add a " character on each side
      string result(2+strlen(u.string_value), '\"');
      memcpy(&(result[1]), u.string_value, result.size()-2);
      return result;
    }
  default:
    cerr << "\nSettingValue::toString - unknown value_type\n";
    return "error";
  }
}

//################################################


Settings::Settings(string filename) {
  load_from_file(filename);
}

bool Settings::load_from_file(string _filename)
{
  filename = _filename;
  ifstream in(filename.c_str());
  if (!in) return false;

  cerr << "Loading settings from " << filename << '\n';
  clear();
  values = map<string, SettingValue *>();
  string comment="";
  char line[128];
  while (in.getline(line, 128, '\n')) {
    // cerr << "Reading line: " << line << '\n';
    if (*line=='#'  ||  *line=='%'  ||  *line=='['  ||  *line=='/') {
      // Just a comment line
      comment = string(line);
      continue;
    }
    if (strlen(line)<3) {
      // Empty line
      comment == "";
      continue;
    }
    char tag[64], value[64];
    if (sscanf(line, "%s = %s", tag, value) == 2) {
      define(tag, value, comment, true);
      comment = "";
    }
  }
  send_all();
  return true;
}

Settings::~Settings() {
  clear();
}

void Settings::clear() {
  for(map<string, SettingValue *>::const_iterator i=values.begin(); i!=values.end(); i++)
    delete i->second;
}

void Settings::print(ostream &os, string only_with_prefix) {
  for(map<string, SettingValue *>::const_iterator i=values.begin(); i!=values.end(); i++)
    if (only_with_prefix.size()==0) {
      os << i->first << " = " << i->second->toString() << '\n';
    } else if (i->first.size() >= only_with_prefix.size()  &&
	       memcmp(i->first.c_str(), only_with_prefix.c_str(), only_with_prefix.size()) == 0) {
      os << &(i->first.c_str()[only_with_prefix.size()]) << " = "
	 << i->second->toString() << '\n';
    }
}

void Settings::save() {
  ofstream out(filename.c_str());
  bool first_line = true;
  for(map<string, SettingValue *>::const_iterator i=values.begin(); i!=values.end(); i++) {
    if (!first_line) out << '\n';
    if (i->second->comment != "") out << i->second->comment << '\n';
    out << i->first << " = " << i->second->toString() << '\n';
    first_line = false;
  }
  cerr << "Settings saved.\n";
}

bool *Settings::get_bool_setting(string name, bool _default) {
  if (values.count(name)) return &(values[name]->u.bool_value);
  cerr << "\nWarning: setting " << name << " not found. Returning " << BOOL_STR(_default) << ".\n";
  values[name] = new SettingValue(BOOL_STR(_default), "// default value set");
  return &(values[name]->u.bool_value);
}

int *Settings::get_int_setting(string name, int _default) {
  if (values.count(name)) return &(values[name]->u.int_value);
  cerr << "\nWarning: setting " << name << " not found. Returning " << _default << ".\n";
  values[name] = new SettingValue(toString(_default), "// default value set");
  return &(values[name]->u.int_value);
}


char *Settings::get_string_setting(string name, char *_default) {
  if (values.count(name)) return values[name]->u.string_value;
  cerr << "\nWarning: setting " << name << " not found. Returning "
       << (_default ? _default : "\"\"") << ".\n";
  values[name] = new SettingValue(_default, "// default value set");
  return values[name]->u.string_value;
}

void Settings::define(string name, string value, string comment, bool force_new) {
  // cerr << "define(" << name << ',' << value << ',' << comment << ")\n";
  if (values.count(name)) {
    *values[name] = SettingValue(value, comment);
  } else {
    if (force_new) {
      values[name] = new SettingValue(value, comment);
    } else {
      cerr << "Unknown setting named " << name << "\n";
    }
  }
  send_all();
}



void Settings::add_listener(SettingListener *listener) {
  listeners.push_back(listener);
}
void Settings::remove_listener(SettingListener *listener) {
  listeners.remove(listener);
}
void Settings::send_all() {
  typedef list<SettingListener *>::iterator CI;
  for (CI i = listeners.begin(); i!=listeners.end(); i++)
    (*i)->set_settings(this);
}
