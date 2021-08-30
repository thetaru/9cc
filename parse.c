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

void program() {
	int i = 0;
	while(!at_eof()) {
		code[i++] = function();
	}
	code[i] = NULL;
}

// function = ident "(" ")" "{" stmt* "}"
Node *function() {
	Node *node;
	Token *tok = consume_tokenkind(TK_IDENT);
	if (!tok)
		error("%s is not function.\n",  tok->str);
	node = calloc(1, sizeof(Node));
	node->kind = ND_FUNC;
	node->funcname = calloc(1, tok->len + 1);
	strncpy(node->funcname, tok->str, tok->len);
	expect("(");
	expect(")");
	expect("{");
	
	Node head;
	head.next = NULL;
	Node *cur = &head;

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

// stmt = expr ";" |
//      | "{" stmt* "}"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "return" expr ";"
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
	} else {
		// returnトークンでない場合
		node = expr();
	}

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

// unary = ("+" | "-")? primary | primary
Node *unary() {
	if (consume("+"))
		// +x
		// -> x
		return primary();
	if (consume("-"))
		// -x
		// -> 0-x
		return new_binary(ND_SUB, new_node_num(0), unary());
	return primary();
}

// primary = num | ident ("(" | ")")? | "(" expr ")"
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
			expect(")");
			return node;
		}

		// 変数の場合
		Node *node = calloc(1, sizeof(Node));
		node->kind = ND_LVAR;
		LVar *lvar = find_lvar(tok);
		if (lvar) {
			node->offset = lvar->offset;
		} else {
			lvar = calloc(1, sizeof(LVar));
			lvar->next = locals;
			lvar->name = tok->str;
			lvar->len = tok->len;
			if (locals == NULL) {
				lvar->offset = 8;
			} else {
				lvar->offset += locals->offset + 8;
			}
			node->offset = lvar->offset;
			locals = lvar;
		}
		return node;
	}

	// そうでなければ数値のはず
	return new_node_num(expect_number());
}

