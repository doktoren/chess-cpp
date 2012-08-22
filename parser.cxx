#include <stdarg.h>

#include "parser.hxx"

vector<string> parse_result;

vector<string> parse(string s) {
  vector<string> result;
  unsigned int next_i, i=0;
  while ((next_i = s.find(' ', i)) != string::npos) {
    if (next_i - i > 0) {
      string tmp = s.substr(i, next_i - i);
      // cerr << "####### " << tmp << " ##########\n";
      result.push_back(tmp);
    }
    i = next_i+1;// jump over ' '
  }
  // cerr << s << "-" << i << " " << s.length()-1 << "\n";
  if (i < s.length()) result.push_back(s.substr(i));
  return result;
}

string short_version(string s) {
  char result[16];
  unsigned int ir=0, is=0;
  if (s[is] == '-') result[(is++,ir++)] = '-';
  while (is < s.length()) {
    if (s[is] != ' ') {
      result[ir++] = s[is];
      while (++is < s.length()  &&  s[is] != ' ') ;
    } else {
      is++;
    }
  }
  result[ir] = 0;
  return result;
}

// Very ugly code, but whatever
bool dot_demand(vector<string> parsed, unsigned int length, ...) {
  if (parsed.size() > length) return false;

  va_list ap;
  // First assume not a shortened version
  if (parsed.size() != length) goto try_shortened_version;
  va_start(ap, length);
  for (unsigned int i=0; i<length; i++) {
    uint arg = va_arg(ap, int);
    if (arg < 0x100) {
      if (arg) {
	// Interpret arg as a demand on length of blah
	if (parsed[i].length() != arg) goto try_shortened_version;
      } else {
	// No demands!
      }
    } else {
      // Compare with char*
      char* s = (char *)arg;
      if (strcmp(parsed[i].c_str(), s)) goto try_shortened_version;
    }
  }
  // Succes
  parse_result = vector<string>();
  va_start(ap, length);
  for (unsigned int i=0; i<length; i++) {
    uint arg = va_arg(ap, int);
    if (arg < 0x100) {
      // Interpret arg as a demand on length of blah
      parse_result.push_back(parsed[i]);
    }
  }
  return true;
  

 try_shortened_version:
  va_start(ap, length);
  unsigned int num = 0;
  for (unsigned int i=0; i<length; i++) {
    uint arg = va_arg(ap, int);
    if (arg < 0x100) break;
    ++num;
  }
  if (parsed[0].size() != num) { /*cerr << "Her1\n";*/ return false; }
  if (parsed.size() != length - num + 1) { /*cerr << "Her2\n";*/ return false; }

  va_start(ap, length);
  for (unsigned int i=0; i<num; i++) {
    uint arg = va_arg(ap, int);
    char* s = (char *)arg;
    if (parsed[0][i] != s[0]) { /*cerr << "Her3 " << num << "\n";*/ return false; }
  }
  for (unsigned int i=num; i<length; i++) {
    uint arg = va_arg(ap, int);
    if (arg >= 0x100) { /*cerr << "Her4\n";*/ return false; }
    if (arg) {
      // Interpret arg as a demand on length of blah
      if (parsed[i-num+1].length() != arg) { /*cerr << "Her5\n";*/ return false; }
    } else {
      // No demands!
    }
  }

  // Succes
  parse_result = vector<string>();
  va_start(ap, length);
  for (unsigned int i=0; i<length; i++) {
    uint arg = va_arg(ap, int);
    if (arg < 0x100) {
      // Interpret arg as a demand on length of blah
      parse_result.push_back(parsed[i-num+1]);
    }
  }

  //  cerr << "<("; for (unsigned int i=0; i<parse_result.size(); i++) cerr << parse_result[i] << " "; cerr << ")>\n";

  return true;
}
