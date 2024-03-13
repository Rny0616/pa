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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint
{
  int NO;
  struct watchpoint *next;
  char expr_str[32];
  word_t val;
  /* TODO: Add more members if necessary */

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool()
{
  int i;
  for (i = 0; i < NR_WP; i++)
  {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

int new_wp(char *s)
{
  if (free_ == NULL)
  {
    assert(0);
  }
  bool success = false;
  WP *wp = free_;
  strcpy(wp->expr_str, s);
  wp->val = expr(s, &success);
  free_ = free_->next;
  wp->next = head;
  head = wp;
  return 0;
};

void free_wp(int no)
{
  WP *temp = head;
  while (temp != NULL )
  {
    if (temp->NO == no)
    {
      break;
    }
    
    temp = temp->next;
  }
  temp->next = free_;
  free_ = temp;
  temp->expr_str[0] = '\0';

};

int scan_wp()
{
  WP *temp = head;
  bool success = false;
  word_t temp_val = 0;
  while (temp != NULL)
  {
    printf("%d : %s\n", temp->NO, temp->expr_str);
    temp_val = expr(temp->expr_str, &success);
    if (temp_val != temp->val)
    {
      return 0;
    }
    temp = temp->next;
  }
  return 1;
}

void info_w()
{
  WP *temp = head;
  while (temp != NULL)
  {
    printf("NUM:%d \t EXPR:%s \t VAL:%ld\n", temp->NO, temp->expr_str,temp->val);
    
    temp = temp->next;
  }
}