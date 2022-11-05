#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_NUM,      // 整数トークン
  TK_EOF,      // 入力の終端
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
  TokenKind kind; // トークンの型
  Token *next;    // 次の入力トークン
  int val;        // 整数トークンの時,数値
  char *str;      // トークン文字列
};

// 現在のトークン(global scope)
// 連結リストとして表現
// パーサが読み込むトークン列
Token *token;

// エラーを報告する関数
// 標準出力して異常終了する
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// 次のトークンが期待する記号の時はトークンを1つ読み進め真を返す
// それ以外の時は偽を返す
// トークンを直接触る(副作用あり)
bool consume(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op)
    return false;
  token = token->next;
  return true;
}

// 次のトークンが期待する記号の時はトークンを1つ読み進め
// それ以外の時はエラーを報告する
// トークンを直接触る(副作用あり)
void expect(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op)
    error("'%c'ではありません", op);
  token = token->next;
}

// 次のトークンが数値の時はトークンを1つ読み進めてその数値を返す
// それ以外の時はエラーを報告する
// トークンを直接触る(副作用あり)
int expect_number() {
  if (token->kind != TK_NUM)
    error("数値ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

// 終端の時1を返す
// トークンを直接触らない(副作用なし)
bool at_eof() { return token->kind == TK_EOF; }

// 新しいトークンを作成してcurに繋げる
// トークンを直接触らない(副作用なし)
Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

// 入力文字列pをトークナイズしトークンの先頭のToken型を返す
// トークンを直接触らない(副作用なし)
Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // skip space
    if (isspace(*p)) {
      p++;
      continue;
    }
    if (*p == '+' || *p == '-') {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }
    error("トークナイズできません");
  }
  new_token(TK_EOF, cur, p);
  return head.next;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  // tokenize
  token = tokenize(argv[1]);

  // アセンブリの前半部分（定型文）を出力
  printf(".intel_syntax noprefix\n");
  printf(".globl main\n");
  printf("main:\n");

  // 式の最初は必ず数であるのでそれをチェックし出力
  printf("	mov rax, %d\n", expect_number());

  // `+ <数値>`あるいは`- <数値>`というトークンの並びをconsumeしつつ
  // アセンブリを出力
  while (!at_eof()) {
    if (consume('+')) {
      printf("	add rax, %d\n", expect_number());
      continue;
    }

    expect('-');
    printf("	sub rax, %d\n", expect_number());
  }
  printf("	ret\n");
  return 0;
}
