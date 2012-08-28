#ifndef _PARSER_
#define _PARSER_

#include <string>
#include <vector>
#include <iostream>

using namespace std;

class Board;

// CLR(self, Board (Engine), output, command line argument list)
typedef bool CommandLineReceiver(Board *, ostream&, vector<string>&);

vector<string> parse(string s);

string short_version(string s);

bool dot_demand(vector<string> parsed, unsigned int length, ...);

#endif
