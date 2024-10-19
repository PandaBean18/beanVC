#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

using namespace std::filesystem;
using namespace std;

class FileNode
{
    // this class represents a single file and its data
    public:
    string filename;
    string data;

    FileNode() {};

    // when a new filenode is initialized, it should go through all the files in the objects directory and look for 

    void trim(string &s, string &result)
    {
        size_t index_p = s.find_first_of('+');
        size_t index_m = s.find_first_of('-');

        if (index_p == string::npos || index_m < index_p)
        {
            result = "";
            return;
        }

        int i = (int)(index_p)+1;

        for(i; i < s.size(); i++)
        {
            result.push_back(s[i]);
        }
        return;
    }

    void orderData()
    {
        // code to get file data by combining data of all the commits. it works as follows:
        // if the file is of initial commit OR has been added in the current commit, the order decider will be set to -1.
        // this means that all the stuff currently goes before the EOF meaning all the stuff goes below whatever was there before.

        // if the order decider has a value of K, this means that this particular line goes before the Kth line from previous commit
        string pathToObj = current_path().string() + "/.beanVC/objects";

        directory_iterator objIt = directory_iterator(pathToObj);
        directory_iterator e = end(objIt);
        vector<string> fileContent;

        while(objIt != e)
        {
            string currentFilePath = objIt->path().string();
            string line;
            ifstream currentFile = ifstream(currentFilePath, ios_base::in);
            int insideAnotherFile = 0;

            while(getline(currentFile, line))
            {
                if (insideAnotherFile)
                {
                    if (line == "--END--")
                    {
                        insideAnotherFile = 0;
                    }
                    continue;
                } else if (line == "--START--")
                {
                    getline(currentFile, line);
                    if (line == filename) 
                    {
                        insideAnotherFile = 0;
                    } else {
                        insideAnotherFile = 1;
                    }
                } else 
                {
                    if (line == "--END--") 
                    {
                        break;
                    }

                    if (line[1] == '0') // represents the case when order decider is 0
                    {
                        string result;
                        trim(line, result);
                        fileContent.push_back(result);
                    }
                }
            }
            currentFile.close();
            objIt++;
        }

        for(int i = 0; i < fileContent.size(); i++)
        {
            data += fileContent[i];
            data.push_back('\n');
        }
    }

    FileNode(string &name)
    {
        filename = name;
        orderData();
    }
};

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
                stagingFile.write("(0)", 3);
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

int convertCharToInt(const char* c)
{
    if ((*c == '-') && (*(c+1) == '1')) {
        return -1;
    }

    const char nums[] = "0123456789";
    int val = 0;

    while(*c != '\0') 
    {
        int i = 0;

        for(i; nums[i] != '\0'; i++) {
            if (nums[i] == *c) {
                break;
            }
        }

        val = val*10;
        val += i;
        c++;
    }

    return val;
}

void addStagedChanges()
{
    // this function checks out the objects directory looking for a tempStaging.bin file, if found, it writes that data in tempStaging.bin
    // else it throws error and exits 
    ifstream tempStagingFile = ifstream(".beanVC/objects/tempStaging.bin", ios_base::in);
    
    if (!tempStagingFile.is_open()) {
        cerr << "Staging file not found. Have you ran ./vc add to stage your files? \n";
        tempStagingFile.close();
        return ;
    }

    ifstream lastCommitVersionFile = ifstream(".beanVC/lastCommitVersion.txt", ios_base::in);

    char c[2];
    lastCommitVersionFile.read(c, 2);
    lastCommitVersionFile.close();

    int commitVersion = convertCharToInt(c) + 1;

    ofstream stagingFile;

    if (commitVersion < 10)
    {
        string s = ".beanVC/objects/0";
        s += commitVersion;
        s += ".bin";
        stagingFile = ofstream(s, ios_base::out);
    } else {
        string s = ".beanVC/objects/";
        s += commitVersion;
        s += ".bin";
        stagingFile = ofstream(s, ios_base::out);
    }

    string line;

    while(getline(tempStagingFile, line))
    {
        stagingFile.write(line.c_str(), line.size());
        stagingFile.write("\n", 1);
    }

    stagingFile.close();
    tempStagingFile.close();
    path tempStagingFilePath = path(".beanVC/objects/tempStaging.bin");
    remove(tempStagingFilePath);
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

void showFiles()
{
    string name = "todo.txt";
    FileNode todo = FileNode(name);
    cout << todo.data << endl;
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
    } else if (argument == "commit")
    {
        addStagedChanges();
    } else if (argument == "show")
    {
        showFiles();
    }
    return 0;
}