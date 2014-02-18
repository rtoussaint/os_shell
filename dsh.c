#include "dsh.h"
#include "time.h" //should go in header, but not sure if header is being turned in

void seize_tty(pid_t callingprocess_pgid); /* Grab control of the terminal for the calling process pgid.  */
void continue_job(job_t *j); /* resume a stopped job */
void spawn_job(job_t *j, bool fg); /* spawn a new job */
void jobs();
void compile_string(process_t* p);
void write_error();
int io_handler(char* file, char** argv, int inOutBit);
void pipelining(char** argv, int argc);

job_t* jobptr;
bool isBuiltIn;

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
    else if (p->ofile != NULL) {
      io_handler(p->ofile, p->argv, 1);
    }
    else if(p->ifile != NULL) {
      io_handler(p->ifile, p->argv, 0);
    }
    else if (p->next != NULL) {
      printf("pipe here\n");
      pipelining(p->argv, p->argc, p->next);
    }
    else {
      struct stat s;
      char path[100];
      strcat(path, "/usr/bin/");
      strcat(path, p->argv[0]);
      if( stat(path,&s) == 0 ) {
        execvp(path, p->argv);
      }
      else {
        execvp(p->argv[0], p->argv);
        char commandName[500];
        strcpy(commandName, p->argv[0]);
        strcat(commandName, " is an invalid command, new child should have done an exec");
                                    
        write_error(commandName);
        perror(commandName);
        //error
      }

      
      /*
      execvp(p->argv[0], p->argv);
         // YOUR CODE HERE?  Child-side code for new process.

      //error handling
      char commandName[500];
      strcpy(commandName, p->argv[0]);
      strcat(commandName, " is an invalid command, new child should have done an exec");
      
      write_error(commandName);
      perror(commandName);
              // NOT REACHED  
      */
      input = pipe[0];

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

    //Make a breakpoint in the log file.
    FILE *file = fopen("dsh.log", "ab+");
    time_t logTime;
    logTime = time(NULL);
    const char *text = "\n****End of Session ****";
    fprintf(file, "%s\t\t%s\n\n", text, asctime( localtime(&logTime)) );
    isBuiltIn = true;
    exit(EXIT_SUCCESS);
  }
  else if (!strcmp("jobs", argv[0])) {
		jobs(jobptr);
            /* Your code here */
            //jobs(argv[0]);
    return true;
  }
  else if (!strcmp("cd", argv[0])) {
        if(argc == 2) {
           chdir(argv[1]); 
           isBuiltIn = true;
        }
  }
  else if (!strcmp("bg", argv[0])) {
            /* Your code here */
  }
  else if (!strcmp("fg", argv[0])) {
            /* Your code here */
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
 strcat(my_prompt, " $ ");
  return my_prompt;

}

int main() 
{
  init_dsh();
  DEBUG("Successfully initialized\n");
  remove("dsh.log");
  //remove_log();
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
      //if(PRINT_INFO) print_job(j);
      
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
      if(!isBuiltIn) {
        spawn_job(j, false);
      }

        isBuiltIn = false;
  }
}


void jobs(job_t* myJob){

  if (myJob != NULL) {
	printf("\033[1;32mCURRENT JOBS\033[0m\n");
	printf("PID\tSTATUS\tNAME\n");

	while(myJob != NULL) {
		int stat = (myJob->first_process)->status;
		printf("%d \t", (int) myJob->pgid);
		printf("%d \t", stat);
		printf("%s \n", myJob->commandinfo);
		myJob = myJob->next;
	}
  }
  else {
	printf("No current jobs\n");
  } 
  /* if(p->completed){
    delete_job(j, jobptr);
    job_t* myjob = readcmdline(promptmsg());
  }
  */

}

void reapZombieProcesses() {

//waitpid()

}

void write_error(char* errorMsg, char** argv) {
  FILE *file = fopen("dsh.log", "ab+");
  time_t logTime;
  logTime = time(NULL);
  fprintf(file, "%s\t\t%s\n", errorMsg, asctime( localtime(&logTime)) );

}

/*
 * Sends output of process to file or input of file to process
 * int inOutBit - 1 is for output files, 0 is for input files
 */
int io_handler(char* file, char** argv, int inOutBit) {
  
  int fileDes;
  //if its input <
  if (inOutBit == 0) {
    fileDes = open(file, O_RDONLY);
    dup2(fileDes, STDIN_FILENO); //pointing 0 at fileDes
  }
  //if it output >
  else if (inOutBit == 1) {
   fileDes = open(file, O_APPEND | O_WRONLY | O_CREAT, 0777); 
   dup2(fileDes, STDOUT_FILENO); //redirecting the standout to fileDes
  }
  else {
    //error case
    return 1;
  }                  
  execvp(argv[0], argv);
  close(fileDes);
  return 0;
}

void compile_string(process_t* p){
  char example[MAX_LEN_FILENAME];
  strcpy(example, "gcc ");
  strcat(example, p->argv[0]);
  strcat(example, " -o devil");       
  execvp(example, NULL);
}






