// ******************************
// author: Tram Ta
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h> 
#include <sys/wait.h>

// ******************************
const char error_message[30] = "An error has occurred\n";

char **path = NULL;
int path_size = 0;
    

// ******************************
char **parse_cmdLine(char *cmdLine)
{   
    char **cmdArr; 
    cmdArr = (char **) malloc(sizeof(char *) * 100); 
    if(cmdArr == NULL){
        return NULL; 
    }

    int cmdArr_size = 0;
    char *cmd = NULL;

    while( (cmd = strsep(&cmdLine, "&")) != NULL){
        if(strcmp(cmd, "") != 0){
            cmdArr[cmdArr_size] = strdup(cmd);
            cmdArr_size++;
        }       
    }
    cmdArr[cmdArr_size] = NULL;
    return cmdArr;
}


// ******************************
void free_process_cmd(char *pre_redirect2, char *post_redirect2, char *output_path){
    free(pre_redirect2); pre_redirect2 = NULL;
    free(post_redirect2); post_redirect2 = NULL;
    free(output_path); output_path = NULL;
}


// ******************************
int process_cmd(char *cmd, char **cmd_argv, int *redirect_cnt, int *fd) 
{   
    char *pre_redirect = NULL, *post_redirect = NULL;
    char *pre_redirect2 = NULL, *post_redirect2 = NULL;

    int cmd_argv_size2 = 0, redirect_cnt2 = 0;      
    char *token = NULL;

    int add_pre = 1;
    while( (token = strsep(&cmd, ">")) != NULL) 
    {
        if(strcmp(token, "") == 0){
            free_process_cmd(pre_redirect2, post_redirect2, NULL);
            return 1;
        }
        
        if(add_pre == 1){
            pre_redirect2 = strdup(token);
            pre_redirect = pre_redirect2;
            add_pre = 0;
        }
        else if(add_pre == 0){
            if(redirect_cnt2 == 0){
                post_redirect2 = strdup(token); 
                post_redirect = post_redirect2;
                redirect_cnt2++;
            }
            else if(redirect_cnt2 > 0){
                free_process_cmd(pre_redirect2, post_redirect2, NULL);
                return 1; 
            }
        }            
    }  

    char *output_path = NULL, *output_path2=NULL;
    int output_path_cnt = 0;
    int fd2 = 0;

    if(redirect_cnt2 > 0)
    {
        if( post_redirect == NULL || strlen(post_redirect) == 0){ 
            free_process_cmd(pre_redirect2, post_redirect2, NULL);
            return 1;  
        }

        token = NULL;
        while( (token = strsep(&post_redirect, " \t")) != NULL)
        {
            if( strcmp(token, "") == 0){
                // ...
            }
            else if( strcmp(token, "") != 0){
                if(output_path_cnt == 0){
                    output_path = strdup(token); 
                    output_path2 = output_path;
                    output_path_cnt++;
                }
                else{ 
                    free_process_cmd(pre_redirect2, post_redirect2, output_path2);
                    return 1;  
                }
            }
        } 

        fd2 = open(output_path, O_WRONLY | O_TRUNC | O_CREAT, 0666);
        if(fd2 < 0){
            free_process_cmd(pre_redirect2, post_redirect2, output_path2);
            return 1; 
        }
        *fd = fd2;
    }

    token = NULL;
    while( (token = strsep(&pre_redirect, " \t")) != NULL ){
        if( strcmp(token, "") != 0 ){        
            cmd_argv[cmd_argv_size2] = strdup(token); 
            cmd_argv_size2++;
        }
    }
    cmd_argv[cmd_argv_size2] = NULL;

    *redirect_cnt = redirect_cnt2;
    free_process_cmd(pre_redirect2, post_redirect2, output_path2);
    return 0;
}


// ******************************
void free_exec_cmd(char **cmd_argv, char *bin_path, char *bin_path2){
    int cmd_argv_size = 0; 
    while(1){
        if(cmd_argv[cmd_argv_size] == NULL)
            break;
        cmd_argv_size++;
    }

    int arg_i;
    for(arg_i =0; arg_i < cmd_argv_size; arg_i++){
        free(cmd_argv[arg_i]); cmd_argv[arg_i] = NULL;  
    }
    free(cmd_argv); cmd_argv = NULL;

    free(bin_path); bin_path = NULL;
    free(bin_path2); bin_path2 = NULL;
}


// ******************************
int exec_cmd(char *cmd) 
{     
    char **cmd_argv = (char **)malloc(sizeof(char *) * 100); 
    if(cmd_argv == NULL){
        free(cmd_argv);
        return 1;
    }
    cmd_argv[0] = NULL;
    
    int redirect_cnt = 0;   
    int fd = STDOUT_FILENO;  

    int tmp = process_cmd(cmd, cmd_argv, &redirect_cnt, &fd); 

    if(tmp == 1){
        free_exec_cmd(cmd_argv, NULL, NULL);
        return 1;  
    }

    int cmd_argv_size = 0; 
    while(1){
        if(cmd_argv[cmd_argv_size] == NULL)
            break;
        cmd_argv_size++;
    }
    cmd_argv[cmd_argv_size] = NULL; 

    if(cmd_argv_size == 0){ 
        free_exec_cmd(cmd_argv, NULL, NULL);
        return 0;   
    }

    if( strcmp(cmd_argv[0], "cd") == 0 ) { 
        if( cmd_argv_size != 2 || chdir(cmd_argv[1]) != 0 ) {
            free_exec_cmd(cmd_argv, NULL, NULL);
            return 1;  
        }
        free_exec_cmd(cmd_argv, NULL, NULL);
        return 0;
    }

    else if( strcmp(cmd_argv[0], "path") == 0 ){
        int tmp;
        for(tmp =0; tmp < path_size; tmp++){
            free(path[tmp]);
            path[tmp] = NULL;
        }
        
        path_size = 0;
        for(tmp =1; tmp < cmd_argv_size; tmp++){
            path[path_size] = strdup(cmd_argv[tmp]);
            path_size++;
        }
        path[path_size] = NULL;

        free_exec_cmd(cmd_argv, NULL, NULL);
        return 0;
    }

    else if (strcmp(cmd_argv[0], "exit") == 0) {
        free_exec_cmd(cmd_argv, NULL, NULL);

        if(cmd_argv_size != 1){
            return 1;  
        }
        else exit(0);
    }

    else 
    {   
        int found_bin_path = 0;
        
        char *bin_path = NULL; 
        char *bin_path2 = (char *)malloc(sizeof(char) *500); 
        if(bin_path2 == NULL){
            free_exec_cmd(cmd_argv, NULL, NULL);
            return 1;  
        }

        int path_i = 0;
        while(path[path_i] != NULL){            
            strcpy(bin_path2, path[path_i]);
            strcat(bin_path2, "/");
            strcat(bin_path2, cmd_argv[0]);  

            if( access(bin_path2, X_OK) == 0 ){ 
                found_bin_path = 1;
                bin_path = strdup(bin_path2); 
                break;
            }
            path_i++;
        }

        if( !found_bin_path ){
            free_exec_cmd(cmd_argv, bin_path, bin_path2);
            return 1;  
        }

        int fork_rc = fork();
        if (fork_rc == -1){
            free_exec_cmd(cmd_argv, bin_path, bin_path2);
            return 1;  
        }

        else if(fork_rc == 0){ 
            if(redirect_cnt > 0){               
                int dup2_out = dup2(fd, STDOUT_FILENO); 
                if(dup2_out == -1){
                    free_exec_cmd(cmd_argv, bin_path, bin_path2);
                    return 1;   
                }
            }

            execv(bin_path, cmd_argv);  
            free_exec_cmd(cmd_argv, bin_path, bin_path2);                   
            return 1; 
        }
        else if(fork_rc > 0) { 
            free_exec_cmd(cmd_argv, bin_path, bin_path2);

            if(redirect_cnt > 0){ 
                close(fd);
            }
        }   
        return 0;   
    } 
    return 0;
} 


// ******************************
int main(int argc, char *argv[])
{ 
    if(argc > 2) {
        write(STDERR_FILENO, error_message, strlen(error_message));     
        exit(1);
    }

    path = (char **) malloc(sizeof(char *) * 100);  
    if(path == NULL){
        write(STDERR_FILENO, error_message, strlen(error_message)); 
        exit(1);
    }

    path[0] = strdup("/bin");    
    path[1] = NULL;
    path_size = 1;

    FILE *fin = stdin;   

    char *cmdLine = NULL;
    char **cmdArr = NULL;    
    int cmdArr_size = 0;

    size_t len;
    ssize_t nread;

    int isError = 0;
    int ii;

    while(1)
    {
        isError = 0;
        if (argc == 1) // cmd: ./wish
            printf("wish> ");
        
        if(argc == 2){  // cmd: ./wish  input.txt
            fin = fopen(argv[1], "r");
            if(fin == NULL){    
                write(STDERR_FILENO, error_message, strlen(error_message)); 
                exit(1);
            }
        }

        while( (nread = getline(&cmdLine, &len, fin)) != -1 )  
        {
            if(cmdLine[strlen(cmdLine)-1] == '\n')
                cmdLine[strlen(cmdLine)-1] = '\0';  

            cmdArr = parse_cmdLine(cmdLine);  
            if(cmdArr == NULL){
                write(STDERR_FILENO, error_message, strlen(error_message)); 
                free(cmdArr); 
                continue;  
            }

            ii = 0;
            while(cmdArr[ii] != NULL){
                char *cmd = strdup(cmdArr[ii]);  
                char *cmd2 = cmd;
                
                int tmp = exec_cmd(cmd); 
                free(cmd2); cmd2 = NULL;   

                if(tmp == 1){
                    isError = 1;
                    break; 
                }
                ii++;
            }

            if(isError){
                write(STDERR_FILENO, error_message, strlen(error_message)); 
                free_exec_cmd(cmdArr, NULL, NULL);
                continue; 
            }
            cmdArr_size = ii;
            
            ii = 0;
            for(ii =0; ii < cmdArr_size; ii++){
                wait(NULL);
            }

            free_exec_cmd(cmdArr, NULL, NULL);
        }

        free(cmdLine); cmdLine = NULL;
        if(argc == 2){
            fclose(fin);
            break; 
        }
    }

    int path_i;
    for(path_i=0; path_i < path_size; path_i++){
        free(path[path_i]);
    }
    free(path); 

    return 0;   
} 
