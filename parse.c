#include "9cc.h"
#include <stdio.h>

Node *code[100];

Node *new_node(NodeKind kind) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
	Node *node = new_node(kind);
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_node_num(int val) {
	Node *node = new_node(ND_NUM);
	node->val = val;
	return node;
}

// ノードに呼び出された関数の引数を追加し、その単方向リストを返す。
// 引数がない場合はNULLを返す。
Node *func_args() {
	// 引数がない場合
	if (consume(")")) {
		return NULL;
	}

	// 引数がある場合
	Node head;
	head.next = NULL;
	Node *cur = &head;
	cur->next = expr(); // 先に引数を1つ読み取る
	cur = cur->next;
	while(consume(",")) {
		cur->next = expr();
		cur = cur->next;
	}
	expect(")");
	return head.next;
}

// ローカル変数を設定する
Node *set_lvar(Token *tok) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_LVAR;
	LVar *lvar = find_lvar(tok);
	if (lvar) {
		char *name =  calloc(1, tok->len + 1);
		strncpy(name, tok->str, tok->len);
		error("'%s'は定義済みの変数です。\n", name);
	} else {
		lvar = calloc(1, sizeof(LVar));
		lvar->next = locals[funcseq];
		lvar->name = tok->str;
		lvar->len = tok->len;

		// DEBUG - START
		char *name =  calloc(1, tok->len + 1);
		strncpy(name, tok->str, tok->len);
		fprintf(stderr, "DEBUG: local variable %s is defined.\n", name);
		// DEBUG - END

		if (locals[funcseq] == NULL) {
			lvar->offset = 8;
		} else {
			lvar->offset += locals[funcseq]->offset + 8;
		}
		node->offset = lvar->offset;
		locals[funcseq] = lvar;
	}
	return node;
}

Node *use_lvar(Token *tok) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_LVAR;
	LVar *lvar = find_lvar(tok);
	if (!lvar) {
		char *name =  calloc(1, tok->len + 1);
		strncpy(name, tok->str, tok->len);
		error("'%s'は未定義の変数です。\n", name);
	}
	node->offset = lvar->offset;
	return node;
}

void program() {
	int i = 0;
	while(!at_eof()) {
		code[i++] = function();
	}
	code[i] = NULL;
}

// function = type ident "(" ident* ")" "{" stmt* "}"
Node *function() {
	funcseq++;
	Node *node;

	// type
	if (!is_type())
		error("型が定義されていません。");

	// ident
	Token *tok = consume_tokenkind(TK_IDENT);
	if (!tok)
		error("%s is not function.\n",  tok->str);
	node = calloc(1, sizeof(Node));
	node->kind = ND_FUNC;
	node->funcname = calloc(1, tok->len + 1);
	strncpy(node->funcname, tok->str, tok->len);

	// DEBUG - START
	fprintf(stderr, "DEBUG: function %s is defined.\n", node->funcname);
	// DEBUG - END

	// ( arg_1, ... , arg_n )
	expect("(");
	Node head;
	head.next = NULL;
	Node *cur = &head;
	while (!consume(")")) {
		Token *tok = consume_tokenkind(TK_IDENT);
		if (tok) {
			cur->next = set_lvar(tok);
			cur = cur->next;
		}
		if (consume(")"))
			break;
		expect(",");
	}
	node->args = head.next;

	// { ... }
	expect("{");
	head.next = NULL;
	cur = &head;
	while (!consume("}")) {
		cur->next = stmt();
		cur = cur->next;
	}
	node->body = head.next;

	return node;
}

// assign = equality ("=" assign)?
Node *assign() {
	Node *node = equality();
	if (consume("="))
		node = new_binary(ND_ASSIGN, node, assign());

	return node;
}

// expr = equality
Node *expr() {
	return assign();
}

// stmt = expr ";"
//      | "{" stmt* "}"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "return" expr ";"
//      | type ident ";"
Node *stmt() {
	Node *node;

	if (consume("{")) {
		// ブロック内のステートメントをNodeの単方向リストとして表現する
		Node head;
		head.next = NULL;
		Node *cur = &head;
		while(!consume("}")) {
			cur->next = stmt();
			cur = cur->next;
		}
		node = calloc(1, sizeof(Node));
		node->kind = ND_BLOCK;
		node->body = head.next;
		return node;
	}

	if (consume("if")) {
		// "("と")"の確認をしないといけないためnew_binaryは使用不可
		expect("(");
		node = calloc(1, sizeof(Node));
		node->kind = ND_IF;
		node->lhs = expr();
		expect(")");
		node->rhs = stmt();
		if (consume("else"))
			node->els = stmt();
		return node;
	}

	if (consume("while")) {
		expect("(");
		node = calloc(1, sizeof(Node));
		node->kind = ND_WHILE;
		node->lhs = expr();
		expect(")");
		node->rhs = stmt();
		return node;
	}

	if (consume("for")) {
		expect("(");
		node = calloc(1, sizeof(Node));
		node->kind = ND_FOR;

		// トークンを1つ先読みする
		if (!consume(";")) {
			node->init = expr();
			expect(";");
		}
		if (!consume(";")) {
			node->cond = expr();
			expect(";");
		}
		if (!consume(")")) {
			node->inc = expr();
			expect(")");
		}

		node->then = stmt();
		return node;
	}

	if (consume("return")) {
		// returnトークンの場合
		node = calloc(1, sizeof(Node));
		node->kind = ND_RETURN;
		node->lhs = expr();
		expect(";");
		return node;
	}

	// 変数定義
	if (is_type()) {
		Token *tok = consume_tokenkind(TK_IDENT);
		node = set_lvar(tok);
		expect(";");
		return node;
	}

	node = expr();
	expect(";");
	return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
	Node *node = relational();

	for (;;) {
		if (consume("=="))
			node = new_binary(ND_EQ, node, relational());
		else if (consume("!="))
			node = new_binary(ND_NE, node, relational());
		else
			return node;
	}
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
	Node *node = add();

	for (;;) {
		if (consume("<"))
			node = new_binary(ND_LT, node, add());
		else if (consume("<="))
			node = new_binary(ND_LE, node, add());
		// ">",">="はそれぞれ"<","<="のLHS, RHS部を反転させて実装
		else if (consume(">"))
			node = new_binary(ND_LT, add(), node);
		else if (consume(">="))
			node = new_binary(ND_LE, add(), node);
		else
			return node;
	}
}

// add = mul ("+" mul | "-" mul)*
Node *add() {
	Node *node = mul();

	for (;;) {
		if (consume("+"))
			node = new_binary(ND_ADD, node, mul());
		else if (consume("-"))
			node = new_binary(ND_SUB, node, mul());
		else
			return node;
	}
}

// mul = unary ("*" unary | "/" unary)*
Node *mul() {
	Node *node = unary();

	for (;;) {
		if (consume("*"))
			node = new_binary(ND_MUL, node, unary());
		else if (consume("/"))
			node = new_binary(ND_DIV, node, unary());
		else
			return node;
	}
}

// unary = ("+" | "-")? primary | ("*" | "&") unary
Node *unary() {
	if (consume("+"))
		// +x
		// -> x
		return primary();
	if (consume("-"))
		// -x
		// -> 0-x
		return new_binary(ND_SUB, new_node_num(0), unary());
	if (consume("*"))
		return new_binary(ND_DEREF, unary(), NULL);
	if (consume("&"))
		return new_binary(ND_ADDR, unary(), NULL);
	return primary();
}

// primary = num | ident ("(" expr* ")")? | "(" expr ")"
Node *primary() {
	// 次のトークンが"("なら、"(" expr ")"のはず
	if (consume("(")) {
		Node *node = expr();
		expect(")");
		return node;
	}

	Token *tok = consume_tokenkind(TK_IDENT);
	if (tok) {
		// 関数の場合
		if (consume("(")) {
			Node *node = calloc(1, sizeof(Node));
			node->kind = ND_FUNCALL;
			node->funcname = calloc(1, tok->len + 1);
			strncpy(node->funcname, tok->str, tok->len);

			// 引数の割り当て
			node->args = func_args();

			return node;
		}

		// (定義済みの)変数の場合
		return use_lvar(tok);
	}

	// そうでなければ数値のはず
	return new_node_num(expect_number());
}
