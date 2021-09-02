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

	// 先頭の式から順にコード生成
	funcseq = 0;
	for (int i = 0; code[i]; i++) {
		funcseq++;
		gen(code[i]);
	}

	return 0;
}
