/*NAME : ATHARVA NAIK ROSHAN & RADHIKA PATWARI 
ROLL : 18CS10067 & 18CS10062
GROUP NO. : 8
OPERATING SYSTEM ASSIGNMENT 2*/

// declaring header files
#include <pwd.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <bits/stdc++.h>

#define MAX_STR_LEN 1000 // maximum length for any string
using namespace std;

// Remove spaces from beginning and end
string strip(string s) {
    for(int i=0; i<s.size(); i++)
        if(s[i] != ' ')
        {
            s = s.substr(i);
            break;
        }
    for(int i=0; i<s.size(); i++)
        if(s[s.size()-i-1] != ' ')
        {
            s = s.substr(0, s.size()-i);
            break;
        }
    return s;
}

// Splitting up string on basis of delimiter ; ignoring empty strings (only when space is a delimiter)
vector<string> split(string s, char delim) {
    vector<string> res;
    string elem="";
    for(char c : s)
    {
        if(c == delim)
        {
            if(delim != ' ' || elem != "")
                res.push_back(elem);
            elem="";
        }
        else
            elem.push_back(c);
    }
    res.push_back(elem);
    return res;
}

// Check if command is a valid shell command using system syscall
bool check_valid(string cmd) {
    ifstream fin;
    string result;
    cmd = strip(cmd);
    cmd = split(cmd, ' ')[0];
    /* the type commands helps in knowing if the command exists on bash
    the output is stored in result.txt */
    string test_cmd = ("type " + cmd + " > result.txt 2>&1").c_str();
    system(test_cmd.c_str());
    // read the output from result.txt
    fin.open("result.txt");
    getline(fin, result);
    // remove result.txt after storing the output in a string
    system("rm result.txt");
    /* if not found occurs in output then the program is not installed
     or it is not a valid bash command */
    vector<string> vec=split(result, ' ');
    if (*(vec.end()-2)+*(vec.end()-1) == "notfound")
        return false;
    return true; 
}

// Check if the command is a cd and change directory using chdir syscall
void check_chdir(string cmd) {
    cmd = strip(cmd);
    if (split(cmd, ' ')[0] != "cd")
    {
        return;
    }
    cmd.replace(0, 3, "");
    cmd.erase(remove(cmd.begin(), cmd.end(), '\"' ), cmd.end());  

    if(!cmd.size())
    {
        chdir(getenv("HOME"));    
        return;
    }

    int status = chdir(cmd.c_str());
    if(status == -1)
    {
        cout<<"virtual-bash: cd: "+cmd+": No such file or directory\n";
    }
}

// Splits the command into input and ouput
vector<string> split_io(string cmd) {
    vector <string> parsed;
    string output, input, command;
    bool lsign=true, rsign=true;

    if (cmd.find('>') == string::npos)      //check if output file does not exist
    {
        output = "";
        rsign = false;
    }
    else
        output = split(cmd, '>').back();    //find the output file if exists
    
    output = strip(output);
    output = split(output, '<')[0];
    if(output[0] == '\"')
        output = "\""+split(output, '\"')[1]+"\"";
    else    
        output = split(output, ' ')[0];
    output = strip(output);

    if (cmd.find('<') == string::npos)      //check if input file does not exist
    {
        input = "";
        lsign = false;
    }
    else
        input = split(cmd, '<').back();     //find the input file if exists
    
    input = strip(input);
    input = split(input, '>')[0];
    if(input[0] == '\"')
        input = "\""+split(input, '\"')[1]+"\"";
    else 
        input = split(input, ' ')[0];
    input = strip(input);

    // handling and returning syntax errors for these cases :
    // case 1 : same input and output file 
    /* Treating this as error to avoid overwriting the input file 
    In bash, the input file gets completely wiped out */
    if(input == output && (lsign||rsign))   
    {
        cout<<"virtual-bash: syntax error"<<"\n";
        return {};
    }

    // case 2 : no input or outfile files corresponding to input and output symbols
    if(lsign)                               
    {
        if(input == "")                  
        {
            cout<<"virtual-bash: syntax error"<<"\n";
            return {};
        }
    }
    if(rsign)
    {
        if(output == "")
        {
            cout<<"virtual-bash: syntax error"<<"\n";
            return {};
        }
    }

    /* using regular expressions to extract the command, by erasing
    names of input output files and the "<" and ">" signs */ 
    regex re_op(output);
    regex re_inp(input);
    regex re_lsign("<");
    regex re_rsign(">");
    
    command = regex_replace(cmd, re_inp, "");
    command = regex_replace(command, re_op, "");
    command = regex_replace(command, re_lsign, "");
    command = regex_replace(command, re_rsign, "");
    command = strip(command);

    input.erase(remove(input.begin(), input.end(), '\"' ), input.end());
    output.erase(remove(output.begin(), output.end(), '\"' ), output.end());
    parsed.push_back(command);    // parsed[0] = command
    parsed.push_back(input);      // parsed[1] = input file
    parsed.push_back(output);     // parsed[2] = output file
    
    return parsed;
}
// Open files and redirect input and output with files as arguments
void redirect_io(string inp, string out)
{
    // Open input redirecting file
    if(inp.size())
    {
        int inp_fd = open(inp.c_str(),O_RDONLY);  // Open in read only mode
        int status = dup2(inp_fd,0); // Redirect input
        if(inp_fd < 0)
        {
            cout<<"virtual-bash: "<<inp<<": No such file or directory"<<endl;
            exit(EXIT_FAILURE);
        }
        if(status < 0)
        {
            cout<<"Input redirecting error"<<endl;
            exit(EXIT_FAILURE);
        }
    }
    // Open output redirecting file
    if(out.size())
    {
        int out_fd = open(out.c_str(), O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);  // Open in create and truncate mode
        int status = dup2(out_fd,1);// Redirect output
        if(status < 0)
        {
            cout<<"Ouput redirecting error"<<endl;
            exit(EXIT_FAILURE);
        }
    }
}

// Execute external commands
void exec_cmd(string cmd)
{
    // Split the command and its arguments
    // execvp needs a constant array of c strings
    cmd = strip(cmd);
    regex quoted_string("\"([^\"]*)\"");
    vector<string> quoted_strings;
    for(sregex_iterator it = sregex_iterator(cmd.begin(), cmd.end(), quoted_string); it != sregex_iterator(); it++)
    {
        smatch match = *it;
        quoted_strings.push_back(match.str(0));
    }
    cmd = regex_replace(cmd, quoted_string, "<QUOTED-STRING>");

    vector <char*> args;
    int i=0;
    for(string s: split(cmd, ' '))
    {
        if(s == "<QUOTED-STRING>")
        {
            string temp = quoted_strings[i]; 
            temp.erase(remove(temp.begin(), temp.end(), '\"' ), temp.end());  
            s=temp; 
            i++;
        }
        char *temp = new char[s.size()+1];
        strcpy(temp, const_cast<char*>(s.c_str()));
        args.push_back(temp);
    }
    args.push_back(NULL);
    char** argv = &args[0];
    // Call the execvp command
    execvp(split(cmd,' ')[0].c_str(), argv); 
}

// Execute pipe commands
void exec_pipe(string cmd, int status, bool bg) {
    string new_cmd;
    bool resume_pipe=false;

    if(cmd.rfind("|", 0) == 0)
    {
        cout<<"virtual-bash: syntax error\n";
        return;
    }
    vector<string> cmds = split(cmd, '|');
    // if no pipes found
    if(cmds.size()==1)
    {
        // Split the commands and redirection
        vector<string> parsed = split_io(cmds[0]);        
        if(parsed.size()==0)    // returning in case of syntax error
            return;
        // Check if command is valid
        if(!check_valid(parsed[0]))
        {
            cout<<split(parsed[0], ' ')[0]<<": command not found\n";
            return;
        }
        pid_t pid = fork();     
        // Catch errors in forking the process
        if(pid == -1)
        {
            perror("Failure in creation of child process; fork() failed!");
            exit(EXIT_FAILURE);
        }
        // inside child process
        if(pid == 0)     
        {
            redirect_io(parsed[1],parsed[2]); // Redirect input and output if required
            exec_cmd(parsed[0]);              // Execute the command
            exit(0);                          // Exit the child process
        }
        //inside the parent process
        if(!bg)                 // running child process in background and continuing parent process execution in case of ampersand
            wait(&status);      // waiting for child process to finish in case of no ampersand
    }
    
    else
    {
        int n=cmds.size(); // No. of pipe commands
        int newFD[2], oldFD[2];

        for(int i=0; i<n; i++)
        {
            // For cases that erroneously end in pipe, e.g. "cat Ass2.cpp|"
           if(strip(cmds[i])=="")
                continue; 
            vector<string> parsed = split_io(cmds[i]);
            if(parsed.size() == 0)      // returning in case of syntax error
                return;
            // Check if command is valid
            if(!check_valid(parsed[0]))
            {
                cout<<split(parsed[0], ' ')[0]<<": command not found\n";
                vector<string> new_cmds = vector<string> (cmds.begin()+i+1, cmds.end()); 
                
                for(string s : new_cmds)
                    new_cmd += (strip(s)+" | ");
                new_cmd = strip(new_cmd.substr(0, new_cmd.size()-2));
                resume_pipe=true;
                break;
            }
            // Create new pipe except for the last command
            if(i!=n-1)                   
                pipe(newFD);
            // Fork for every command
            pid_t pid = fork();          
            // Error in forking the process
            if(pid == -1)
            {
                perror("Failure in creation of child process!");
                exit(EXIT_FAILURE);
            }
            // Inside the child process
            if(pid == 0)
            {
                // if( !i || i==n-1)
                // Read from previous command for everything except the first command
                if(i)
                    dup2(oldFD[0],0), close(oldFD[0]), close(oldFD[1]);
                // Write into pipe for everything except last command
                if(i!=n-1)
                    close(newFD[0]), dup2(newFD[1],1), close(newFD[1]);
                // Execute command
                redirect_io(parsed[1], parsed[2]);  // For the first and last command redirect the input output files
                exec_cmd(parsed[0]);
            }
            // In parent process
            if(i)
                close(oldFD[0]), close(oldFD[1]);
                
            // Copy newFD into oldFD for everything except the last process
            if(i!=n-1)
                oldFD[0] = newFD[0], oldFD[1] = newFD[1];
        }
        // If no background, then wait for all child processes to return
        if(!bg)
            while(wait(&status) > 0);
    }

    if(resume_pipe)
    {
        exec_pipe(new_cmd, status, bg);
        return;
    }
}

// Check for exit command
void check_exit(string cmd) {
    if (split(cmd, ' ')[0] == "exit")
    {
        cout << "\033[1;31mExiting shell-on-shell ...\033[0m\n";
        exit(EXIT_FAILURE);
    }
}

int main()
{
    string cmd;
    int status = 0;
    cout << "\033[1;32mWelcome to shell-on-shell:\033[0m\n";
    cout << "\033[1;31mcreated by [18CS10067] Atharva Naik: \033[0m"
         << "\033[1;4mhttps://github.com/atharva-naik\033[0m\n";
    cout << "\033[1;31m"
         << "    "
         << "and [18CS10062] Radhika Patwari: \033[0m"
         << "\033[1;4mhttps://github.com/rsrkpatwari1234\033[0m\n";
    while(true)
    {
        char curr_dir[MAX_STR_LEN] = "";
        char *str_pt; // string pointer to get current working directory

        str_pt = getcwd(curr_dir, sizeof(curr_dir));
        if (str_pt == NULL)
        {
            perror("Couldn't fetch current working directory!"); // to print error in getcwd.
            // Needs to be called immediately after error is encountered or
            // it may get written by other function calls
            continue;
        }

        bool bg = false; // flag for background running

        // Get input command
        string cwd = string(curr_dir).replace(0, 14, "~/");
        cout << "(virtual) \033[1;33mnow-running-shell-on-shell:\033[0m"
             << "\033[1;34m" << cwd << "\033[0m"
             << "$ ";
        getline(cin, cmd); // get the entire line

        // Remove trailling and preceeding spaces
        cmd = strip(cmd);
        // Check for exit
        check_exit(cmd);
        // Change dir if needed
        check_chdir(cmd);
        // Check for background run
        if(cmd.back() == '&')
            bg = true, cmd.back() = ' ';
        // Parse piped commands and set up child processes
        exec_pipe(cmd, status, bg);
    }
}