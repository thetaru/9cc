#include "9cc.h"

bool is_integer(Type *type) {
	return type->kind = TY_INT;
}

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
Type *set_type(char *tyname) {
	Type *type;
	type = calloc(1, sizeof(Type));

	// int型を設定
	if (startswith(tyname, "int")) {
		type->kind = TY_INT;
		type->size = 4;
	}

	// 型を追加するたびに追記する...

	return type;
}
