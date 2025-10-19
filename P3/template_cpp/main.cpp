#include "Absyn.H"
#include "Parser.H"
#include <cstdint>
#include <iostream>
#include <stdio.h>
#include "CodeGen.H"

int process(FILE *input) {

  Program *parse_tree = pProgram(input);
  if (parse_tree) {
    CodeGen codegen;
    codegen.generate(parse_tree);
    //std::cout << "OK" << std::endl;
  } else {
    printf("SYNTAX ERROR\n");
    exit(1);
  }
  return 0;
}

int main(int argc, char **argv) {
  //std::cout << "Welcome to the Compiler!" << std::endl;
  FILE *input;
  char *filename = NULL;

  if (argc > 1) {
    filename = argv[1];
  } else {
    input = stdin;
  }

  if (filename) {
    input = fopen(filename, "r");
    if (!input) {
      printf("Cannot open the input file");
      exit(1);
    }
  } else {
    input = stdin;
    
  }
  process(input);

  return 0;
}
