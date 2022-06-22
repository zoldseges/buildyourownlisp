#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>
#include <mpc/mpc.h>

#include "rules.h"

long eval_op(char *op, long x, long y) {
  if(!strcmp(op, "+")) {
    return x + y;
  } else if (!strcmp(op, "-")) {
    return x - y;
  } else if (!strcmp(op, "*")) {
    return x * y;
  } else if (!strcmp(op, "/")) {
    return x / y;
  } else {
    fprintf(stderr, "Couldn't evaluate operator: `%s`\n", op);
    exit(1);
  }
}

long eval(mpc_ast_t *t) {
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  char *op = t->children[1]->contents;
  long x = eval(t->children[2]);

  for(int i = 3; strstr(t->children[i]->tag, "expr"); ++i) {
    x = eval_op(op, x, eval(t->children[i]));
  }
  
  return x;
}

int main(int argc, char *argv[]) {

  mpc_parser_t *Number		= mpc_new("number");
  mpc_parser_t *Operator	= mpc_new("operator");
  mpc_parser_t *Expr		= mpc_new("expr");
  mpc_parser_t *Program		= mpc_new("program");

  mpca_lang(MPCA_LANG_DEFAULT, (const char *)src_rules_txt,
	    Number, Operator, Expr, Program);

  puts("Lispy Version 0.0.1");
  puts("Press Ctrl+c to Exit\n");

  mpc_result_t r;

  while (1) {
    char *input = readline("Lispy> ");

    if(!input) {
      printf("\n");
      break;
    }

    add_history(input);

    if (mpc_parse("<stdin>", input, Program, &r)) {
      printf("%li\n", eval(r.output));
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    } 

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expr, Program);

  return 0;
}
