/**
 * g_lang.l
 * Copyright (C) Tony Givargis, 2019-2020
 *
 * This file is part of The Gravity Compiler.
 *
 * The Gravity Compiler is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version. The Gravity Compiler is distributed in
 * the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details. You should
 * have received a copy of the GNU General Public License along with Foobar.
 * If not, see <https://www.gnu.org/licenses/>.
 */

%{
#define YY_NO_LEAKS
#include "y.tab.h"
#include "g_ir.h"
%}

%option never-interactive
%option nodefault
%option noyywrap
%option yylineno
%option nounput
%option noinput

%%
".module"                        { return G__MODULE;                    }
".prefix"                        { return G__PREFIX;                    }
".optimizer"                     { return G__OPTIMIZER;                 }
".precision"                     { return G__PRECISION;                 }
".costfnc"                       { return G__COSTFNC;                   }
".batch"                         { return G__BATCH;                     }
".input"                         { return G__INPUT;                     }
".output"                        { return G__OUTPUT;                    }
".hidden"                        { return G__HIDDEN;                    }
"sgd"                            { return G__SGD;                       }
"float"                          { return G__FLOAT;                     }
"double"                         { return G__DOUBLE;                    }
"fixed"                          { return G__FIXED;                     }
"quadratic"                      { return G__QUADRATIC;                 }
"exponential"                    { return G__EXPONENTIAL;               }
"cross_entropy"                  { return G__CROSS_ENTROPY;             }
"relu"                           { return G__RELU;                      }
"linear"                         { return G__LINEAR;                    }
"softmax"                        { return G__SOFTMAX;                   }
"sigmoid"                        { return G__SIGMOID;                   }
","                              { return ',';                          }
";"                              { return ';';                          }
"["                              { return '[';                          }
"]"                              { return ']';                          }
"("                              { return '(';                          }
")"                              { return ')';                          }
"+"                              { return '+';                          }
"-"                              { return '-';                          }
"*"                              { return '*';                          }
"/"                              { return '/';                          }
"%"                              { return '%';                          }
-?[0-9]+                         { yylval.s = yytext; return G__LONG;   }
-?[0-9]+"."[0-9]* |
-?"."[0-9]+ |
-?[0-9]+E[-+]?[0-9]+ |
-?[0-9]+"."[0-9]*E[-+]?[0-9]+ |
-?"."[0-9]*E[-+]?[0-9]+          { yylval.s = yytext; return G__REAL;   }
'(\\.|''|[^'\n])*' |
\"(\\.|\"\"|[^"\n])*\"           { yylval.s = yytext; return G__STRING; }
[ \t\n]                          ;
\/\/.*\n                         ;
.                                { return G__ERROR;                     }
%%

int yyerroron;

void
yyerror(const char *format, ...)
{
        va_list va;

        if (yyerroron) {
                fprintf(stderr, "error: ");
                va_start(va, format);
                vfprintf(stderr, format ? format : "syntax error", va);
                va_end(va);
                fprintf(stderr, " near line %d\n", yylineno);
        }
}
