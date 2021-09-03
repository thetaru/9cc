#include "9cc.h"

bool is_intger(Type *ty) {
	return ty->kind == TY_INT;
}

// trueのときはトークンを1つ進める
bool is_type() {
	// 型名
	static char *keywords[] = {"int"};
	char *name = token->str;
	if (token->kind == TK_RESERVED)
		for (int i = 0; i < sizeof(keywords) / sizeof(*keywords); i++) {
			char *kw = keywords[i];
			int len = strlen(kw);
			if (startswith(name, kw) && !is_alnum(name[len])) {
				token = token->next;
				return true;
			}
		}
	return false;
}

LVar set_type(LVar lvar, Type *ty) {
	lvar.ty = ty;
	return lvar;
}
