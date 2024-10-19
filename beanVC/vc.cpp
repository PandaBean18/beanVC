#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <ctime>

using namespace std::filesystem;
using namespace std;

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

class FileNode
{
    // this class represents a single file and its data
    public:
    string filename;
    string data;

    FileNode() {};

    // when a new filenode is initialized, it should go through all the files in the objects directory and look for 

    static void trim(string &s, string &result)
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

    int checkPlaceToBeInserted(vector<string> prevCommitContent, vector<string> currentFileContent, int index, int indexToBeInsertedAt)
    {
        for(int x = 0; x < prevCommitContent.size(); x++) 
        {
            if (prevCommitContent[x] == "--START--")
            {
                if (prevCommitContent[x+1] == filename)
                {
                    // by this point we have found the commit for this version. The line above which the string is to be inserted is at 
                    // prevCommitContent[x+1+index]. now we check sentences in currentFileContent from indexToBeInsertedAt. 
                    // since no changes have been made to the content below line at index (we are moving iteratively), lines should match
                    // this comparision goes on till prevCommitContent[x+1+index+i] == "--END--"
                    int i = index;
                    int j = indexToBeInsertedAt;
                    int looped = 0;

                    while(prevCommitContent[x+1+i] != "--END--")
                    {
                        looped = 1;
                        string trimmedLine;
                        FileNode::trim(prevCommitContent[x+1+i], trimmedLine);
                        if(currentFileContent[j] != trimmedLine) {
                            return 0;
                        }
                        j++;
                        i++;
                    }

                    if (!looped) {
                        cerr << "Ordering index out of bounds for previous commit file.\n";
                        exit(1);
                    }

                    return 1;
                }
            }
        }
        return 0;
    }

    void orderData()
    {
        // code to get file data by combining data of all the commits. it works as follows:
        // if the file is of initial commit OR has been added in the current commit, the order decider will be set to -1.
        // this means that all the stuff currently goes before the EOF meaning all the stuff goes below whatever was there before.

        // if the order decider has a value of K, this means that this particular line goes before the Kth line from previous commit
        ifstream lastCommitFile = ifstream(".beanVC/lastCommitVersion.txt", ios_base::in);
        char c[2];
        lastCommitFile.read(c, 2);
        lastCommitFile.close();

        int lastCommitVersion = convertCharToInt(c);
        int currentFileVersion = 0;
        vector<string> fileContent;
        vector<string> prevCommitContent;

        while(currentFileVersion <= lastCommitVersion)
        {
            
            string currentFilePath = ".beanVC/objects/";
            vector<string> currentCommitContent;

            if (currentFileVersion < 10) 
            {
                currentFilePath.push_back('0');
                currentFilePath += to_string(currentFileVersion);
            } else {
                currentFilePath += to_string(currentFileVersion);
            }
            currentFilePath += ".bin";

            string line;
            ifstream currentFile = ifstream(currentFilePath, ios_base::in);
            int insideAnotherFile = 0;

            while(getline(currentFile, line))
            {
                currentCommitContent.push_back(line);
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
                    currentCommitContent.push_back(line);
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
                    } else if (line[1] >'0' && line[1] < '9') {
                        char a[2] = {line[1], '\0'};
                        int index = convertCharToInt(a);
                        string lowerBound; 
                        for(int x = 0; x < prevCommitContent.size(); x++) 
                        {
                            if (prevCommitContent[x] == "--START--")
                            {
                                if (prevCommitContent[x+1] == filename)
                                {
                                    lowerBound = string(prevCommitContent[x+1+index]);
                                    break;
                                }
                            }
                        }

                        int indexToBeInsertedAt = -1;
                        string trimmedLowerBound;
                        FileNode::trim(lowerBound, trimmedLowerBound);

                        for(int j = fileContent.size()-1; j >= 0; j--)
                        {
                            int shouldBeInsertedHere = checkPlaceToBeInserted(prevCommitContent, fileContent, index, j);
                            if (shouldBeInsertedHere) {
                                indexToBeInsertedAt = j;
                                break;
                            }
                        }

                        if (indexToBeInsertedAt == -1)
                        {
                            cerr << "The line that commit file " << currentFileVersion << " was using for ordering content does not exist in previous commit file.\n";
                            exit(1);
                        }

                        string result;
                        trim(line, result);
                        fileContent.insert(fileContent.begin()+indexToBeInsertedAt, result);
                    }
                }
            }
            currentFile.close();
            currentFileVersion++;
            prevCommitContent = vector(currentCommitContent);
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
    } else {

    }
}

void addStagedChanges(string commitMessage)
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
        s += to_string(commitVersion);
        s += ".bin";
        stagingFile = ofstream(s, ios_base::out);
    } else {
        string s = ".beanVC/objects/";
        s += to_string(commitVersion);
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
    string logPathString = ".beanVC/logs/";
    remove(tempStagingFilePath);

    ofstream commitVersionFile = ofstream(".beanVC/lastCommitVersion.txt", ios_base::out);

    if (commitVersion < 10)
    {
        logPathString.push_back('0');
        commitVersionFile.write("0", 1);
        commitVersionFile.write(to_string(commitVersion).c_str(), 1);
    } else {
        commitVersionFile.write(to_string(commitVersion).c_str(), 2);
    }
    logPathString += to_string(commitVersion);
    logPathString += "_log.txt";
    commitVersionFile.close();


    time_t timestamp = time(0);
    string t = to_string(timestamp);

    ofstream logFile = ofstream(logPathString, ios_base::out);
    logFile.write(t.c_str(), t.size());
    logFile.write("\n", 1);
    logFile.write(commitMessage.c_str(), commitMessage.size());

    logFile.close();

    return;
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
        string commitMessage = string(argv[2]);
        addStagedChanges(commitMessage);
    } else if (argument == "show")
    {
        showFiles();
    } else if (argument == "test")
    {
        cout << string(argv[2]);
    }
    return 0;
}