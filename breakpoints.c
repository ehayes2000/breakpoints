#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <elf.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/time.h>


size_t get_process_base_address(pid_t);
size_t get_pc(pid_t);
void debug_region(pid_t, size_t, size_t);
void the_joker(){
  printf("I'm going to break open arkham asylum and no one can stop me!\n");
  printf("collecting cards ...\n");
  printf("gathering cronies...\n");
  printf("plotting route...\n");
  printf("loading unmarked van...\n");
  printf("driving to asylum...\n");
  printf("melting gate with acid...\n");
  printf("escaping with inmates...\n");
  printf("we live in a society!!!\n");
}

int main(){
  // create child process
  pid_t child = fork();
  if (child == 0){
    if (ptrace(PTRACE_TRACEME, 0, (void*)NULL, (void*)NULL) < 0){
      perror("ptrace(TRACEME)");
      exit(errno);
    }
    raise(SIGSTOP);
    the_joker();
    exit(0);
  }
  printf("the joker pid %lu\n", child);
  // if our parent process exits kill the child process 
  ptrace(PTRACE_SETOPTIONS, child, (void*)NULL, PTRACE_O_EXITKILL);
  int status;
  waitpid(child, &status, 0);
  if (!WIFSTOPPED(status)){
    fprintf(stderr, "whats he up to??");
    exit(1);
  }
  printf("finding lair...\n");
  size_t base_address = get_process_base_address(child);
  printf("decyphering plans...\n");
  long breakpt = 0xd4200000;
  size_t offset = 0xc4c; 
  printf("placing tracker on unmarked van...\n\n");

  long original = ptrace(PTRACE_PEEKTEXT, child, offset + base_address, NULL);
  long break_instn = (0xffffffff00000000 & original) | breakpt;
  ptrace(PTRACE_POKETEXT, child, offset + base_address, break_instn);
  ptrace(PTRACE_CONT, child, NULL, NULL);
  waitpid(child, &status, 0);
  size_t pc_offset = get_pc(child) - base_address;
  if (WIFSTOPPED(status)){
    int signal = WSTOPSIG(status);
    if (signal == SIGTRAP){
      printf("we got him [%p]\n", pc_offset);
    } else { 
      fprintf(stderr, "we lost him\n");
      exit(2);
    }
  } else { 
    fprintf(stderr,"we lost him\n");
    exit(3);
  }
  printf("he's escaping\n\n");
  ptrace(PTRACE_POKETEXT, child, offset + base_address, original);
  ptrace(PTRACE_SINGLESTEP, child, NULL, NULL);
  waitpid(child, &status, 0);
  ptrace(PTRACE_POKETEXT, child, offset + base_address, break_instn);
  ptrace(PTRACE_CONT, child, NULL, NULL);

  if (waitpid(child, &status, 0) == -1){
    perror("failed to wait");
    exit(errno);
  }
  if (WIFEXITED(status)){
    printf("the joker died\n");
    exit(0);
  }
  else if (WIFSTOPPED(status)){
    int signal = WSTOPSIG(status);
    size_t npc = get_pc(child);
    printf("stopped %p -> %p %s\n", npc, npc - base_address, strsignal(signal));
    debug_region(child, npc, base_address);
    exit(1);
  }
  // we could check for more conditions herre
  printf("something unexpected happened\n");
  exit(1);
}


// I have to believe there's a better way of doing this but this
// This is the way I found and its been working well 
size_t get_process_base_address(pid_t pid) {
    char filename[32];
    sprintf(filename, "/proc/%d/maps", pid);
    
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Failed to open memory map");
        exit(1);
    }
    size_t base_address;
    if (fscanf(fp, "%lx", &base_address) != 1) {
        fprintf(stderr, "Failed to read base address\n");
        exit(1);
    }
    fclose(fp);
    return base_address;
}

size_t get_pc(pid_t pid) {
  struct user_regs_struct regs;  
  struct iovec iov;
  iov.iov_base = &regs;
  iov.iov_len = sizeof(regs);
  ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &iov);
  return (size_t)regs.pc;
}

// print memory n above and below given pc
// show addresses as offsets
void debug_region(pid_t child, size_t pc, size_t base_address){
  long instn;
  int n = 2;
  printf("\n");
  for (int i = -n; i <= n; i ++){
    instn = ptrace(PTRACE_PEEKTEXT, child, pc + 4*i, NULL);
    printf("[%p]  %lx    %d\n", (pc+ 4*i) - base_address, (uint32_t)instn, i);
  }
  printf("\n");
}