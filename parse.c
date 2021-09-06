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

// ノードから型を取得する
Type *get_type(Node *node) {
	if (!node)
		return NULL;

	// 型がある場合
	if (node->type)
		return node->type;

	// 型がない場合
	// 左辺または右辺ノードの型を再帰的に取得する
	Type *t = get_type(node->lhs);
	if (!t)
		t = get_type(node->rhs);

	// 左辺または右辺ノード(のどこか)に型があり、そのタイプがND_DEREFの場合
	// 1回ポインタをたどる
	if (t && node->kind == ND_DEREF) {
		t = t->ptr_to;
		if (t == NULL) {
			error("ポインタをたどることができません。\n");
		}
		return t;
	}
	return t;
}

// 変数の型のサイズを取得する
int get_tysize(Type *type) {
	// 変数が利用する領域
	int size = type->size; // ポインタ型でない場合の領域はこれでOK

	while (type->ptr_to) {
		type = type->ptr_to;
		size = type->size;
	}

	return size;
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
	type = set_type(tyname);
	type->ptr_to = NULL;
	type->array_size = 1; // 配列型でない変数も0次元の要素数1の配列とみなす
	while (consume("*")) {
		Type *t;
		t = calloc(1, sizeof(Type));
		t->kind = TY_PTR;
		t->ptr_to = type;
		t->array_size = 1;
		t->size = 8;
		type = t;
	}

	Token *tok = consume_tokenkind(TK_IDENT);
	if (!tok)
		error("変数が定義できません。\n");

	// 配列を定義する
	// e.g. dim = 1, [3]
	//      dim = 2, [3][5]
	while (consume("[")) {
		Type *t;
		t = calloc(1, sizeof(Type));
		t->kind = TY_ARRAY;
		t->ptr_to = type;
		t->array_size = expect_number(); //要素数を取得
		type = t;
		expect("]");
	}

	LVar *lvar = find_lvar(tok);
	if (lvar) {
		char *name =  calloc(1, tok->len + 1);
		strncpy(name, tok->str, tok->len);
		error("'%s'は定義済みのローカル変数です。\n", name);
	}

	lvar = calloc(1, sizeof(LVar));
	lvar->next = locals[funcseq];
	lvar->name = tok->str;
	lvar->len = tok->len;
	lvar->ty = type;

	// DEBUG - START
	char *name =  calloc(1, tok->len + 1);
	strncpy(name, tok->str, tok->len);
	fprintf(stderr, "DEBUG: local variable '%s' is defined.\n", name);
	// DEBUG - END

	// 変数が利用する領域
	int var_size;

	if (type->kind == TY_ARRAY) {
		// 配列の場合
		var_size = get_tysize(type);
		while (type->ptr_to) {
			var_size *= type->array_size;
			type = type->ptr_to;
		}
	} else {
		// 配列でない場合
		var_size = type->size;
	}

	// offsetは8の倍数でないといけないらしい...？
	while ((var_size % 8) != 0) {
		var_size += 4;
	}

	// DEBUG - START
	fprintf(stderr, "DEBUG: type size of %s is %d.\n", name, var_size);
	// DEBUG - END

	if (locals[funcseq] == NULL) {
		lvar->offset = var_size;
	} else {
		lvar->offset += locals[funcseq]->offset + var_size;
	}
	node->offset = lvar->offset;
	node->type = lvar->ty;
	node->varsize = var_size;
	locals[funcseq] = lvar;
	return node;
}

// (定義済み)変数を利用する
Node *use_var(Token *tok) {
	Node *node = calloc(1, sizeof(Node));
	node->varname = calloc(1, tok->len + 1);
	memcpy(node->varname, tok->str, tok->len);

	GVar *gvar = find_gvar(tok);
	LVar *lvar = find_lvar(tok);
	if (gvar || lvar) {
		// ローカル変数・グローバル変数のどちらかが定義されている場合
		// どちらも定義されている場合はローカル変数を優先する
		node->kind = lvar ? ND_LVAR : ND_GVAR;
	} else {
		// どちらも定義されていない場合
		error("'%s'は未定義の変数です。\n", node->varname);
	}

	if (node->kind == ND_LVAR) {
		// ローカル変数のメンバの値を設定する
		node->offset = lvar->offset;
		node->type = lvar->ty;
	} else if (node->kind == ND_GVAR) {
		// グローバル変数のメンバの値を設定する
		node->type = gvar->ty;
	}

	// 変数が配列の場合
	// ident "[" expr "]"
	// e.g. x[2] は *(x+2) を意味する
	while (consume("[")) {
		Node *add = new_binary(ND_ADD, node, expr());
		node = new_binary(ND_DEREF, add, NULL);
		expect("]");
	}
	return node;
}

void program() {
	int i = 0;
	while(!at_eof()) {
		code[i++] = function();
	}
	code[i] = NULL;
}

// function = type ident "(" type ident* ")" "{" stmt* "}" | type indent ";"
Node *function() {
	Node *node;

	// type
	char *idname = consume_type();
	if (!idname)
		error("関数の型が未定義です。");

	// ident
	Token *tok = consume_tokenkind(TK_IDENT);
	if (!tok)
		error("%s is not function or global variable.\n",  tok->str);

	// ( type_1 arg_1, ... , type_n arg_n )
	if (consume("(")) {
		// 関数を定義する
		funcseq++;

		node = calloc(1, sizeof(Node));
		node->kind = ND_FUNC;
		node->funcname = calloc(1, tok->len + 1);
		strncpy(node->funcname, tok->str, tok->len);

		Node head;
		head.next = NULL;
		Node *cur = &head;
		while (!consume(")")) {
			// type check
			char *tyname = consume_type();
			if (!tyname)
				error("引数の型が未定義です。\n");

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

		// DEBUG - START
		fprintf(stderr, "DEBUG: function %s is defined.\n", node->funcname);
		// DEBUG - END

		return node;
	} else {
		// グローバル変数を定義する
		node = calloc(1, sizeof(Node));
		node->kind = ND_GVAR_DEF;

		Type *type;
		type = set_type(idname);
		type->ptr_to = NULL;
		type->array_size = 1;
		while (consume("*")) {
			Type *t;
			t = calloc(1, sizeof(Type));
			t->kind = TY_PTR;
			t->ptr_to = type;
			t->array_size = 1;
			t->size = 8;
			type = t;
		}

		while (consume("[")) {
			Type *t;
			t = calloc(1, sizeof(Type));
			t->kind = TY_ARRAY;
			t->ptr_to = type;
			t->array_size = expect_number();
			type = t;
			expect("]");
		}

		GVar *gvar = find_gvar(tok);
		if (gvar) {
			char *name = calloc(1, tok->len + 1);
			strncpy(name, tok->str, tok->len);
			error("'%s'は定義済みのグローバル変数です。\n", name);
		}

		gvar = calloc(1, sizeof(GVar));
		gvar->next = globals;
		gvar->name = tok->str;
		gvar->len = tok->len;
		gvar->ty = type;

		int var_size;
		if (type->kind == TY_ARRAY) {
			var_size = get_tysize(type);
			while (type->ptr_to) {
				var_size *= type->array_size;
				type = type->ptr_to;
			}
		} else {
			var_size = type->size;
		}

		// 不要?
		//while ((var_size % 8) != 0) {
			//var_size += 4;
		//}

		expect(";");

		// DEBUG - START
		char *name = calloc(1, tok->len + 1);
		strncpy(name, tok->str, tok->len);
		fprintf(stderr, "DEBUG: global variable '%s' is defined.\n", name);
		// DEBUG - END

		node->type = gvar->ty;
		node->varname = calloc(1, tok->len + 1);
		strncpy(node->varname, tok->str, tok->len);
		node->varsize = var_size;
		globals = gvar;
		return node;
	}
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
			if (node->type && node->type->ptr_to) {
				int size = get_tysize(node->type);
				r = new_binary(ND_MUL, r, new_node_num(size));
			}
			node = new_binary(ND_ADD, node, r);
		} else if (consume("-")) {
			Node *r = mul();
			if (node->type && node->type->ptr_to) {
				int size = get_tysize(node->type);
				r = new_binary(ND_MUL, r, new_node_num(size));
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

// unary = "sizeof" unary | ("+" | "-")? primary | ("*" | "&") unary
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
	if (consume("sizeof")) {
		Node *node = unary();
		Type *type = get_type(node);
		int size;

		if (type) {
			// 変数の場合
			size = type->kind == TY_PTR ? 8 : get_tysize(type);
		} else {
			// 数値の場合
			// e.g. sizeof(1)
			size = 4;
		}

		return new_node_num(size);
	}
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
		return use_var(tok);
	}

	// そうでなければ数値のはず
	return new_node_num(expect_number());
}

