#include <mpc/mpc.h>
#include "rules.h"

#include <editline/readline.h>
#include <editline/history.h>

int main(int argc, char *argv[]) {

  mpc_parser_t* Number		= mpc_new("number");
  mpc_parser_t* Operator	= mpc_new("operator");
  mpc_parser_t* Expr		= mpc_new("expr");
  mpc_parser_t* Program		= mpc_new("program");

  mpca_lang(MPCA_LANG_DEFAULT, (const char *)src_rules_txt,
	    Number, Operator, Expr, Program);

  printf("%s\n", src_rules_txt);

  puts("Lispy Version 0.0.1");
  puts("Press Ctrl+c to Exit\n");

  mpc_result_t r;

  while (1) {
    char *input = readline("Lispy> ");
    add_history(input);

    if (mpc_parse("<stdin>", input, Program, &r)) {
      mpc_ast_print(r.output);
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
