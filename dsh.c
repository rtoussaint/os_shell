#include "dsh.h"

void seize_tty(pid_t callingprocess_pgid); /* Grab control of the terminal for the calling process pgid.  */
void continue_job(job_t *j); /* resume a stopped job */
void spawn_job(job_t *j, bool fg); /* spawn a new job */
void jobs();
void compile_string(process_t* p);

job_t* jobptr;

/* Sets the process group id for a given job and process */
int set_child_pgid(job_t *j, process_t *p)
{
    if (j->pgid < 0) /* first child: use its pid for job pgid */
  j->pgid = p->pid;
  return(setpgid(p->pid,j->pgid));
}

/* Creates the context for a new child by setting the pid, pgid and tcsetpgrp */
void new_child(job_t *j, process_t *p, bool fg)
{
         /* establish a new process group, and put the child in
          * foreground if requested
          */

         /* Put the process into the process group and give the process
          * group the terminal, if appropriate.  This has to be done both by
          * the dsh and in the individual child processes because of
          * potential race conditions.  
          * */

          p->pid = getpid();

         /* also establish child process group in child to avoid race (if parent has not done it yet). */
          set_child_pgid(j, p);

         if(fg) // if fg is set
    seize_tty(j->pgid); // assign the terminal

         /* Set the handling for job control signals back to the default. */
 signal(SIGTTOU, SIG_DFL);
}

/* Spawning a process with job control. fg is true if the 
 * newly-created process is to be placed in the foreground. 
 * (This implicitly puts the calling process in the background, 
 * so watch out for tty I/O after doing this.) pgid is -1 to 
 * create a new job, in which case the returned pid is also the 
 * pgid of the new job.  Else pgid specifies an existing job's 
 * pgid: this feature is used to start the second or 
 * subsequent processes in a pipeline.
 * */

 void spawn_job(job_t *j, bool fg) 
 {

  pid_t pid; //
  process_t *p;

  //add this job to the job list
  if(jobptr == NULL){
    jobptr = j;
  }
  else{
    job_t* lastJPtr = find_last_job(jobptr);
    lastJPtr->next = j;
  }


  for(p = j->first_process; p; p = p->next) {

    /* YOUR CODE HERE? */
    /* Builtin commands are already taken care earlier */

   switch (pid = fork()) {

          case -1: /* fork failure */
    perror("fork");
    exit(EXIT_FAILURE);

          case 0: /* child process  */
    p->pid = getpid();      
    new_child(j, p, fg);

            //Need to compile C Program
    if(endswith(p->argv[0],".c")) {
      compile_string(p);

    }
    else {
      execvp(p->argv[0], p->argv);
         /* YOUR CODE HERE?  Child-side code for new process. */
      perror("New child should have done an exec");
              /* NOT REACHED */  
    }

    exit(EXIT_FAILURE);
    break;
    default: /* parent */
            /* establish child process group */
    wait(NULL);
    p->pid = pid;
    set_child_pgid(j, p); 
    if(endswith(p->argv[0],".c")) {
      pid_t test;
      switch(test = fork()) {
        case -1:
          exit(EXIT_FAILURE);
        case 0:
            execvp("./devil", p->argv);
        default:
          wait(NULL);
      }

    }
     /* YOUR CODE HERE?  Parent-side code for new process.  */

  }

     /* YOUR CODE HERE?  Parent-side code for new job.*/
      seize_tty(getpid()); // assign the terminal back to dsh

   }
 }

/* Sends SIGCONT signal to wake up the blocked job */
 void continue_job(job_t *j) 
 {
   if(kill(-j->pgid, SIGCONT) < 0)
    perror("kill(SIGCONT)");
}


/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 * it immediately.  
 */
 bool builtin_cmd(job_t *last_job, int argc, char **argv) 
 {

      /* check whether the cmd is a built in command
        */

  if (!strcmp(argv[0], "quit")) {
            /* Your code here */
          //free jobs here
    exit(EXIT_SUCCESS);
  }
  else if (!strcmp("jobs", argv[0])) {
            /* Your code here */
            //jobs(argv[0]);
    return true;
  }
  else if (!strcmp("cd", argv[0])) {
          chdir(argv[1]); //test this
  }
  else if (!strcmp("bg", argv[0])) {
            /* Your code here */
  }
  else if (!strcmp("fg", argv[0])) {
            /* Your code here */
  }
  else if(!strcmp(">", argv[0])){
      last_job->mystdout = argv[1];
  }
        return false;       /* not a builtin command */
}

/* Build prompt messaage */
char* promptmsg() 
{
    /* Modify this to include pid */
  //char* returnStr = "RyanRyan dsh %d $ ", 
 int my_pid = getpid();
 char my_prompt[50];
 strcpy(my_prompt, "dsh ");
 char convertToString[15];
 
 sprintf(convertToString, "%d", my_pid);

 strcat(my_prompt, convertToString);
 strcat(my_prompt, " $");
  return my_prompt;
  //return "dsh $ ";
}

int main() 
{

  init_dsh();
  DEBUG("Successfully initialized\n");

  while(1) {
    job_t *j = NULL;
    if(!(j = readcmdline(promptmsg()))){
      if (feof(stdin)) { /* End of file (ctrl-d) */
     fflush(stdout);
     printf("\n");
     exit(EXIT_SUCCESS);
      }
      continue; /* NOOP; user entered return or spaces with return */
    }

    /* Only for debugging purposes to show parser output; turn off in the
     * final code */
      if(PRINT_INFO) print_job(j);
      
    /* Your code goes here */
    /* You need to loop through jobs list since a command line can contain ;*/
    /* Check for built-in commands */
      process_t* temp = j->first_process;
      int argc_temp = temp->argc;
      char** argv_temp = temp->argv;

      builtin_cmd(j, argc_temp, argv_temp);
    /* If not built-in */
        /* If job j runs in foreground */
        /* spawn_job(j,true) */
        /* else */
        /* spawn_job(j,false) */
      spawn_job(j, false);

  }
}


void jobs(job_t* myJob){

  while(myJob->next != NULL){
    printf("%s \n", myJob->commandinfo);
    int stat = (myJob->first_process)->status;
    printf("%d \n", stat);
    myJob = myJob->next;
  }

  /* if(p->completed){
    delete_job(j, jobptr);
    job_t* myjob = readcmdline(promptmsg());
  }
  */

}

void compile_string(process_t* p){
  char example[MAX_LEN_FILENAME];
  strcpy(example, "gcc ");
  strcat(example, p->argv[0]);
  strcat(example, " -o devil");       
  execvp(example, NULL);
}












