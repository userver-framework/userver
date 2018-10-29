#!/usr/bin/env python

import requests, re

err_codes_page = "https://www.postgresql.org/docs/10/static/errcodes-appendix.html"
regex = ur"(?:class=\"bold\"[\S]*>([^<]+))|(?:class=\"literal\">([^<]+).*\n.*class=\"symbol\">([^<]+))"

DEF_OFFSET = "  "
LIT_OFFSET = "  "

error_class = None
enum_def = u'''
// This goes to the header
enum class SqlState {
  kUnknownState, ///!< Unknown state, not in PostgreSQL docs
'''
enum_literals = u'''
// This goes to the cpp file in an anonimous namespace
// TODO Replace with a string_view
const std::unordered_map<std::string, SqlState> CODESTR_TO_STATE{
'''


def to_camel_case(snake_str):
	components = snake_str.split('_')
	return 'k' + ''.join(x.title() for x in components)

def print_def(*args):
	global enum_def
	enum_def += DEF_OFFSET + ("\n" + DEF_OFFSET).join(args) + "\n"

def print_lit(*args):
    global enum_literals
    enum_literals += LIT_OFFSET + ("\n" + LIT_OFFSET).join(args) + "\n"

def end_class_group():
    global error_class
    if error_class != None:
        print_def("//@}")
        print_lit("//@}")

def start_class_group(group):
    global enum_def
    global enum_literals
    global error_class
    end_class_group()
    error_class = group.replace("\n", " ")
    print_def("//@{", u"/** @name {error_class} */".format(error_class = error_class))
    print_lit("//@{", u"/** @name {error_class} */".format(error_class = error_class))

def print_symbol(symbol, literal):
    print_def(u"{symbol}, //!< {literal}".format(symbol = symbol, literal = literal))
    print_lit('{"' + literal + '", SqlState::' + symbol + '},')


page = requests.get(err_codes_page)


matches = re.finditer(regex, page.text, re.MULTILINE)


for matchNum, match in enumerate(matches):
    matchNum = matchNum + 1

    if match.group(1):
    	start_class_group(match.group(1))
    elif match.group(2):
    	literal = match.group(2)
    	symbol = to_camel_case(match.group(3))
        print_symbol(symbol, literal)

end_class_group()

enum_def += "}\n"
enum_literals += "}\n"

print enum_def
print "//------------"
print enum_literals
