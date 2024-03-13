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

#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>
#include <../src/monitor/sdb/sdb.h>
#include <elf.h>
/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 10

CPU_state cpu = {};
uint64_t g_nr_guest_inst = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;

char ring_buffer[20][128];
int ring_buffer_no = 0;

// ftrace
#ifdef CONFIG_FTRACE
// 解析elf
void elf_parse(char *file)
{
  FILE *fp;
  fp = fopen(file, "r");
  if (NULL == fp)
  {
    printf("fail to open the file");
    exit(0);
  }
  // 解析head
  Elf64_Ehdr elf_head;
  // int shnum, a;
  int a;

  a = fread(&elf_head, sizeof(Elf64_Ehdr), 1, fp); // fread参数1：读取内容存储地址，参数2：读取内容大小，参数3：读取次数，参数4：文件读取引擎
  if (0 == a)
  {
    printf("fail to read head\n");
    exit(0);
  }
  if (elf_head.e_ident[0] != 0x7F ||
      elf_head.e_ident[1] != 'E' ||
      elf_head.e_ident[2] != 'L' ||
      elf_head.e_ident[3] != 'F')
  {
    printf("Not a ELF file\n");
    exit(0);
  }
  for (int i = 0; i < EI_NIDENT; i++)
  {
    
  printf("%d",elf_head.e_ident[i]);
  }
  
  // 解析section 分配内存 section * 数量
  // Elf64_Shdr *shdr = (Elf64_Shdr *)malloc(sizeof(Elf64_Shdr) * elf_head.e_shnum);
  // if (NULL == shdr)
  // {
  //   printf("shdr malloc failed\n");
  //   exit(0);
  // }

  // // 设置fp偏移量 offset，e_shoff含义
  // a = fseek(fp, elf_head.e_shoff, SEEK_SET); // fseek调整指针的位置，采用参考位置+偏移量
  // if (0 != a)
  // {
  //   printf("\nfaile to fseek\n");
  //   exit(0);
  // }

  // // 读取section 到 shdr, 大小为shdr * 数量
  // a = fread(shdr, sizeof(Elf64_Shdr) * elf_head.e_shnum, 1, fp);
  // if (0 == a)
  // {
  //   printf("\nfail to read section\n");
  //   exit(0);
  // }

  // // 重置指针位置到文件流开头
  // rewind(fp);

  // // 将fp指针移到 字符串表偏移位置处
  // fseek(fp, shdr[elf_head.e_shstrndx].sh_offset, SEEK_SET);

  // // 第e_shstrndx项是字符串表 定义 字节 长度 char类型 数组
  // char shstrtab[shdr[elf_head.e_shstrndx].sh_size];
  // char *temp = shstrtab;

  // // 读取内容
  // a = fread(shstrtab, shdr[elf_head.e_shstrndx].sh_size, 1, fp);
  // if (0 == a)
  // {
  //   printf("\nfaile to read\n");
  // }

  // uint8_t *symtab_data=NULL, *strtab_data = NULL;
  // // int sym_len, str_len;
  // int  sym_len=0,str_len = 0;
  // // 遍历
  // for (int i = 0; i < elf_head.e_shnum; i++)
  // {
  //   temp = shstrtab;
  //   temp = temp + shdr[i].sh_name;
  //   if (strcmp(temp, ".symtab") == 0)
  //   {
  //     symtab_data = (uint8_t *)malloc(sizeof(uint8_t) * shdr[i].sh_size);
  //     fseek(fp, shdr[i].sh_offset, SEEK_SET);
  //     a = fread(symtab_data, sizeof(uint8_t) * shdr[i].sh_size, 1, fp);
  //     sym_len = shdr[i].sh_size;
  //   }
  //   else if (strcmp(temp, ".strtab") == 0)
  //   {
  //     strtab_data = (uint8_t *)malloc(sizeof(uint8_t) * shdr[i].sh_size);
  //     fseek(fp, shdr[i].sh_offset, SEEK_SET);
  //     a = fread(strtab_data, sizeof(uint8_t) * shdr[i].sh_size, 1, fp);
  //     str_len = shdr[i].sh_size;
  //   }
  //   else
  //   {
  //     continue;
  //   }
  //   // if (strcmp(temp, ".strtab") != 0) {}continue;//该section名称 .symtab .strtab
  //   // printf("节的名称: %s\n", temp);
  //   // printf("节首的偏移: %lx\n", shdr[i].sh_offset);
  //   // printf("节的大小: %lx\n", shdr[i].sh_size);
  //   // uint8_t *sign_data=(uint8_t*)malloc(sizeof(uint8_t)*shdr[i].sh_size);
  //   // // 依据此段在文件中的偏移读取出
  //   // fseek(fp, shdr[i].sh_offset, SEEK_SET);
  //   // a = fread(sign_data, sizeof(uint8_t)*shdr[i].sh_size, 1, fp);
  //   // // 显示读取的内容
  // }
  //   uint8_t *p = symtab_data;
  //   int j = 0;
  //   for (j = 0; j < sym_len; j++)
  //   {
  //     printf("%c", *p);
  //     p++;
  //   }
  //   printf("----------------------------------\n");
  //   p = strtab_data;
  //   for (j = 0; j < str_len; j++)
    // {
    //   printf("%x", *p);
    //   p++;
    // }
  return;
}
#endif

void device_update();

static void trace_and_difftest(Decode *_this, vaddr_t dnpc)
{
#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND)
  {
    log_write("%s\n", _this->logbuf);
  }
  strcpy(ring_buffer[(ring_buffer_no++) % 20], _this->logbuf);
#endif
  if (g_print_step)
  {
    IFDEF(CONFIG_ITRACE, puts(_this->logbuf));
  }

  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));

  int i = scan_wp();
  if (i == 0)
  {
    nemu_state.state = NEMU_STOP;
  }
  printf("触发了监测点\n");
}

static void exec_once(Decode *s, vaddr_t pc)
{
  s->pc = pc;
  s->snpc = pc;
  s->n_pc = 0;
  s->jar_pc = 0;
  isa_exec_once(s);
  cpu.pc = s->dnpc;
#ifdef CONFIG_ITRACE
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc;
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst.val;
  for (i = ilen - 1; i >= 0; i--)
  {
    p += snprintf(p, 4, " %02x", inst[i]);
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0)
    space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;

#ifndef CONFIG_ISA_loongarch32r
  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
              MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst.val, ilen);
#else
  p[0] = '\0'; // the upstream llvm does not support loongarch32r
#endif
#endif
#ifdef CONFIG_FTRACE

  if (s->jar_pc != 0)
  {
    printf("%lx---%lx\n", s->n_pc, s->jar_pc);
  }

#endif
}

static void execute(uint64_t n)
{
  Decode s;
  for (; n > 0; n--)
  {
    exec_once(&s, cpu.pc);
    g_nr_guest_inst++;
    trace_and_difftest(&s, cpu.pc);
    if (nemu_state.state != NEMU_RUNNING)
      break;
    IFDEF(CONFIG_DEVICE, device_update());
  }
}

static void statistic()
{
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
  if (g_timer > 0)
    Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
  else
    Log("Finish running in less than 1 us and can not calculate the simulation frequency");
}
static void print_ringbuffer()
{
  for (int i = 0; i < 20; i++)
  {
    if (*ring_buffer[i] == '\0')
    {
      return;
    }
    printf("%s\n", ring_buffer[i]);
  }
}
void assert_fail_msg()
{
  isa_reg_display();
  print_ringbuffer();
  statistic();
}
extern char elf[128];
/* Simulate how the CPU works. */
void cpu_exec(uint64_t n)
{
#ifdef CONFIG_FTRACE
  elf_parse(elf);
#endif
  g_print_step = (n < MAX_INST_TO_PRINT);
  switch (nemu_state.state)
  {
  case NEMU_END:
  case NEMU_ABORT:
    printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
    return;
  default:
    nemu_state.state = NEMU_RUNNING;
  }

  uint64_t timer_start = get_time();

  execute(n);

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state)
  {
  case NEMU_RUNNING:
    nemu_state.state = NEMU_STOP;
    break;

  case NEMU_END:
  case NEMU_ABORT:
    Log("nemu: %s at pc = " FMT_WORD,
        (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) : (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) : ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
        nemu_state.halt_pc);
    // fall through
  case NEMU_QUIT:
    statistic();
  }
}
