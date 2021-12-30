#include "9cc.h"
int main(int argc, char **argv)
{
  if (argc != 2)
  {
    error("引数の個数が正しくありません");
    return 1;
  }

  // トークナイズする
  user_input = argv[1];
  token = tokenize(user_input);
  Node *node = expr();

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".globl main\n");
  printf("main:\n");

  // 木を下りながらコード生成
  gen(node);

  // raxに値をロードして返り値とする
  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}
