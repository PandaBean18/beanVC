#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <filesystem>
#include <fstream>

using namespace std;
using namespace std::filesystem;

class UninitializedError : public exception 
{
    public:
    const char* what() const noexcept override
    {
        return "The VC has not been initialized. Try running ./vc init.\n";
    }
};

class UnstagedError : public exception
{
    public:
    const char* what() const noexcept override
    {
        return "Staging file not found. Have you ran ./vc add to stage your files? \n";
    }
};

class VersionManager 
{
    public:
    static void findLatestCommitVersion(char* c)
    {
        string path = current_path().string();
        path += "/.beanVC/lastCommitVersion.txt";
        ifstream lastCommitVersionFile = ifstream(path, ios_base::in);

        if (!lastCommitVersionFile.is_open())
        {
            throw UninitializedError();
        }

        lastCommitVersionFile.read(c, 2);
        lastCommitVersionFile.close();
        return;
    }

    static int convertCharToInt(const char* c)
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

    static void initialize()
    {
        path pathname = current_path();
        cout << "Initializing empty repository in " << pathname << "..." << endl;
        create_directory(".beanVC");
        create_directory(".beanVC/logs");
        create_directory(".beanVC/objects");
        ofstream lastComitVersion = ofstream(".beanVC/lastCommitVersion.txt", ios_base::out);
        lastComitVersion.write("-1", 2);
        lastComitVersion.close();
    return;
    }

    static void stageChanges()
    {
        char c[2];
        findLatestCommitVersion(c);
        int commitVersion = convertCharToInt(c);
        recursive_directory_iterator it = recursive_directory_iterator(current_path());

        if (commitVersion == -1)
        {
            // no commits have been made yet.
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
                int lineCount = 0;
                while(getline(currentFile, line))
                {
                    string lc = to_string(lineCount);
                    lc.push_back(')');
                    stagingFile.write("(", 1);
                    stagingFile.write(lc.c_str(), lc.size());
                    stagingFile.write("+", 1);
                    stagingFile.write(line.c_str(), line.size());
                    stagingFile.write("\n", 1);
                    lineCount++;
                }
                stagingFile.write("--END--\n", 8);
                currentFile.close();

                it++;

                if (it == e)
                {
                    stop = 1;
                }
            }
        }
    }

    static void commitStagedChanges(string commitMessage)
    {
        string p = current_path().string();
        p += "/.beanVC/objects/tempStaging.bin";
        ifstream tempStagingFile = ifstream(p.c_str(), ios_base::in);
    
        if (!tempStagingFile.is_open()) {
            throw UnstagedError();
        }

        char c[2];
        findLatestCommitVersion(c);
        int commitVersion = convertCharToInt(c)+1;

        ofstream stagingFile;

        if (commitVersion < 10)
        {
            string s = current_path().string();
            s += "/.beanVC/objects/0";
            s += to_string(commitVersion);
            s += ".bin";
            stagingFile = ofstream(s, ios_base::out);
        } else {
            string s = current_path().string();
            s += "/.beanVC/objects/";
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

        path tempFilePath = path(p);
        remove(tempFilePath);
        incrementCommitVersion();

        string logPathString = current_path().string();
        logPathString += "/.beanVC/logs/";

        if (commitVersion < 10)
        {
            logPathString.push_back('0');
        }

        logPathString += to_string(commitVersion);
        logPathString += "_log.txt";

        time_t timestamp = time(0);
        string t = to_string(timestamp);

        ofstream logFile = ofstream(logPathString, ios_base::out);
        logFile.write(t.c_str(), t.size());
        logFile.write("\n", 1);
        logFile.write(commitMessage.c_str(), commitMessage.size());

        logFile.close();
    }

    private:

    static void incrementCommitVersion()
    {
        char c[2];
        findLatestCommitVersion(c);
        int commitVersion = convertCharToInt(c)+1;
        string path = current_path().string();
        path += "/.beanVC/lastCommitVersion.txt";

        ofstream lastCommitVersionFile = ofstream(path.c_str(), ios_base::out);

        if (commitVersion < 10)
        {
            lastCommitVersionFile.write("0", 1);
        }

        string cv = to_string(commitVersion);

        lastCommitVersionFile.write(cv.c_str(), cv.size());

        lastCommitVersionFile.close();
        return;
    }
};

class LineNode 
{
    public:
    int lineNumber;
    string data;
    LineNode *nextLine;

    LineNode() {};

    LineNode(string d, int n)
    {
        data = string(d);
        lineNumber = n;
        nextLine = NULL;
    }
};

class FileNode
{
    public:
    string filename;
    LineNode *lines;
    char *commitVersion;

    FileNode(string f)
    {
        // this constructor is to be used when you want the latest version of the file.
        filename = string(f);
        char c[2];
        VersionManager::findLatestCommitVersion(c);
    }
};

int main(int argc, char *argv[])
{
    string argument = string(argv[1]);
    
    if (argument == "init")
    {
        VersionManager::initialize();
    } else if (argument == "add")
    {
        VersionManager::stageChanges();
    } else if (argument == "commit")
    {
        string message = argv[2];
        VersionManager::commitStagedChanges(message);
    }
    return 0;
}