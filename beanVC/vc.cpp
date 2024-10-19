#include <iostream>
#include <fstream>
#include <filesystem>

using namespace std;
using namespace std::filesystem;

void stageChanges()
{
    string currentPath = current_path().string();
    string pathToLogs = current_path().string();
    pathToLogs += "/.beanVC/logs";

    path logs = path(pathToLogs);
    directory_iterator logIt = directory_iterator(logs);
    recursive_directory_iterator it = recursive_directory_iterator(current_path());

    if (logIt == end(logIt))
    {
        // if this is the case, there are no logs, so we can directly add the contents of all the files
        recursive_directory_iterator e = end(it);
        int stop = 0;
        ofstream stagingFile = ofstream(".beanVC/objects/tempStaging.bin", ios_base::out);
        while (!stop)
        {
            string filename = relative(it->path()).string();

            if ((filename[0] == *".") && (filename != "."))
            {
                if (status(it->path()).type() == file_type::directory)
                {
                    it++;
                    it.pop();
                    continue;
                }
                it++;
                continue;
            }

            if (filename.find_first_of(".", 1) == string::npos && (status(it->path()).type() != file_type::directory))
            {
                it++;
                continue;
            }

            cout << "Adding " << filename << endl;

            ifstream currentFile = ifstream(it->path().string(), ios_base::in);

            stagingFile.write("--START--\n", 10);
            stagingFile.write(filename.c_str(), filename.size());
            stagingFile.write("\n", 1);

            string line;

            while(getline(currentFile, line))
            {
                stagingFile.write("+", 1);
                stagingFile.write(line.c_str(), line.size());
                stagingFile.write("\n", 1);
            }
            stagingFile.write("--END--\n", 8);
            currentFile.close();
            it++;

            if (it == e)
            {
                stop = 1;
            }
        }
        stagingFile.close();
    }
}

void initRepo()
{
    path pathname = current_path();
    cout << "Initializing empty repository in " << pathname << " ..." << endl;
    create_directory(".beanVC");
    create_directory(".beanVC/logs");
    create_directory(".beanVC/objects");
    ofstream lastComitVersion = ofstream(".beanVC/lastCommitVersion.txt", ios_base::out);
    lastComitVersion.write("-1", 2);
    lastComitVersion.close();
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
    if (argc == 1)
    {
        printHelpMenu();
        return 0;
    } 

    string argument = string(argv[1]);
    
    if (argument == "init")
    {
        initRepo();
    } else if (argument == "add")
    {
        stageChanges();
    }
    return 0;
}