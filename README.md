# The Gravity Compiler

The Gravity Compiler is an Artificial Neural Network to ANSI C optimizing
compiler intended for embedded applications. Specifically, the generated
ANSI C file is a self contained, dependency free, C file that can be
compiled to a target platform/architecture.

The Gravity compiler has two modes of operation:
  * It can be used as a traditional compiler
  * It can be used as a just-in-time compiler

# Obtaining & Building Gravity
  1. $ git clone https://github.com/givargis/gravity
  2. $ cd gravity/src
  3. $ make

 # A Simple Gravity Program (i.e., test.g)
 ```
 // A Simple g Program
.module "test"         ;
.prefix "test"         ;
.optimizer sgd 0.1     ;
.precision float       ;
.costfnc cross_entropy ;
.batch 8               ;
.input 28 * 28         ;
.output 10 softmax     ;
.hidden 100 relu       ;
.hidden 100 relu       ;
 ```

 # Running the Gravity Compiler
 ```
 usage: gravity [--verion][--debug] input
 ```
   1. $ cd gravity/src
   2. $ ./gravity test.g

The above will create test.h/test.c for inclusing in your driver application.
