#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>
#include <mpc/mpc.h>

#include "rules.h"

typedef struct lval lval;
struct lval {
  int type;
  long num;
  char *err;
  char *sym;

  int count;
  lval** cell;
};

lval *lval_eval(lval *v);
lval *builtin_op(lval *a, char *op);
void lval_print(lval *v);
// types
enum {
  LVAL_NUM,
  LVAL_ERR,
  LVAL_SYM,
  LVAL_SEXPR,
};

lval *lval_num(long x) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval *lval_err(char *m) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

lval *lval_sym(char *s) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval *lval_sexpr(void) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void *lval_add(lval *v, lval *x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}

void lval_del(lval *v) {
  switch (v->type) {
  case LVAL_NUM:
    break;
  case LVAL_ERR:
    free(v->err);
    break;
  case LVAL_SYM:
    free(v->sym);
    break;
  case LVAL_SEXPR:
    for (int i = 0; i < v->count; ++i) {
      lval_del(v->cell[i]);
    }
    free(v->cell);
    break;
  }

  free(v);
}

void *lval_pop(lval *v, int i) {
  lval *x = v->cell[i];

  memmove(&v->cell[i],
	  &v->cell[i+1],
	  sizeof(lval*) * (v->count - i - 1));
  v->count--;
  // Should we realloc after memory move?
  return x;
}

lval *lval_take(lval *v, int i) {
  lval *x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval *lval_read_num(mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno == ERANGE
    ? lval_err("invalid number")
    : lval_num(x);
}

lval *lval_read(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) return lval_read_num(t);
  if (strstr(t->tag, "symbol")) return lval_sym(t->contents);

  // Pitfall: tag is not checked if "sexpr" or ">"
  lval *x = lval_sexpr();

  for (int i = 0; i < t->children_num; i++) {
    if (!strcmp(t->children[i]->tag, "regex")
	|| t->children[i]->contents[0] == '('
	|| t->children[i]->contents[0] == ')'
	) {
      continue;
    }
    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

void lval_expr_print(lval *v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; ++i) {
    if (i != 0) putchar(' ');
    lval_print(v->cell[i]);
  }
  putchar(close);
}

void lval_print(lval *v) {
  switch (v->type) {
  case LVAL_NUM		: printf("%li", v->num);	break;
  case LVAL_ERR		: printf("Error: %s", v->err);	break;
  case LVAL_SYM		: printf("%s", v->sym);		break;
  case LVAL_SEXPR	: lval_expr_print(v, '(', ')'); break;
  }
}

void lval_println(lval *v) {
  lval_print(v);
  putchar('\n');
}

lval *lval_eval_sexpr(lval *v) {
  for (int i = 0; i < v->count; ++i) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  for (int i = 0; i < v->count; ++i) {
    if (v->cell[i]->type == LVAL_ERR) {
      return lval_take(v, i);
    }
  }

  if (v->count == 0) return v;
  if (v->count == 1) return lval_take(v, 0);

  lval *f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f);
    lval_del(v);
    return lval_err("S-expression does not start with symbol!");
  }

  lval *result = builtin_op(v, f->sym);
  lval_del(f);
  return result;
}

lval *lval_eval(lval *v) {
  if (v->type == LVAL_SEXPR) return lval_eval_sexpr(v);
  return v;
}

lval *builtin_op(lval *a, char *op) {
  for (int i = 0; i < a->count; ++i) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on non-number!");
    }
  }

  lval *x = lval_pop(a, 0);
  if (op[0] == '-' && a->count == 0) {
    x->num = -x->num;
  }

  lval *y;
  switch (op[0]) {
  case '+':
    while(a->count > 0) { y = lval_pop(a, 0); x->num += y->num; lval_del(y); };
    break;
  case '-':
    while(a->count > 0) { y = lval_pop(a, 0); x->num -= y->num; lval_del(y); };
    break;
  case '*':
    while(a->count > 0) { y = lval_pop(a, 0); x->num *= y->num; lval_del(y); };
    break;
  case '/':
    while(a->count > 0) {
      y = lval_pop(a, 0);
      if (y->num == 0) {
	lval_del(x);
	lval_del(y);
	x = lval_err("Division by Zero!");
	break;
      }
      x->num /= y->num;
      lval_del(y);
    };
    break;
  }
  lval_del(a);
  return x;
}

int main(int argc, char *argv[]) {

  mpc_parser_t *Number	= mpc_new("number");
  mpc_parser_t *Symbol	= mpc_new("symbol");
  mpc_parser_t *Sexpr	= mpc_new("sexpr");
  mpc_parser_t *Expr	= mpc_new("expr");
  mpc_parser_t *Program	= mpc_new("program");

  char rules[src_rules_txt_len+1];
  for(int i = 0; i < src_rules_txt_len; ++i) rules[i] = src_rules_txt[i];
  rules[src_rules_txt_len] = '\0';

  mpca_lang(MPCA_LANG_DEFAULT, (const char *)rules,
	    Number, Symbol, Sexpr, Expr, Program);
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
      lval *result = lval_eval(lval_read(r.output));
      lval_println(result);
      lval_del(result);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    } 

    free(input);
  }

  mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Program);

  return 0;
}
