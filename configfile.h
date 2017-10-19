#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include <map>
#include "configfile1.h"

typedef std::map<std::string,ValueType> SymbolTable;
extern SymbolTable get_symbol_table(const char*, const char*);

#endif
