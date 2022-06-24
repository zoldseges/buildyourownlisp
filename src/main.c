#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>
#include <mpc/mpc.h>

#include "rules.h"

typedef struct {
  int type;
  long num;
  int err;
} lval;

// types
enum {
  LVAL_NUM,
  LVAL_ERR
};

// errors
enum {
  LERR_DIV_ZERO,
  LERR_BAD_OP,
  LERR_BAD_NUM,
};

lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

lval lval_err(int e) {
  lval v;
  v.type = LVAL_ERR;
  v.err = e;
  return v;
}

void lval_print(lval v) {
  switch (v.type) {
  case LVAL_NUM:
    printf("%li", v.num);
    break;

  case LVAL_ERR:
    switch (v.err) {
    case LERR_DIV_ZERO:
      printf("Error: Division by Zero!");
      break;
    case LERR_BAD_OP:
      printf("Error: Invalid Operator!");
      break;
    case LERR_BAD_NUM:
      printf("Error: Invalid Number!");
      break;
    }
  }
}

void lval_println(lval v) {
  lval_print(v);
  putchar('\n');
}

lval eval_op(char *op, lval x, lval y) {
  if (x.type == LVAL_ERR) return x;
  if (y.type == LVAL_ERR) return y;

  if(!strcmp(op, "+")) {
    return lval_num(x.num + y.num);
  } else if (!strcmp(op, "-")) {
    return lval_num(x.num - y.num);
  } else if (!strcmp(op, "*")) {
    return lval_num(x.num * y.num);
  } else if (!strcmp(op, "/")) {
    return y.num == 0
      ? lval_err(LERR_DIV_ZERO)
      : lval_num(x.num / y.num);
  } else {
    return lval_err(LERR_BAD_OP);
  }
}

lval eval(mpc_ast_t *t) {
  if (strstr(t->tag, "number")) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno == ERANGE ? lval_err(LERR_BAD_NUM) : lval_num(x);
  }

  char *op = t->children[1]->contents;
  lval x = eval(t->children[2]);

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
  char rules[src_rules_txt_len+1];
  for(int i = 0; i < src_rules_txt_len; ++i) rules[i] = src_rules_txt[i];
  rules[src_rules_txt_len] = '\0';

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
      lval result = eval(r.output);
      lval_println(result);
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
