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
// "*"* ident ";"
// tyname: 変数に設定する型
Node *set_lvar(char *tyname) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_LVAR;

	Type *type;
	type = calloc(1, sizeof(Type));
	type->ptr_to = NULL;
	while (consume("*")) {
		Type *t;
		t = calloc(1, sizeof(Type));
		t->ty = TY_PTR;
		t->ptr_to = type;
		t->size = 8;
		type = t;
	}

	Token *tok = consume_tokenkind(TK_IDENT);
	if (!tok)
		error("変数が定義できません。\n");

	LVar *lvar = find_lvar(tok);
	if (lvar) {
		char *name =  calloc(1, tok->len + 1);
		strncpy(name, tok->str, tok->len);
		error("'%s'は定義済みの変数です。\n", name);
	}

	lvar = calloc(1, sizeof(LVar));
	lvar->next = locals[funcseq];
	lvar->name = tok->str;
	lvar->len = tok->len;
	lvar->ty = type;
	lvar->ty = set_type(lvar, tyname);

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
	node->type = lvar->ty;
	locals[funcseq] = lvar;
	return node;
}

// (定義済み)変数を利用する
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
	node->type = lvar->ty;
	return node;
}

void program() {
	int i = 0;
	while(!at_eof()) {
		code[i++] = function();
	}
	code[i] = NULL;
}

// function = type ident "(" type ident* ")" "{" stmt* "}"
Node *function() {
	funcseq++;
	Node *node;

	// type
	if (!consume_type())
		error("関数の型が未定義です。");

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

	// ( type_1 arg_1, ... , type_n arg_n )
	expect("(");
	Node head;
	head.next = NULL;
	Node *cur = &head;
	while (!consume(")")) {
		// type check
		char *tyname = consume_type();
		if (!tyname)
			error("引数の型が未定義です。");

		// arg
		cur->next = set_lvar(tyname);
		cur = cur->next;

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
//      | type "*"* ident ";"
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
	// type "*"* ident ";"
	char *tyname = consume_type();
	if (tyname) {
		// ident ";"
		node = set_lvar(tyname);
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
		if (consume("+")) {
			Node *r = mul();
			if (node->type && node->type->ty == TY_PTR) {
				// DEBUG - START
				fprintf(stderr, "DEBUG: type size is %d.\n", node->type->ptr_to->size);
				// DEBUG - END

				int ptr_size = node->type->ptr_to->size;
				r = new_binary(ND_MUL, r, new_node_num(ptr_size));
			}
			node = new_binary(ND_ADD, node, r);
		} else if (consume("-")) {
			Node *r = mul();
			if (node->type && node->type->ty == TY_PTR) {
				// DEBUG - START
				fprintf(stderr, "DEBUG: type size is %d.\n", node->type->ptr_to->size);
				// DEBUG - END

				int ptr_size = node->type->ptr_to->size;
				r = new_binary(ND_MUL, r, new_node_num(ptr_size));
			}
			node = new_binary(ND_SUB, node, r);
		} else {
			return node;
		}
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
