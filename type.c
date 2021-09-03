#include "9cc.h"

// trueのときはトークンを1つ進める
char *consume_type() {
	// 型名
	static char *keywords[] = {"int"};
	char *name = token->str;
	if (token->kind == TK_RESERVED)
		for (int i = 0; i < sizeof(keywords) / sizeof(*keywords); i++) {
			char *kw = keywords[i];
			int len = strlen(kw);
			if (startswith(name, kw) && !is_alnum(name[len])) {
				token = token->next;
				return name;
			}
		}
	return NULL;
}

// 変数に型を設定する
Type *set_type(LVar *lvar, char *tyname) {
	Type *type;
	type = calloc(1, sizeof(Type));
	type->ptr_to = NULL;
	// int
	if (startswith(tyname, "int")) {
		// DEBUG - START
		int len = strlen("int");
		char *type_name = calloc(1, len + 1);
		strncpy(type_name, tyname, len);
		len = lvar->len;
		char *lvar_name = calloc(1, len + 1);
		strncpy(lvar_name, lvar->name, len);
		fprintf(stderr, "DEBUG: type of variable '%s' is %s.\n", lvar_name, type_name);
		// DEBUG - END

		type->ty = TY_INT;
	}

	// 型を追加するたびに追記する...

	return type;
}
