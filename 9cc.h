#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// tokenize.c
//

// トークンの種類
typedef enum {
	TK_RESERVED, // 記号
	TK_IDENT,    // 識別子
	TK_NUM,      // 整数トークン
	TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
	TokenKind kind; // トークンの型
	Token *next;    // 次の入力トークン
	int val;        // kindがTK_NUMの場合、その数値
	char *str;      // トークン文字列
	int len;        // トークンの長さ
};

typedef struct LVar LVar;

// ローカル変数の型
struct LVar {
	LVar *next; // 次の変数かNULL
	char *name; // 変数の名前
	int len;    // 名前の長さ
	int offset; // RBPからのオフセット
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(char *op);
Token *consume_tokenkind(TokenKind kind);
void expect(char *op);
int expect_number();
bool at_eof();
bool is_alpha(char c);
int is_alnum(char c);
char *fetch_reserved(char *p);
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
bool startswith(char *p, char *q);
Token *tokenize();
LVar *find_lvar(Token *tok);

extern char *user_input;
extern Token *token;
extern LVar *locals;

//
// parse.c
//

typedef enum {
	ND_ADD,    // +
	ND_SUB,    // -
	ND_MUL,    // *
	ND_DIV,    // /
	ND_EQ,     // ==
	ND_NE,     // !=
	ND_LT,     // <
	ND_LE,     // <=
	ND_ASSIGN, // =
	ND_NUM,    // Integer
	ND_LVAR,   // ローカル変数
	ND_RETURN, // return
	ND_IF,     // if
	ND_WHILE,  // while
	ND_FOR,    // for
	ND_BLOCK,  // { ... }
	ND_FUNCALL, // 関数の呼び出し
} NodeKind;

typedef struct Node Node;
// 抽象構文木のノードの型
struct Node {
	NodeKind kind; // ノードの型
	Node *next;    // 次のノード
	Node *lhs;     // 左辺
	Node *rhs;     // 右辺

	Node *els;     // kindがND_IFの場合のみ使う

	/*
	 * for (init; cond; inc) then
	 */
	Node *init;    // 初期化式
	Node *cond;    // 条件式
	Node *inc;     // 変化式
	Node *then;    // 処理内容

	Node *body;    // kindがND_BLOCKの場合のみ使う

	/*
	 * funcname ()
	 */
	char *funcname; // 関数名

	int val;       // kindがND_NUMの場合のみ使う
	int offset;    // kindがND_LVARの場合のみ使う
};

Node *new_node(NodeKind kind);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);

void program();
Node *assign();
Node *expr();
Node *stmt();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

extern Node *code[100];

//
// codegen.c
//

void gen_lval(Node *node);
void gen(Node *node);
