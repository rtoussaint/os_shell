#include "dsh.h"
#include "time.h" //should go in header, but not sure if header is being turned in

void seize_tty(pid_t callingprocess_pgid); /* Grab control of the
terminal for the calling process pgid.  */
void continue_job(job_t *j); /* resume a stopped job */
void spawn_job(job_t *j, bool fg); /* spawn a new job */
void jobs();
void write_error();
int io_handler(char* file, char** argv, int inOutBit);
void* initialize_process(job_t* j, process_t* p, int input, int output);
char* build_path(process_t* p);
char* check_job_status(job_t* job);
void reapZombies();
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

    /* also establish child process group in child to avoid race (if
parent has not done it yet). */
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

void spawn_job(job_t *j, bool fg){
    pid_t pid; //process id
    process_t *p; //structure for a process
    int input = STDIN_FILENO;
    int output = STDOUT_FILENO;
    int pipefd[2];


    for(p = j->first_process; p; p = p->next) {
        /* YOUR CODE HERE? */
        /* Builtin commands are already taken care earlier */
        if(p->next != NULL) {
            pipe(pipefd);
            output = pipefd[1];
        }
        else {
            output = STDOUT_FILENO;
        }

        switch (pid = fork()) { //create new process with fork


            case -1: /* fork failure */
                perror("fork");
                exit(EXIT_FAILURE);
                break;
            case 0: /* child process  */
                p->pid = getpid();  //get id from the new child
                new_child(j, p, fg);

                if(p == j->first_process){

                    if(p->ifile != NULL){
                        io_handler(p->ifile, p->argv, STDIN_FILENO);
                    }
                    else{
                        input = STDIN_FILENO;
                    }

                }
                if(p->next == NULL){
                    if (p->ofile != NULL) {
                        io_handler(p->ofile, p->argv, STDOUT_FILENO);
                    }
                    else{
                        output = STDOUT_FILENO;
                    }
                }

                initialize_process(j, p, input, output);
                return;
                break;
            default:
                p->pid = pid;
                set_child_pgid(j, p);
                

                wait(NULL);

         





                if (p->next != NULL) {
                    close(pipefd[1]);
                }

                break;
        }





        if (input != STDIN_FILENO) {
            close(input);
        }

        if (output != STDOUT_FILENO) {
            close(output);
        }
        input = pipefd[0];
    }

        int status;

        waitpid(pid, NULL, WUNTRACED);






        seize_tty(getpid());
}

void* initialize_process(job_t* j, process_t* p, int input, int output){
        char* path_to_execute = build_path(p);

        char * test = path_to_execute;
        //DEBUG("%s", path_to_execute);
       // printf("%d ------- %d\n",input, output);
        if(endswith(p->argv[0], ".c")){
            p->argv[0] = "./devil";
        }


        if (input != STDIN_FILENO) {
            //DEBUG("pipe in");
            dup2(input, STDIN_FILENO);
            close(input);
        }
        if (output != STDOUT_FILENO) {
            //DEBUG("pipe out");
            dup2(output, STDOUT_FILENO);
            close(output);
        }
       execvp(p->argv[0], p->argv);
}

char* build_path(process_t* p){

        if(endswith(p->argv[0], ".c")){
            char *variables[] = {"gcc", "-o", "devil", p->argv[0], NULL};
            
            int myI; 
            myI = fork();
            if(myI < 0){
                exit(EXIT_FAILURE);
            }
            if(myI == 0){
                execvp("gcc", variables);
            }
            if(myI > 0){
                waitpid(myI, NULL, 0);
            }
        }
        else{
          return p->argv[0];
        }

}

/* Sends SIGCONT signal to wake up the blocked job */
void continue_job(job_t *j) {
        if(kill(-j->pgid, SIGCONT) < 0)
            perror("kill(SIGCONT)");
}


    /*
     * builtin_cmd - If the user has typed a built-in command then execute
     * it immediately.
     */
bool builtin_cmd(job_t *last_job, int argc, char **argv) {

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
            reapZombies();
            jobs(jobptr);
            isBuiltIn = true;
            return true;
        }
        else if (!strcmp("cd", argv[0])) {
            if(argc == 2) {
                chdir(argv[1]);
                isBuiltIn = true;
            }
        }
        else if (!strcmp("bg", argv[0])) {
            isBuiltIn = true;
            /* Your code here */
              job_t* current = jobptr;
              while(current != NULL) {
                  //argv[1] is a pointer to the string that describes the pgid -- need to cast (atoi)
                  if (current->pgid == atoi(argv[1])) {
                      continue_job(current);
                      //make argv[1] negative so that any process from the job will finish
                      //for loop for every process in the job

                      return true;
                  }
                  else {
                      current = current->next;
                  }

              }
        }
        else if (!strcmp("fg", argv[0])) {
            isBuiltIn = true;
            /* Your code here */
              job_t* current = jobptr;
              while(current != NULL) {
                  //argv[1] is a pointer to the string that describes the pgid -- need to cast (atoi)
                  if (current->pgid == atoi(argv[1])) {
                      continue_job(current);
                      //make argv[1] negative so that any process from the job will finish
                      //for loop for every process in the job

                      process_t* current_process = current->first_process;
                      while (current_process != NULL) {
                          waitpid((atoi(argv[1])*(-1)), &(current_process->status), WUNTRACED);
                          current_process = current_process->next;
                      }
                      return true;
                  }
                  else {
                      current = current->next;
                  }

              }
              seize_tty(getpid());
        }
        return false;       /* not a builtin command */
}

    /* Build prompt messaage */
char* promptmsg() {
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

int main() {
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

            /* Only for debugging purposes to show parser output; turn
off in the
             * final code */
            //if(PRINT_INFO) print_job(j);

            /* Your code goes here */
                //add this job to the job list
    if(jobptr == NULL){
        jobptr = j; //point to the new job
    }
    else{
        job_t* lastJPtr = find_last_job(jobptr);
        lastJPtr->next = j; //add to the end
    }

    job_t* job_temp = j;
            /* You need to loop through jobs list since a command line
can contain ;*/
                while(job_temp != NULL) {
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
                        if(j->bg){
                            spawn_job(job_temp, false);  
                        }
                        else{
                            spawn_job(job_temp, true);

                        }
                    }

                    isBuiltIn = false;
                    job_temp = job_temp->next;
            }
        }
    }

    char* check_job_status(job_t* job) {
      process_t* current_process = job->first_process;
      while(current_process != NULL) {
        int status = waitpid(current_process->pid, &(current_process->status), WNOHANG);

        if (status == -1) {
          current_process->completed = true;
          return "Completed";
        }

        if (WIFEXITED(status)) {
          current_process->completed = false;
 return "Stopped";
          //return "Terminated normally";
        }
        else if (WIFSIGNALED(status)) {
          int terminationSignal = WTERMSIG(status);
          char first[100];
          //char* first = (char*)malloc(100);
          strcat(first,"Terminated by signal");
          char str[15];
          sprintf(str, "%d", terminationSignal);
          strcat(first, str);
          return first;
        }
        else if (WIFSTOPPED(status)) {
          current_process->completed = false;
          return "Stopped";
        }
        current_process = current_process->next;

      }
      return NULL;

    }


    void reapZombies() {
        job_t* current = jobptr;
        job_t* prev = NULL;
        while (current != NULL) {
            if (job_is_completed(current)) {
              if (prev == NULL) {
                current = current->next;
                jobptr = current;
              }
              else {
                prev->next = current->next;
                //delete_job(current, jobptr);
                current = current->next;
              }
            }
            else {
              prev = current;
              current = current->next;
            }
        }
    }

    void jobs(job_t* myJob){
      job_t* curr = myJob;

      if (curr != NULL) {
        printf("\033[1;32mCURRENT JOBS\033[0m\n");
        printf("PID\tSTATUS\t\tNAME\n");

          while(curr != NULL) {
              int stat = (curr->first_process)->status;
              printf("%d \t", (int) curr->pgid);
              printf("%s \t", check_job_status(curr));
              printf("%s \n", curr->commandinfo);
              curr = curr->next;
        }
      }
      else {
          printf("No current jobs\n");
      }
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
            return 0;
        }
        //if it output >
        else if (inOutBit == 1) {
            fileDes = open(file, O_APPEND | O_WRONLY | O_CREAT, 0777);
            dup2(fileDes, STDOUT_FILENO); //redirecting the standout to fileDes
            return 0;
        }
        else {
            //error case
            return 1;
        }
        execvp(argv[0], argv);
        close(fileDes);
        return 0;
    }