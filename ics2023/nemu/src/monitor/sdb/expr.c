/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum
{
  TK_NOTYPE = 256,
  TK_EQ,
  TK_NUM,
  TK_HNUM,
  TK_AND,
  TK_UNEQ,
  TK_REG,
  /* TODO: Add more token types */

};

static struct rule
{
  const char *regex;
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */
    {" +", TK_NOTYPE},   // spaces
    {"\\+", '+'},        // plus
    {"\\-", '-'},        // sub
    {"\\*", '*'},        // times
    {"\\%", '%'},        // mod
    {"\\/", '/'},        // div
    {"==", TK_EQ},       // equal
    {"\\(", '('},        // parenthesis
    {"\\)", ')'},        // parenthesis
    {"0[xX][0-9a-fA-F]+",TK_HNUM}, //hexadecimal-number
    {"\[0-9]+", TK_NUM}, // num
    {"\\$[0-9a-zA-Z]+",TK_REG} ,//REG
    {"!=",TK_UNEQ}, //unequal
    {"&&",TK_AND},//AND
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++)
  {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0)
    {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token
{
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e)
{
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0')
  {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++)
    {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
      {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;
        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        if (rules[i].token_type == TK_NOTYPE)
        {
          break;
        }

        tokens[nr_token].type = rules[i].token_type;             // 匹配到的类型
        strncpy(tokens[nr_token].str, substr_start, substr_len); // 匹配到的字符
        nr_token++;
        // switch (rules[i].token_type) {
        //   default: TODO();
        // }

        break;
      }
    }

    if (i == NR_REGEX)
    {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int p,int q){
  int s = 0;
  if (tokens[p].type != '(')
  {
    return false;
  }
  
  while (p<q)
  { 
    if (tokens[p].type == ')')
    {
      s--;
      if (s <=0 && p !=q-1)
      {
        return false;
      }
    }else if(tokens[p].type == '(') {
      s++;
    }
    p++;  
  }
  if (s != 0)
  {
    return false;
  }
  return true;
  
}

int main_op(int p, int q)
{
  int op = 0;
  while (p < q)
  {
    if (tokens[p].type == TK_NUM)
    {
      p++;
    }
    else if (tokens[p].type == '(')
    {
      p++;
      while (tokens[p].type != ')')
      {
        p++;
      }
      p++;
    }
    else
    {
      if (tokens[op].type == '+' || tokens[op].type == '-')
      {
        if (tokens[p].type == '+' || tokens[p].type == '-')
        {
          op = p;
        }
      }
      else
      {
        op = p;
      }
      p++;
    }
  }
  return op;
}

word_t eval(int p, int q)
{
  if (p > q)
  {
    return 0;
    /* Bad expression */
  }
  else if (p == q)
  {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    if (tokens[p].type == TK_NUM)
    {
      
      return atoi(tokens[p].str);
    }else if (tokens[p].type == TK_HNUM)
    {
      char *endptr;
      word_t num;
      printf("\n");
      num = strtol(tokens[p].str, &endptr, 0);
      return num; 
    }else{
      // int i = 0;
      // sscanf( tokens[p].str, "$%d",&i);
      bool success = false;
      return isa_reg_str2val(tokens[p].str,&success);
      
    }
    
  }
  else if (check_parentheses(p, q+1) == true)
  {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q-1);
  }
  else
  {
    /* We should do more things here. */
    int op = main_op(p, q+1);
    int val1 = eval(p, op - 1);
    int val2 = eval(op + 1, q);

    switch (tokens[op].type)
    {
    case '+':
      return val1 + val2;
    case '-':
      return val1 - val2;
    case '*':
      return val1 * val2;
    case '/':
      return val1 / val2;
    case '%':
      return val1 % val2;
    case TK_EQ:
      return val1 == val2;
    case TK_UNEQ:
      return val1 != val2;
    case TK_AND:
      return val1 && val2;
    default:
      assert(0);
    }
  }
  return 0;
}
word_t expr(char *e, bool *success)
{
  if (!make_token(e))
  {
    *success = false;
    return 0;
  }

  word_t val = eval(0,nr_token-1);
  *success = true;
  

  return val;
}
