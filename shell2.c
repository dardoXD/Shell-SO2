#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <fcntl.h>

void printJobs()
{
    printf("\nActive jobs:\n");
    printf("---------------------------------------------------------------------------\n");
    printf("| %7s  | %30s | %5s | %10s | %6s |\n", "job no.", "name", "pid","descriptor", "status");
    printf("---------------------------------------------------------------------------\n");
    t_job* job = jobsList;
    if (job == NULL) {
        printf("| %s %62s |\n", "No Jobs.", "");
    } else {
        while (job != NULL) {
            printf("|  %7d | %30s | %5d | %10s | %6c |\n", job->id, job->name,job->pid, job->descriptor, job->status);
            job = job->next;
        }
    }
    printf("---------------------------------------------------------------------------\n");
}


void handle_signal(int signo)
{
	printf("\n[G7] ");									//Verifica sinal
	fflush(stdout);
}

void fill_argv(char *tmp_argv)										//Preenche my_argv com tmp
{
	char *foo = tmp_argv;
	int index = 0;
	char ret[TAMANHO_MAXIMO];
	bzero(ret, TAMANHO_MAXIMO);
	while(*foo != '\0') {												//Enquanto não acha \0
		if(index == 10)
			break;

		if(*foo == ' ') {
			if(my_argv[index] == NULL)										//Se for null, aloca memoria
				my_argv[index] = (char *)malloc(sizeof(char) * strlen(ret) + 1);
			else {
				bzero(my_argv[index], strlen(my_argv[index]));				//Completa com zero
			}
			strncpy(my_argv[index], ret, strlen(ret));						//Copia ret para my_argv
			strncat(my_argv[index], "\0", 1);								//Concatena o \0
			bzero(ret, TAMANHO_MAXIMO);
			index++;
		} else {
			strncat(ret, foo, 1);
		}
		foo++;
	}
	my_argv[index] = (char *)malloc(sizeof(char) * strlen(ret) + 1);
	strncpy(my_argv[index], ret, strlen(ret));
	strncat(my_argv[index], "\0", 1);
}

void copy_envp(char **envp)
{
	int index = 0;
	for(;envp[index] != NULL; index++) {									//Percorre envp
		my_envp[index] = (char *)malloc(sizeof(char) * (strlen(envp[index]) + 1));
		memcpy(my_envp[index], envp[index], strlen(envp[index]));			//Copia da memória
	}
}

void get_path_string(char **tmp_envp, char *bin_path)				//Pega a string do path
{
	int count = 0;
	char *tmp;
	while(1) {
		tmp = strstr(tmp_envp[count], "PATH=/usr/local");			//Pega a primeira ocorrencia de /usr/local
		if(tmp == NULL) {
			count++;
		} else {
			break;
		}
	}
    strncpy(bin_path, tmp, strlen(tmp));							//Copia para string path
}

void insert_path_str_to_search(char *path_str) 
{
	int index=0;
	char *tmp = path_str;
	char ret[TAMANHO_MAXIMO];
	while(*tmp != '='){													//Verifica tmp com =
		tmp++;
	}
	tmp++;
	search_path[index] = (char *) malloc(sizeof(char) * 300);
	while(*tmp != '\0') {												//Enquanto for diferente de \0
		if(*tmp != ':'){
			strncat(search_path[index], tmp, 1);						//Concatena path com tmp
			tmp++;
		}
		else{
			tmp++;
			index++;													//anda com index e tmp
			search_path[index] = (char *) malloc(sizeof(char) * 300);
		}
	}
}

int attach_path(char *cmd)
{
	char ret[TAMANHO_MAXIMO];
	int index = 0;
	int fd;
	bzero(ret, TAMANHO_MAXIMO);
	for(index=0;search_path[index]!=NULL;index++) {						//Anda na string path
		strcpy(ret, search_path[index]);								//Copia para ret
		strcat(ret, "/");												//Concatena /
		strcat(ret, cmd);												//Concatena comando
		fd = open(ret, O_RDONLY);
		if((fd = open(ret, O_RDONLY)) > 0) {							//Testa arquivo
			strncpy(cmd, ret, strlen(ret));								//Copia ret para comando
			close(fd);													//Fecha arquivo
			return 0;
		}
	}
	return 0;
}

int Verifica_Comandos(){									//Verifica se é um comando interno
	if(strcmp("cd", my_argv[0]) == 0){

        	printf("Entrou\n");
		if (my_argv[1] == NULL) {
            chdir(getenv("HOME"));							//Muda o diretório para Home
        } 
        else {
            if (chdir(my_argv[1]) == -1) {					//Não achou o diretório executado
                printf(" %s: no such directory\n", my_argv[1]);
            }
        }
        return 1;
	}

	if (strcmp("bg", my_argv[0]) == 0) {
            if (my_argv[1] == NULL)
                    return 0;
            if (strcmp("in", my_argv[1]) == 0)
                    call_execve(my_argv[0] + 3,  BACKGROUND);
            else if (strcmp("out", my_argv[1]) == 0)
                    call_execve(my_argv[0] + 3, BACKGROUND);
            else
                    call_execve(my_argv[0] + 1, BACKGROUND);
            return 1;
    }

    if (strcmp("fg", my_argv[0]) == 0) {
            if (my_argv[1] == NULL)
                    return 0;
            int jobId = (int) atoi(my_argv[1]);
            t_job* job = getJob(jobId);
            if (job == NULL)
                    return 0;
            if (job->status == SUSPENDED || job->status == WAITING)
                    putJobForeground(job, TRUE);
            else                                                                                                // status = BACKGROUND
                    putJobForeground(job, FALSE);
            return 1;
    }

	if (strcmp(my_argv[0], "pwd") == 0) {					//Mostra o diretório atual
		getcwd(dir, 4096);									//Pega o diretório e coloca na variavel
		printf("%s\n", dir);
		return 1;
	}

	if (strcmp("jobs", my_argv[0]) == 0) {
        printJobs();									//Imprime a tabela de processos
        return 1;
    }

    return 0;
}

t_job* getJob(int valor)						//Pega o job através do valor do pid
{
        t_job* job = jobsList;
        while (job != NULL) {
	        if (job->pid == valor)				//Verifica valor do job	
                return job;
            else
                job = job->next;				//Anda o ponteiro
    	}
    	return NULL;
}

t_job* delJob(t_job* job)
{

        if (jobsList == NULL)					//Se lista vazia, retorna NULL
            return NULL;

        t_job* JobAtual;
        t_job* ProximoJob;

        JobAtual = jobsList->next;
        ProximoJob = jobsList;

        if (ProximoJob->pid == job->pid) {					//Procura no primeiro job da
        													//lista o job para ser deletado
            ProximoJob = ProximoJob->next;
            numActiveJobs--;								//Diminui o tamanho da lista
            return JobAtual;
        }

        while (JobAtual != NULL) {							//Procura no resto da lista		
            if (JobAtual->pid == job->pid) {
                numActiveJobs--;
                ProximoJob->next = JobAtual->next;
            }
            ProximoJob = JobAtual;
            JobAtual = JobAtual->next;
        }
        return jobsList;
}

t_job* insertJob(pid_t pid, pid_t pgid, char* name, int status)
{
        t_job *newJob = malloc(sizeof(t_job));					//Aloca memória para o novo job

        newJob->name = (char*) malloc(sizeof(name));
        newJob->name = strcpy(newJob->name, name);
        newJob->pgid = pgid;
        newJob->pid = pid;
       	newJob->status = status;
        newJob->next = NULL;									//Atribui valores para os jobs

        if (jobsList == NULL) {									//Adiciona o job na ultima
        														//posição da lista
            numActiveJobs++;
            newJob->id = numActiveJobs;
            return newJob;
        } else {
            t_job *auxNode = jobsList;
            while (auxNode->next != NULL) {
                auxNode = auxNode->next;
            }
            newJob->id = auxNode->id + 1;
            auxNode->next = newJob;
            numActiveJobs++;
            return jobsList;
        }
}

void waitJob(t_job* job)
{
        int terminationStatus;
        while (waitpid(job->pid, &terminationStatus, WNOHANG) == 0) {		//Espera ate o status mudar
            if (job->status == SUSPENDED)									//Porem o WNOHANG faz com que
                return;														//o processo pai não pare
        }
        jobsList = delJob(job);												//Deleta o job da lista
}

void putJobForeground(t_job* job, int continueJob)
{
    job->status = FOREGROUND;											//Altera o status para FOREGROUND = 0
    tcsetpgrp(TERMINAL, job->pgid);
    if (continueJob) {													//Enquanto o job está ativo
        if (kill(-job->pgid, SIGCONT) < 0)								//Erro no sinal do job
            perror("kill (SIGCONT)");
    }
    waitJob(job);														//Espera o job
    tcsetpgrp(TERMINAL, PGID);
}

void putJobBackground(t_job* job, int continueJob)
{
	int status;
    if (job == NULL)													//Erro na criação do job
	    return;

    if (continueJob && job->status != WAITING)							//Job acabou
        job->status = WAITING;

    if (continueJob){
        if (kill(-job->pgid, SIGCONT) < 0)								//Erro no sinal do job
            perror("kill (SIGCONT)");
    	
    }

    tcsetpgrp(TERMINAL, PGID);
}

int changeJobStatus(int pid, int status)
{
    t_job *job = jobsList;
    if (job == NULL) {												//Erro na criação do job
        return 0;
    } 
    else {
        int counter = 0;
        while (job != NULL) {										//Percorre a lista de jobs
            if (job->pid == pid) {									//Verifica pid do job
                job->status = status;								//Atualiza o status do job
                return 0;
            }
            counter++;
            job = job->next;										//Anda com o ponteiro
        }
        return 1;
    }
}

void executeCommand(char *command[], char *file, int newDescriptor,
                    int executionMode)
{
        int commandDescriptor;
        if (newDescriptor == STDIN) {
                commandDescriptor = open(file, O_RDONLY, 0600);
                dup2(commandDescriptor, STDIN_FILENO);
                close(commandDescriptor);
        }
        if (newDescriptor == STDOUT) {
                commandDescriptor = open(file, O_CREAT | O_TRUNC | O_WRONLY, 0600);
                dup2(commandDescriptor, STDOUT_FILENO);
                close(commandDescriptor);
        }
        if (execvp(*command, command) == -1)
                perror("MSH");
}

void launchJob(char *command[], char *file, int newDescriptor,
               int executionMode)
{
        pid_t pid;
        pid = fork();
        switch (pid) {
        case -1:
                perror("MSH");
                exit(EXIT_FAILURE);
                break;
        case 0:
                signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                signal(SIGCHLD, &signalHandler_child);
                signal(SIGTTIN, SIG_DFL);
                usleep(20000);
                setpgrp();
                if (executionMode == FOREGROUND)
                        tcsetpgrp(MSH_TERMINAL, getpid());
                if (executionMode == BACKGROUND)
                        printf("[%d] %d\n", ++numActiveJobs, (int) getpid());

                executeCommand(command, file, newDescriptor, executionMode);
                exit(EXIT_SUCCESS);
                break;
        default:
                setpgid(pid, pid);

                jobsList = insertJob(pid, pid, *(command), file, (int) executionMode);

                t_job* job = getJob(pid, BY_PROCESS_ID);

                if (executionMode == FOREGROUND) {
                        putJobForeground(job, FALSE);
                }
                if (executionMode == BACKGROUND)
                        putJobBackground(job, FALSE);
                break;
        }
}



/*void call_execve(char *cmd, int executionMode)
{
	int i = 0;
	pid_t pid;

	t_job* job;
	printf("cmd is %s\n", cmd);
	if(Verifica_Comandos() == 0){										//Verifica se é um comando implementado
		pid = fork();													//Cria um processo filho
		switch (pid){
			case -1:
				exit(EXIT_FAILURE);										//Erro no fork
				break;
			case 0:
				signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                signal(SIGCHLD, &signalHandler_child);
                signal(SIGTTIN, SIG_DFL);
                usleep(20000);
                setpgrp();
                if (executionMode == FOREGROUND)
                        tcsetpgrp(TERMINAL, getpid());
                if (executionMode == BACKGROUND)
                        printf("[%d] %d\n", ++numActiveJobs, (int) getpid());

                call_execve(cmd, executionMode);

                exit(EXIT_SUCCESS);
                break;

				i = execve(cmd, my_argv, my_envp);						//Executa o comando
				printf("errno is %d\n", errno);
				if(i < 0) {
					printf("%s: %s\n", cmd, "command not found");
					exit(1);		
				}

				exit(0);
            	break;

            default:
            	setpgid(pid, pid);

                jobsList = insertJob(pid, pid, cmd, (int) executionMode);

                t_job* job = getJob(pid);

                if (executionMode == FOREGROUND) {
                    putJobForeground(job, FALSE);
                }
                if (executionMode == BACKGROUND)
                    putJobBackground(job, FALSE);
                break;
		}
	}
}
*/

void signalHandler_child(int p)
{
    pid_t pid;
    int terminationStatus;
    pid = waitpid(WAIT_ANY, &terminationStatus, WUNTRACED | WNOHANG);
    if (pid > 0) {
        t_job* job = getJob(pid);
        if (job == NULL)
                return;
        if (WIFEXITED(terminationStatus)) {
            if (job->status == BACKGROUND) {
                printf("\n[%d]+  Done\t   %s\n", job->id, job->name);
                jobsList = delJob(job);
            }
        } 
        else if (WIFSIGNALED(terminationStatus)) {
            printf("\n[%d]+  KILLED\t   %s\n", job->id, job->name);
            jobsList = delJob(job);
        } 
        else if (WIFSTOPPED(terminationStatus)) {
            if (job->status == BACKGROUND) {
                tcsetpgrp(TERMINAL, PGID);
                changeJobStatus(pid, WAITING);
                printf("\n[%d]+   suspended [wants input]\t   %s\n",
                numActiveJobs, job->name);
            } 
            else {
                tcsetpgrp(TERMINAL, job->pgid);
                changeJobStatus(pid, SUSPENDED);
                printf("\n[%d]+   stopped\t   %s\n", numActiveJobs, job->name);
            }
            return;
        } 
        else {
            if (job->status == BACKGROUND) {
                jobsList = delJob(job);
            }
        }
        tcsetpgrp(TERMINAL, PGID);
    }
}

void free_argv()
{
	int index;
	for(index=0;my_argv[index]!=NULL;index++) {						//Percorre my_Argv
		bzero(my_argv[index], strlen(my_argv[index])+1);			//Completa com 0
		my_argv[index] = NULL;										//Libera my_argv
		free(my_argv[index]);
	}
}

int main(int argc, char *argv[], char *envp[])
{
	char c;
	int i, fd;
	i = 0;
    
	char *tmp = (char *)malloc(sizeof(char) * TAMANHO_MAXIMO);
	char *path_str = (char *)malloc(sizeof(char) * 256);
	char *cmd = (char *)malloc(sizeof(char) * TAMANHO_MAXIMO);
	int pid = getpid();
	
	TERMINAL = STDIN_FILENO;
	signal(SIGINT, SIG_IGN);
	signal(SIGINT, handle_signal);

	copy_envp(envp);
	get_path_string(my_envp, path_str);
	insert_path_str_to_search(path_str);
	
	if(fork() == 0) {
		execve("/usr/bin/clear", argv, my_envp);			//Limpa a tela antes de iniciar
		exit(1);
	} 
	else {
		wait(NULL);											//Espera terminar
	}
	printf("[G7] ");
	fflush(stdout);
	setpgid(pid, pid);										//Seta PGID
	PGID = getpgrp();										//Pega PGID

	while((c != EOF)) {
		c = getchar();											//Pega caracter
		switch(c) {
			case '\n': 
				if(tmp[0] == '\0') {								//Verifica se tem alguma coisa em tmp
					printf("[G7] "); 
				}
				else {
				   		fill_argv(tmp);											//Coloca os valores em my_argv
					   	strcpy(cmd, my_argv[0]);
					   	strcat(cmd, "\0");
					   	if((index(cmd, '/') == NULL) && (strcmp(cmd, "quit"))){
					   		if(attach_path(cmd) == 0) {
					   			call_execve(cmd, FOREGROUND);							//Executa comando
						   	} 
						   	else {
							   printf("%s: command not found\n", cmd);		//Erro
						   	}
					   	}
					   	else if(!strcmp(cmd, "quit")){					//Comando quit
					   		exit(1);
					   	}
					   	else {
					   		if((fd = open(cmd, O_RDONLY)) > 0) {		//Verifica arquivo
							   close(fd);								//Fecha arquivo
							   call_execve(cmd, FOREGROUND);						//Executa comando
						   	} 
						   	else {
							   printf("%s: command not found\n", cmd);
						   	}
					   	}
					   	free_argv();								//Libera my_argv
					   	printf("[G7] ");
					   	status_new_process = 0;						//Seta para novo job
					   	bzero(cmd, TAMANHO_MAXIMO);
				   	
			   	}
			   	bzero(tmp, TAMANHO_MAXIMO);
			   	break;
			case '&':									//Verifica se vai executar em background
				status_new_process = 1;					//Seta variavel para background
				printf("%s\n", tmp);
				break;
			default: 
				strncat(tmp, &c, 1);					//Adiciona letras na tmp
			 	break;
		}
		i++;
		fflush(stdin);
	}

	free(tmp);										//Libera tmp
	free(path_str);

	for(i=0;my_envp[i]!=NULL;i++)					//Libera my_envp
		free(my_envp[i]);

	for(i=0;i<10;i++)								//Libera path
		free(search_path[i]);

	printf("\n");
	return 0;
}