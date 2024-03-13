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

  int a;
  char strtable[1280];
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

  Elf64_Shdr shdr[99];
  int count = elf_head.e_shnum; // 节头表数量
  fseek(fp, elf_head.e_shoff, SEEK_SET);
  a =fread(shdr, sizeof(Elf64_Shdr), count, fp);
  fseek(fp, shdr[elf_head.e_shstrndx].sh_offset, SEEK_SET);
  a = fread(strtable, 1, shdr[elf_head.e_shstrndx].sh_size, fp);
  printf("There are %d section headers, starting at offset 0x%04lx:\n\n", count, elf_head.e_shoff);
  puts("节头表:");
  printf("[编号]\t名称\t\t\t\t属性\t虚拟地址\t\t偏移量\t\t大小\t\t索引值\t信息\t对齐长度\t表项大小\n");
  for (int i = 0; i < count; ++i)
  {
    printf("[%02d]\t%s", i, &strtable[shdr[i].sh_name]);
    for (int j = 0; j < 20 - strlen(&strtable[shdr[i].sh_name]); ++j)
    {
      putchar(' ');
    }
    switch (shdr[i].sh_type)
    {
    case 0:
      printf("SHT_NULL\t");
      break;
    case 1:
      printf("SHT_PROGBITS");
      break;
    case 2:
      printf("SHT_SYMTAB\t");
      break;
    case 3:
      printf("SHT_STRTAB\t");
      break;
    case 4:
      printf("SHT_RELA\t");
      break;
    case 5:
      printf("SHT_HASH\t");
      break;
    case 6:
      printf("SHT_DYNAMIC\t");
      break;
    case 7:
      printf("SHT_NOTE\t");
      break;
    case 8:
      printf("SHT_NOBITS\t");
      break;
    case 9:
      printf("SHT_REL\t");
      break;
    case 10:
      printf("SHT_SHLIB\t");
      break;
    case 11:
      printf("SHT_DYNSYM\t");
      break;
    case 14:
      printf("SHT_INIT_ARRAY");
      break;
    case 15:
      printf("SHT_FINI_ARRAY");
      break;
    case 0x70000000:
      printf("SHT_LOPROC");
      break;
    case 0x7fffffff:
      printf("SHT_HIPROC");
      break;
    case 0x80000000:
      printf("SHT_LOUSER");
      break;
    case 0xffffffff:
      printf("SHT_HIUSER");
      break;
    case 0x6ffffff6:
      printf("SHT_GNU_HASH");
      break;
    case 0x6fffffff:
      printf("SHT_GNU_versym");
      break;
    case 0x6ffffffe:
      printf("SHT_GNU_verneed");
      break;
    }
    printf("\t0x%lx\t", shdr[i].sh_flags);
    printf("0x%016lx\t", shdr[i].sh_addr);
    printf("0x%08lx\t", shdr[i].sh_offset);
    printf("%4lu bytes\t", shdr[i].sh_size);
    printf("%u\t", shdr[i].sh_link);
    printf("%u\t", shdr[i].sh_info);
    printf("%2lu bytes\t", shdr[i].sh_addralign);
    printf("%4lx\n", shdr[i].sh_entsize);
  }
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
