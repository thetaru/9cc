#include "9cc.h"

// 入力プログラム
char *user_input;
// 現在着目しているトークン
Token *token;
// ローカル変数
LVar *locals;

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	int pos = loc - user_input;
	fprintf(stderr, "%s\n", user_input);
	fprintf(stderr, "%*s", pos, " "); // pos個の空白を出力
	fprintf(stderr, "^ ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて真を返す。
// それ以外の場合は偽を返す。
bool consume(char *op) {
	if (token->kind != TK_RESERVED ||
		strlen(op) != token->len ||
		memcmp(token->str, op, token->len))
		return false;
	token = token->next;
	return true;
}

// 次のトークンが期待している識別子のときには、現在のトークンを返してからトークンを1つ読み進める。
Token *consume_ident() {
	if (token->kind != TK_IDENT)
		return NULL;
	Token *tok = token;
	token = token->next;
	return tok;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
	if (token->kind != TK_RESERVED ||
		strlen(op) != token->len ||
		memcmp(token->str, op, token->len))
		error_at(token->str, "'%c'ではありません", op);
		token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその値を返す。
// それ以外の場合にはエラーを報告する
int expect_number() {
	if (token->kind != TK_NUM)
		error_at(token->str, "数ではありません");
	int val = token->val;
	token = token->next;
	return val;
}

bool at_eof() {
	return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	tok->len = len;
	cur->next = tok;
	return tok;
}

// 文字列qが文字列pと一致しているかを調べ、一致していたら真を返す
bool startswith(char *p, char *q) {
	return memcmp(p, q, strlen(q)) == 0;
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize() {
	char *p = user_input;
	Token head;
	head.next = NULL;
	Token *cur = &head;

	while (*p) {
		// 空白文字をスキップ
		if (isspace(*p)) {
			p++;
			continue;
		}

		if (
			startswith(p, "==") ||
			startswith(p, "!=") ||
			startswith(p, "<=") ||
			startswith(p, ">=")) {

			cur = new_token(TK_RESERVED, cur, p, 2);
			p += 2; // 2文字分アドレスを進める
			continue;
		}

		if (strchr("+-*/()<>=;", *p)) {
			cur = new_token(TK_RESERVED, cur, p++, 1);
			continue;
		}

		if ('a' <= *p && *p <= 'z') {
			char *q = p++;
			while ('a' <= *p && *p <= 'z')
				p++;
			cur = new_token(TK_IDENT, cur, q, p - q);
			continue;
		}

		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p, 0);
			char *q = p;
			cur->val = strtol(p, &p, 10);
			cur->len = p - q;
			continue;
		}

		error_at(p, "トークナイズできません");
	}

	new_token(TK_EOF, cur, p, 0);
	return head.next;
}

LVar *find_lvar(Token *tok) {
	for (LVar *var = locals; var; var = var->next)
		if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
			return var;
	return NULL;
}
