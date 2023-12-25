#include <string>

std::string wlp4string = R"(
.STATES
start
ID!
ZERO!
NUM!
LPAREN!
RPAREN!
LBRACE!
RBRACE!
LBRACK!
RBRACK!
BECOMES!
PLUS!
MINUS!
STAR!
SLASH!
PCT!
AMP!
COMMA!
SEMI!
LT!
GT!
LE!
GE!
EQ!
excl
NE!
?WHITESPACE!
?COMMENT!
.TRANSITIONS
start a-z A-Z ID
ID a-z A-Z 0-9 ID
start 0 ZERO
ZERO 0-9 NUM
start 1-9 NUM
NUM 0-9 NUM
start ( LPAREN
start ) RPAREN
start { LBRACE
start } RBRACE
start [ LBRACK
start ] RBRACK
start = BECOMES
start + PLUS
start - MINUS
start * STAR
start / SLASH
start % PCT
start & AMP
start , COMMA
start ; SEMI
start < LT
start > GT
LT = LE
GT = GE
BECOMES = EQ
start ! excl
excl = NE
start \s \t \r \n ?WHITESPACE
?WHITESPACE \s \t \r \n ?WHITESPACE
SLASH / ?COMMENT
?COMMENT \x00-\x09 \x0B \x0C \x0E-\x7F ?COMMENT
)";