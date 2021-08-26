#include "9cc.h"

int main(int argc, char *argv[]) {
	if (argc != 2) {
		error("引数の個数が正しくありません");
		return 1;
	}

	// トークナイズしてパースする
	// 結果はcodeに保存する
	user_input = argv[1];
	token = tokenize();
	program();

	// アセンブリの前眼部分を出力
	printf(".intel_syntax noprefix\n");
	printf(".global main\n");
	printf("main:\n");

	// プロローグ
	// 変数26個分の領域を確保する
	printf("  push rbp\n");
	printf("  mov rbp, rsp\n");
	printf("  sub rsp, 208\n");

	// 先頭の式から順にコード生成
	for (int i = 0; code[i]; i++) {
		gen(code[i]);

		// 式の評価結果としてスタックに1つの値が残っているはずなので、
		// スタックが溢れないようにポップしておく
		printf("  pop rax\n");
	}

	// エピローグ
	// 最後の式の結果がRAXレジスタに残っているはずなのでそれを関数の返り値とする
	printf("  mov rsp, rbp\n");
	printf("  pop rbp\n");
	printf("  ret\n");
	return 0;
}
