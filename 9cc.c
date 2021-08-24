#include "9cc.h"

int main(int argc, char *argv[]) {
	if (argc != 2) {
		error("引数の個数が正しくありません");
		return 1;
	}

	// トークナイズしてパースする
	user_input = argv[1];
	token = tokenize();
	Node *node = expr();

	// アセンブリの前眼部分を出力
	printf(".intel_syntax noprefix\n");
	printf(".global main\n");
	printf("main:\n");

	// 抽象構文木を下りながらコードを生成
	gen(node);

	// スタックトップに式全体の値が残っているはずなので
	// それをRAXレジスタにロードしてから関数の返り値とする
	printf("  pop rax\n");
	printf("  ret\n");
	return 0;
}
