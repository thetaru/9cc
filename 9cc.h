#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type Type;
typedef struct Node Node;

//
// tokenize.c
//

// トークンの種類
typedef enum {
	TK_RESERVED, // 記号
	TK_IDENT,    // 識別子
	TK_NUM,      // 整数トークン
	TK_STR,      // 文字列
	TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
	TokenKind kind; // トークンの型
	Token *next;    // 次の入力トークン
	int val;        // kindがTK_NUMの場合、その数値
	Type *type;     // kindがTK_STRの場合のみ使う
	char *str;      // トークン文字列
	int len;        // トークンの長さ
};

typedef struct LVar LVar;

// ローカル変数の型
struct LVar {
	LVar *next; // 次の変数かNULL
	char *name; // 変数の名前
	int len;    // 名前の長さ
	Type *ty;   // 変数の型
	int offset; // RBPからのオフセット
};

typedef struct GVar GVar;

struct GVar {
	GVar *next; // 次の変数かNULL
	char *name; // 変数の名前
	int len;    // 変数の長さ
	Type *ty;   // 変数の型
	int pos;    // メモリ位置
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
GVar *find_gvar(Token *tok);

extern char *user_input;
extern Token *token;
extern LVar *locals[];
extern GVar *globals;
extern int funcseq; // 関数の通し番号

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
	ND_STR,    // String
	ND_LVAR,   // ローカル変数
	ND_GVAR,   // グローバル変数
	ND_GVAR_DEF,   // グローバル変数の定義
	ND_RETURN, // return
	ND_IF,     // if
	ND_WHILE,  // while
	ND_FOR,    // for
	ND_BLOCK,  // { ... }
	ND_FUNC,    // 関数の定義
	ND_FUNCALL, // 関数の呼び出し
	ND_ADDR,   // &(アドレス)
	ND_DEREF   // *(アドレス参照)
} NodeKind;

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
	 * funcname (args)
	 */
	char *funcname; // 関数名
	Node *args;     // 引数

	int val;       // kindがND_NUMの場合のみ使う
	char *str;       // kindがND_STRの場合のみ使う
	int offset;    // kindがND_LVARの場合のみ使う
	Type *type;    // kindがND_(L|G)VARの場合のみ使う
	char *varname; // kindがND_(L|G)VARの場合のみ使う 変数名を意味する
	int varsize;   // kindがND_(L|G)VARの場合のみ使う
};

Node *new_node(NodeKind kind);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);

Type *get_type(Node *node);

void program();
Node *function();
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
// type.c
//

typedef enum {
	TY_INT,        // int型
	TY_CHAR,       // char型
	TY_PTR,        // ポインタ型
	TY_ARRAY,      // 配列型
} TypeKind;

struct Type {
	TypeKind kind;
	struct Type *ptr_to; // kindがTY_PTRの場合のみ使う
	int size;            // 型のサイズを指定
	size_t array_size;   // 配列の要素数を意味する
};

bool is_integer(Type *type);
char *consume_type();
Type *set_type(char *tyname);

//
// codegen.c
//

void gen_val(Node *node);
void gen(Node *node);
