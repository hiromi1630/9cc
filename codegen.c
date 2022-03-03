#include "9cc.h"

int count = 0;
// ノードの作成
Node *new_node(NodeKind kind, Node *lhs, Node *rhs)
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

// ノードの作成(整数)
Node *new_node_num(int val)
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

Node *code[100];

void program()
{
  int i = 0;
  while (!at_eof())
  {
    code[i++] = stmt();
  }
  code[i] = NULL;
}

Node *stmt()
{
  Node *node;
  if (consume("{"))
  {
    node = calloc(1, sizeof(Node));
    node->kind = ND_BLOCK;
    // TODO 100
    node->block = calloc(100, sizeof(Node));
    for (int i = 0; !consume("}"); i++)
    {
      node->block[i] = stmt();
    }
    return node;
  }
  if (consume_token(TK_IF))
  {
    node = calloc(1, sizeof(Node));
    node->kind = ND_IF;
    consume("(");
    node->lhs = expr();
    consume(")");
    node->rhs = stmt();
    if (consume_token(TK_ELSE))
    {
      node->els = stmt();
    }
    return node;
  }
  else if (consume_token(TK_RETURN))
  {
    node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->lhs = expr();
  }
  else if (consume_token(TK_WHILE))
  {
    node = calloc(1, sizeof(Node));
    node->kind = ND_WHILE;
    consume("(");
    node->lhs = expr();
    consume(")");
    node->rhs = stmt();
    return node;
  }
  else if (consume_token(TK_FOR))
  {
    node = calloc(1, sizeof(Node));
    node->kind = ND_FOR;
    Node *left = calloc(1, sizeof(Node));
    left->kind = ND_FOR_LEFT;
    Node *right = calloc(1, sizeof(Node));
    right->kind = ND_FOR_RIGHT;
    expect("(");
    if (!consume(";"))
    {
      left->lhs = expr();
      expect(";");
    }
    if (!consume(";"))
    {
      left->rhs = expr();
      expect(";");
    }
    if (!consume(")"))
    {
      right->lhs = expr();
      expect(")");
    }
    right->rhs = stmt();
    node->lhs = left;
    node->rhs = right;
    return node;
  }
  else
  {
    node = expr();
  }
  expect(";");
  return node;
}

Node *expr()
{
  return assign();
}

Node *assign()
{
  Node *node = equality();
  if (consume("="))
  {
    node = new_node(ND_ASSIGN, node, assign());
  }
  return node;
}

Node *equality()
{
  Node *node = relational();
  for (;;)
  {
    if (consume("=="))
    {
      node = new_node(ND_EQ, node, relational());
    }
    else if (consume("!="))
    {
      node = new_node(ND_NE, node, relational());
    }
    else
    {
      return node;
    }
  }
}

Node *relational()
{
  Node *node = add();
  for (;;)
  {
    if (consume("<"))
    {
      node = new_node(ND_LT, node, add());
    }
    else if (consume("<="))
    {
      node = new_node(ND_LE, node, add());
    }
    else if (consume(">"))
    {
      node = new_node(ND_LT, add(), node);
    }
    else if (consume(">="))
    {
      node = new_node(ND_LE, add(), node);
    }
    else
    {
      return node;
    }
  }
}

Node *add()
{
  Node *node = mul();
  for (;;)
  {
    if (consume("+"))
    {
      node = new_node(ND_ADD, node, mul());
    }
    else if (consume("-"))
    {
      node = new_node(ND_SUB, node, mul());
    }
    else
    {
      return node;
    }
  }
}

Node *mul()
{
  Node *node = unary();
  for (;;)
  {
    if (consume("*"))
    {
      node = new_node(ND_MUL, node, unary());
    }
    else if (consume("/"))
    {
      node = new_node(ND_DIV, node, unary());
    }
    else
    {
      return node;
    }
  }
}

Node *unary()
{
  if (consume("+"))
  {
    return unary();
  }
  else if (consume("-"))
  {
    return new_node(ND_SUB, new_node_num(0), unary());
  }
  else
  {
    return primary();
  }
}

LVar *locals;

Node *primary()
{
  if (consume("("))
  {
    Node *node = expr();
    expect(")");
    return node;
  }
  Token *tok = consume_token(TK_IDENT);
  if (tok)
  {
    if (consume("("))
    {
      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_FUNC;
      node->funcName = tok->str;
      node->len = tok->len;
      expect(")");
      return node;
    }

    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    LVar *lvar = find_lvar(tok);
    if (lvar)
    {
      node->offset = lvar->offset;
    }
    else
    {
      lvar = calloc(1, sizeof(LVar));
      lvar->next = locals;
      lvar->name = tok->str;
      lvar->len = tok->len;
      if (locals == NULL)
      {
        lvar->offset = 8;
      }
      else
      {
        lvar->offset = locals->offset + 8;
      }
      node->offset = lvar->offset;
      locals = lvar;
    }
    return node;
  }
  return new_node_num(expect_number());
}

char name[100] = {0};

void gen_lval(Node *node)
{
  if (node->kind != ND_LVAR)
  {
    error("代入の左辺値が変数ではありません");
  }
  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

void gen(Node *node)
{
  count++;
  int id = count;
  if (!node)
  {
    return;
  }
  switch (node->kind)
  {
  case ND_FUNC:
    memcpy(name, node->funcName, node->len);
    printf("  call %s\n", name);
    return;
  case ND_BLOCK:
    for (int i = 0; node->block[i]; i++)
    {
      gen(node->block[i]);
      printf("  pop rax\n");
    }
    return;
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;
  case ND_IF:
    if (node->els == NULL)
    {
      gen(node->lhs);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .Lend%d\n", id);
      gen(node->rhs);
      printf(".Lend%d:\n", id);
    }
    else
    {
      gen(node->lhs);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .Lelse%d\n", id);
      gen(node->rhs);
      printf("  jmp .Lend%d\n", id);
      printf(".Lelse%d:\n", id);
      gen(node->els);
      printf(".Lend%d:\n", id);
    }
    return;
  case ND_WHILE:
    printf(".Lbegin%d:\n", id);
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lend%d\n", id);
    gen(node->rhs);
    printf("  jmp .Lbegin%d\n", id);
    printf(".Lend%d:\n", id);
    return;
  case ND_FOR:
    gen(node->lhs->lhs);
    printf(".Lbegin%d:\n", id);
    gen(node->lhs->rhs);
    if (!node->lhs->rhs)
    {
      printf("  push 1\n");
    }
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .Lend%d\n", id);
    gen(node->rhs->rhs);
    gen(node->rhs->lhs);
    printf("  jmp .Lbegin%d\n", id);
    printf("  .Lend%d:\n", id);
    return;
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
  }

  gen(node->lhs);
  gen(node->rhs);
  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind)
  {
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
    printf("  idiv rax, rdi\n");
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

  printf("push rax\n");
}
