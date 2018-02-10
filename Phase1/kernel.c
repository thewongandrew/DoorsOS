// kernel.c, 159
// OS bootstrap and kernel code for OS phase 1
// OS Name: Doors
// Team Name: A.W.W.W (Members: Wesley Webb, Andrew Wong) 

#include "spede.h"         // given SPEDE stuff
#include "kernel_types.h"  // kernle data types
#include "entry.h"         // entries to kernel
#include "tools.h"         // small tool functions
#include "proc.h"          // process names such as IdleProc()
#include "services.h"      // service code

int i;

// kernel data are all declared here:
struct i386_gate *IDT_p; //May change but this maybe important ... obviously
int run_pid;                       // currently running PID; if -1, none selected
pid_q_t ready_pid_q, avail_pid_q;  // avail PID and those ready to run
pcb_t pcb[PROC_NUM];               // Process Control Blocks
char proc_stack[PROC_NUM][PROC_STACK_SIZE]; // process runtime stacks

void InitKernelData(void) {        // init kernel data
   //initialize run_pid (to negative 1)
   run_pid = -1;
   //clear two PID queues
   MyBzero((char *)&ready_pid_q.q, sizeof(ready_pid_q.q));
   MyBzero((char *)&avail_pid_q.q, sizeof(avail_pid_q.q));
   //enqueue all PID numbers into the available PID queue
   for(i=0; i<Q_SIZE; i++) {   
      EnQ(i,&avail_pid_q);
   }
}

void InitKernelControl(void) {     // init kernel control
   //(similar to the timer lab)
   //locate where IDT is 'MAY CHANGE THE VARIABLE'
   IDT_p = get_idt_base();
   //show its location on target PC
   cons_printf("IDT is located at DRAM addr %x (%d).\n",(unsigned int) IDT_p, (unsigned int) IDT_p);
   //call fill_gate: fill out entry TIMER with TimerEntry
   fill_gate(&IDT_p[TIMER], (int)TimerEntry, get_cs(), ACC_INTR_GATE, 0);
   //send PIC a mask value
   
}

void ProcScheduler(void) {              // choose run_pid to load/run
   if(run_pid > 0) return; // no need if PID is a user proc

   if(ready_pid_q.size == 0) run_pid = 0;
   //get the 1st one in ready_pid_q to be run_pid
   else ready_pid_q.q[ready_pid_q.index] = run_pid; 

   //accumulate its totaltime by adding its runtime
   //and then reset its runtime to zero
   pcb[run_pid].totaltime += pcb[run_pid].runtime;
   pcb[run_pid].runtime = 0;
}

int main(void) {  // OS bootstraps
   InitKernelData();
   InitKernelControl();	

   //call NewProcService() with address of IdleProc to create it
   NewProcService(&IdleProc);
   //call ProcScheduler() to select a run_pid
   ProcScheduler();
   //call ProcLoader() with address of the trapframe of the selected run_pid
   ProcLoader(pcb[run_pid].trapframe_p);

   return 0; // compiler needs for syntax altho this statement is never exec
}

void Kernel(trapframe_t *trapframe_p) {   // kernel code runs (100 times/second)
   char key;

   //save the trapframe_p to the PCB of run_pid
   pcb[run_pid].trapframe_p = trapframe_p;  

   //call TimerService() to service the timer interrupt
   TimerService();


  /* if a key is pressed on target PC {
      get the key
      if it's 'n,' call NewProcService() to create a UserProc
      if it's 'b,' call breakpoint() to go to the GDB prompt
   }*/
   if(cons_kbhit()) {
       key = cons_getchar();
       if(key == 'n') {
            NewProcService(UserProc);
       }
       if(key == 'b') {
            breakpoint();
       }
   }

   //call ProcScheduler() to select run_pid
   //call ProcLoader() given the trapframe_p of the run_pid to load/run it
   ProcScheduler();
   ProcLoader(pcb[run_pid].trapframe_p);
}

