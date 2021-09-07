#include "9cc.h"

// 入力プログラム
char *user_input;
// 入力ファイル名
char *filename;
// 現在着目しているトークン
Token *token;
// ローカル変数
LVar *locals[100];
int funcseq = 0;
int strseq = 0;
// グローバル変数
GVar *globals;
// 文字列
String *strings;

// 指定されたファイルの内容を返す
char *read_file(char *path) {
	// ファイルを開く
	FILE *fp = fopen(path, "r");
	if (!fp)
		error("cannot open %s: %s", path, strerror(errno));

	// ファイルの長さを調べる
	if (fseek(fp, 0, SEEK_END) == -1)
		error("%s: fseek: %s", path, strerror(errno));
	size_t size = ftell(fp);
	if (fseek(fp, 0, SEEK_SET) == -1)
		error("%s: fseek: %s", path, strerror(errno));

	// ファイル内容を読み込む
	char *buf = calloc(1, size + 2);
	fread(buf, size, 1, fp);

	// ファイルが必ず"\n\0"で終わっているようにする
	if (size == 0 || buf[size-1] != '\n')
		buf[size++] = '\n';
	buf[size] = '\0';
	fclose(fp);
	return buf;
}

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
void error_at(char *loc, char *msg) {
	// locが含まれている行の開始地点と終了地点を取得
	char *line = loc;
	while (user_input < line && line[-1] != '\n')
		line--;

	char *end = loc;
	while (*end != '\n')
		end++;

	// 見つかった行が全体の何行目なのかを調べる
	int line_num = 1;
	for (char *p = user_input; p < line; p++)
		if (*p == '\n')
			line_num++;

	// 見つかった行を、ファイル名と行番号と一緒に表示
	int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
	fprintf(stderr, "%.*s\n", (int)(end - line), line);

	// エラー箇所を"^"で指し示して、エラーメッセージを表示
	int pos = loc - line + indent;
	fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
	fprintf(stderr, "^ %s\n", msg);
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

// 次のトークンが期待しているトークン型であるときには、現在のトークンを返してからトークンを1つ読み進める。
// それ以外の場合はNULLを返す。
Token *consume_tokenkind(TokenKind kind) {
	if (token->kind != kind)
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
		error_at(token->str, "'%s'ではありません");
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

// アルファベット(a~z, A~Z)またはアンダーバー(_)であることを確認する
bool is_alpha(char c) {
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

// アルファベット(a~z, A~Z)またはアンダーバー(_)または数字(0~9)であることを確認する
int is_alnum(char c) {
	return is_alpha(c) || ('0' <= c && c <= '9');
}

// 予約語を取得する
char *fetch_reserved(char *p) {
	static char *keywords[] = {"return", "if", "else", "while", "for", "int", "sizeof", "char"};
	for (int i = 0; i < sizeof(keywords) / sizeof(*keywords); i++) {
		char *kw = keywords[i];
		int len = strlen(kw);
		if (startswith(p, kw) && !is_alnum(p[len])) {
			return kw;
		}
	}
	return NULL;
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

		// 行コメント
		if (startswith(p, "//")) {
			p += 2;
			while (*p != '\n')
				p++;
			continue;
		}

		// ブロックコメントをスキップ
		if (strncmp(p, "/*", 2) == 0) {
			char *q = strstr(p + 2, "*/");
			if (!q)
				error_at(p, "コメントが閉じられていません");
			p = q + 2;
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

		if (strchr("+-*/()<>=;{},&[]", *p)) {
			cur = new_token(TK_RESERVED, cur, p++, 1);
			continue;
		}

		char *kw = fetch_reserved(p);
		if (kw) {
			int len = strlen(kw);
			cur = new_token(TK_RESERVED, cur, p, len);
			p += len;
			continue;
		}

		// 変数の1文字目がアルファベットであることを確認する
		if (is_alpha(*p)) {
			char *q = p++;
			while (is_alnum(*p))
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

		// 文字列であることを確認する
		if ('"' == *p) {
			char *q = ++p;

			// 文字列チェック
			for (; *p != '"'; p++) {
				if (*p == '\n' || *p == '\0')
					error("文字列が閉じていません。\n");
			}

			// 文字列の抜き出し
			cur = new_token(TK_STR, cur, q, p - q);
			p += 1;
			continue;
		}

		error_at(p, "トークナイズできません");
	}

	new_token(TK_EOF, cur, p, 0);
	return head.next;
}

// ND_LVARトークンがローカル変数localsに含まれていることを確認する
// 見つからなかった場合はNULLを返す
LVar *find_lvar(Token *tok) {
	for (LVar *var = locals[funcseq]; var; var = var->next)
		if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
			return var;
	return NULL;
}

// ND_GVARトークンがグローバル変数globalsに含まれていることを確認する
// 見つからなった場合はNULLを返す
GVar *find_gvar(Token *tok) {
	for (GVar *var = globals; var; var = var->next)
		if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
			return var;
	return NULL;
}

