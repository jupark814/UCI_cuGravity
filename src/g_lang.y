/**
 * g_lang.y
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
  #include "g_ir.h"
%}

%union {
  long l;
  double d;
  char *s;
  struct t { long a, b, c; } t;
}

%left '+' '-'
%left '*' '/' '%'
%right N

%token G__MODULE
%token G__PREFIX
%token G__OPTIMIZER
%token G__PRECISION
%token G__COSTFNC
%token G__BATCH
%token G__INPUT
%token G__OUTPUT
%token G__HIDDEN
%token G__CUDA
%token G__SGD
%token G__FLOAT
%token G__DOUBLE
%token G__FIXED
%token G__QUADRATIC
%token G__EXPONENTIAL
%token G__CROSS_ENTROPY
%token G__RELU
%token G__LINEAR
%token G__SOFTMAX
%token G__SIGMOID
%token <s> G__LONG
%token <s> G__REAL
%token <s> G__STRING
%token G__ERROR

%type <t> _precision1_
%type <l> _costfnc1_
%type <l> _activation_
%type <l> _expr_
%type <l> _long_
%type <d> _real_
%type <s> _string_

%%

top
  : _phrase_ { if (g__ir_top()) YYABORT; }
  | G__ERROR { yyerror(0); YYABORT;      }
  ;

_phrase_
  : _phrase1_
  | _phrase1_ _phrase_
  ;

_phrase1_
  : _module_ ';'
  | _prefix_ ';'
  | _optimizer_ ';'
  | _precision_ ';'
  | _costfnc_ ';'
  | _batch_ ';'
  | _input_ ';'
  | _output_ ';'
  | _hidden_ ';'
  | _cuda_ ';'
  | ';'
  ;

/*---------------------------------------------------------------------------------------------------------------------------------------*/

_module_
  : G__MODULE _string_ { if (g__ir_module($2)) YYABORT; }
  ;

_prefix_
  : G__PREFIX _string_ { if (g__ir_prefix($2)) YYABORT; }
  ;

_optimizer_
  : G__OPTIMIZER G__SGD _real_ { if (g__ir_optimizer(G__IR_OPTIMIZER_SGD, $3)) YYABORT; }
  ;

_precision_
  : G__PRECISION _precision1_ { if (g__ir_precision($2.a, $2.b, $2.c)) YYABORT; }
  ;

_precision1_
  : G__FLOAT                           { struct t t = {  0,  0, G__IR_PRECISION_FLOAT  }; $$ = t; }
  | G__DOUBLE                          { struct t t = {  0,  0, G__IR_PRECISION_DOUBLE }; $$ = t; }
  | G__FIXED '[' _expr_ ',' _expr_ ']' { struct t t = { $3, $5, G__IR_PRECISION_FIXED  }; $$ = t; }
  ;

_costfnc_
  : G__COSTFNC _costfnc1_ { if (g__ir_costfnc($2)) YYABORT; }
  ;

_costfnc1_
  : G__QUADRATIC     { $$ = G__IR_COSTFNC_QUADRATIC;     }
  | G__EXPONENTIAL   { $$ = G__IR_COSTFNC_EXPONENTIAL;   }
  | G__CROSS_ENTROPY { $$ = G__IR_COSTFNC_CROSS_ENTROPY; }
  ;

_batch_
  : G__BATCH _expr_ { if (g__ir_batch($2)) YYABORT; }
  ;

_input_
  : G__INPUT _expr_ { if (g__ir_input($2)) YYABORT; }
  ;

_output_
  : G__OUTPUT _expr_ _activation_ { if (g__ir_output($2, $3)) YYABORT; }
  ;

_hidden_
  : G__HIDDEN _expr_ _activation_ { if (g__ir_hidden($2, $3)) YYABORT; }
  ;

_cuda_
  : G__CUDA _expr_ { if (g__ir_cuda($2)) YYABORT; }
  ;

_activation_
  : G__RELU    { $$ = G__IR_ACTIVATION_RELU;    }
  | G__LINEAR  { $$ = G__IR_ACTIVATION_LINEAR;  }
  | G__SOFTMAX { $$ = G__IR_ACTIVATION_SOFTMAX; }
  | G__SIGMOID { $$ = G__IR_ACTIVATION_SIGMOID; }
  ;

/*---------------------------------------------------------------------------------------------------------------------------------------*/

_expr_
  : _long_             { $$ = $1;      }
  | '(' _expr_ ')'     { $$ = $2;      }
  | '-' _expr_ %prec N { $$ = -$2;     }
  | _expr_ '+' _expr_  { $$ = $1 + $3; }
  | _expr_ '-' _expr_  { $$ = $1 - $3; }
  | _expr_ '*' _expr_  { $$ = $1 * $3; }
  | _expr_ '/' _expr_  { if(!$3) { yyerror("divide by zero near line %d", yylineno); G__DEBUG(G__ERR_SYNTAX); YYABORT; } $$ = $1 / $3; }
  | _expr_ '%' _expr_  { if(!$3) { yyerror("divide by zero near line %d", yylineno); G__DEBUG(G__ERR_SYNTAX); YYABORT; } $$ = $1 % $3; }
  ;

_long_
  : G__LONG
  {
    char *e = 0;
    $$ = strtol($1, &e, 10);
    if ((e == $1) || (*e)) {
      yyerror("invalid numeric value '%s' near line %d", $1, yylineno);
      G__DEBUG(G__ERR_SYNTAX);
      YYABORT;
    }
  }
  ;

_real_
  : G__REAL
  {
    char *e = 0;
    $$ = strtod($1, &e);
    if ((e == $1) || (*e)) {
      yyerror("invalid numeric value '%s' near line %d", $1, yylineno);
      G__DEBUG(G__ERR_SYNTAX);
      YYABORT;
    }
  }
  ;

_string_
  : G__STRING
  {
    if (!($$ = g__ir_strdup($1 + 1))) {
      yyerror("out of memory");
      G__DEBUG(0);
      YYABORT;
    }
    $$[g__strlen($$)-1] = 0;
  }
  ;

%%
