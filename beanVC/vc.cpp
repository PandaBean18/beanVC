#include <iostream>
#include <fstream>
#include <filesystem>

using namespace std;
using namespace std::filesystem;

void initRepo()
{
    path pathname = current_path();
    cout << "Initializing empty repository in " << pathname << "..." << endl;
    create_directory(".beanVC");
    return;
}

void printHelpMenu()
{
    cout << "BeanVC commands: " << endl << endl;
    cout << "init: initializes an empty repository." << endl;
    cout << "status: gives you a status of repositories and tells you which files are untracked and unstaged." << endl;
    cout << "log: gives a log of previous commits along with their commit messages in latest to oldest order." << endl;
    cout << "add: stages all the untracked files." << endl;
    cout << "commit: commits staged changes." << endl;
}

int main(int argc, char *argv[])
{
    //initRepo();
    if (argc == 1)
    {
        printHelpMenu();
        return 0;
    } 
    
    string argument = string(argv[1]);
    
    if (argument == "init")
    {
        initRepo();
    }
    return 0;
}