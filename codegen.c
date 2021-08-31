#include "9cc.h"

// ラベルの通し番号
int labelseq = 1;
char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

void gen_lval(Node *node) {
	if (node->kind != ND_LVAR)
		error("代入の左辺値が変数ではありません");

	printf("  mov rax, rbp\n");
	printf("  sub rax, %d\n", node->offset);
	printf("  push rax\n");
}

void gen(Node *node) {
	switch (node->kind) {
	case ND_NUM:
		printf("  push %d\n", node->val);
		return;
	case ND_LVAR:
		gen_lval(node);
		printf("  pop rax\n");
		printf("  mov rax, [rax]\n");
		printf("  push rax\n");
		return;
	case ND_ASSIGN:
		gen_lval(node->lhs);
		gen(node->rhs);
		printf("  pop rdi\n");
		printf("  pop rax\n");
		printf("  mov [rax], rdi\n");
		printf("  push rdi\n");
		return;
	case ND_FUNC: {
		int nargs = 0;
		while (node->args) {
			node->args = node->args->next;
			nargs++;
		}

		printf("%s:\n", node->funcname);

		// プロローグ
		printf("  push rbp\n");
		printf("  mov rbp, rsp\n");
		printf("  sub rsp, %d\n", 206);
		for(int i = 0; node->args; i++) {
			node->args = node->args->next;
			printf("  push %s\n", argreg[i]);
		}

		// コード生成
		while (node->body) {
			gen(node->body);
			node->body = node->body->next;
			printf("  pop rax\n");
		}

		// エピローグ
		printf(".Lreturn.%s:\n", node->funcname);
		printf("  mov rsp, rbp\n");
		printf("  pop rbp\n");
		printf("  ret\n");
		return;
	}
	case ND_FUNCALL: {
		int seq = labelseq++;
		int nargs = 0;
		while (node->args) {
			gen(node->args);
			node->args = node->args->next;
			nargs++;
		}
		for (int i = nargs - 1; i >= 0; i--)
			printf("  pop %s\n", argreg[i]);

		printf("  mov rax, rsp\n");
		printf("  and rax, 15\n");
		printf("  jnz .L.call.%d\n", seq);
		printf("  mov rax, 0\n");
		printf("  call %s\n", node->funcname);
		printf("  jmp .L.end.%d\n", seq);
		printf(".L.call.%d:\n", seq);
		printf("  sub rsp, 8\n");
		printf("  mov rax, 0\n");
		printf("  call %s\n", node->funcname);
		printf("  add rsp, 8\n");
		printf(".L.end.%d:\n", seq);

		printf("  push rax\n");
		return;
	}
	case ND_BLOCK:
		while (node->body) {
			gen(node->body);
			node->body = node->body->next;
			printf("  pop rax\n");
		}
		return;
	case ND_IF: {
		/*
		 *                if
		 * if (A) B <=> A    B
		 *
		 */
		int seq = labelseq++;
		if (node->els) { // if-else文
			gen(node->lhs);
			printf("  pop rax\n");
			printf("  cmp rax, 0\n");
			printf("  je  .Lelse%d\n", seq);
			gen(node->rhs);
			printf("  jmp .Lend%d\n", seq);
			printf(".Lelse%d:\n", seq);
			gen(node->els);
			printf(".Lend%d:\n", seq);
		} else { // if文
			gen(node->lhs);
			printf("  pop rax\n");
			printf("  cmp rax, 0\n");
			printf("  je  .Lend%d\n", seq);
			gen(node->rhs);
			printf(".Lend%d:\n", seq);
		}
		return;
	}
	case ND_WHILE: {
		int seq = labelseq++;
		printf(".Lbegin%d:\n", seq);
		gen(node->lhs);
		printf("  pop rax\n");
		printf("  cmp rax, 0\n");
		printf("  je  .Lend%d\n", seq);
		gen(node->rhs);
		printf("  jmp .Lbegin%d\n", seq);
		printf(".Lend%d:\n", seq);
		return;
	}
	case ND_FOR: {
		int seq = labelseq++;
		if (node->init)
			gen(node->init);
		printf(".Lbegin%d:\n", seq);
		if (node->cond)
			gen(node->cond);
		printf("  pop rax\n");
		printf("  cmp rax, 0\n");
		printf("  je  .Lend%d\n", seq);
		gen(node->then);
		if (node->inc)
			gen(node->inc);
		printf("  jmp .Lbegin%d\n", seq);
		printf(".Lend%d:\n", seq);
		return;
	}
	case ND_RETURN:
		gen(node->lhs);
		printf("  pop rax\n");
		printf("  mov rsp, rbp\n");
		printf("  pop rbp\n");
		printf("  ret\n");
		return;
	}

	gen(node->lhs);
	gen(node->rhs);

	printf("  pop rdi\n");
	printf("  pop rax\n");

	switch (node->kind) {
	case ND_ADD:
		printf("  add rax, rdi\n");
		break;
	case ND_SUB:
		printf("  sub rax, rdi\n");
		break;
	case ND_MUL:
		printf("  imul rax, rdi\n");
		break;
	case ND_DIV:
		printf("  cqo\n");
		printf("  idiv rdi\n");
		break;
	case ND_EQ:
		printf("  cmp rax, rdi\n");
		printf("  sete al\n");
		printf("  movzb rax, al\n");
		break;
	case ND_NE:
		printf("  cmp rax, rdi\n");
		printf("  setne al\n");
		printf("  movzb rax, al\n");
		break;
	case ND_LT:
		printf("  cmp rax, rdi\n");
		printf("  setl al\n");
		printf("  movzb rax, al\n");
		break;
	case ND_LE:
		printf("  cmp rax, rdi\n");
		printf("  setle al\n");
		printf("  movzb rax, al\n");
		break;
	}

	printf("  push rax\n");
}
