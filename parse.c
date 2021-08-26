#include "9cc.h"

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
		code[i++] = stmt();
	}
	code[i] = NULL;
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
Node *stmt() {
	Node *node = expr();
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

// primary = num | ident | "(" expr ")"
Node *primary() {
	// 次のトークンが"("なら、"(" expr ")"のはず
	if (consume("(")) {
		Node *node = expr();
		expect(")");
		return node;
	}

	Token *tok = consume_ident();
	if (tok) {
		Node *node = calloc(1, sizeof(Node));
		node->kind = ND_LVAR;
		node->offset = (tok->str[0] - 'a' + 1) * 8;
		return node;
	}

	// そうでなければ数値のはず
	return new_node_num(expect_number());
}

