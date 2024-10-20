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

class CorruptedCommitFileError : public exception
{
    public:
    const char* what() const noexcept override 
    {
        return "The commit file was corrupted. This is an error from within the code and not a user error.\n";
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
    string data;
    LineNode *nextLine;

    LineNode() {};

    LineNode(string d)
    {
        data = string(d);
        nextLine = NULL;
    }
};

class FileNode
{
    public:
    string filename;
    LineNode *lines;
    char commitVersion[3];

    FileNode(const char* f)
    {
        // this constructor is to be used when you want the latest version of the file.
        filename = string(f);
        lines = NULL;
        char c[2];
        VersionManager::findLatestCommitVersion(c);
        commitVersion[0] = c[0];
        commitVersion[1] = c[1];
        commitVersion[2] = '\0';
        int cv = VersionManager::convertCharToInt(commitVersion);
        
        int i = 0;

        while (i <= cv)
        {
            string p = current_path().string();

            p += "/.beanVC/objects/";

            if (i < 10)
            {
                p.push_back('0');
            }

            p += to_string(i);
            p += ".bin";

            ifstream file = ifstream(p, ios_base::in);
            vector<string> currentCommitContent;
            string line;
            
            while (getline(file, line))
            {
                currentCommitContent.push_back(line);
            }

            file.close();
            merge(currentCommitContent);
            i++;
        }
    }

    void data(string &writer)
    {
        LineNode *current = lines;

        while(current != NULL)
        {
            writer += current->data;
            current = current->nextLine;
            writer.push_back('\n');
        }

        return;
    }

    void merge(vector<string> commitContent)
    {
        // this function takes the commit content and then merges it into the lines variable
        // it first checks the commitContent for all the deleted lines and then removes them from the lines linked list
        // once this is done, for all the newly added lines, we check the orderDecider. This gives us the line number above which it is 
        // supposed to be inserted at.

        int fileStartIndex = -1;
        int fileEndIndex = -1;

        for(int i = 0; i < commitContent.size(); i++)
        {
            if (commitContent[i] == "--START--")
            {
                if (commitContent[i+1] == filename)
                {
                    fileStartIndex = i+2;
                }
            }

            if ((commitContent[i] == "--END--") && (fileStartIndex != -1))
            {
                fileEndIndex = i-1;
                break;
            }
        }

        if (fileStartIndex == -1)
        {  
            // if this is the case, then our file does not exist in this commit.
            return;
        }

        if (fileEndIndex == -1)
        {
            // if this is the case, we did find the file, but not where the files ends.
            throw CorruptedCommitFileError();
        }


        for(int i = fileStartIndex; i <= fileEndIndex; i++)
        {
            int index = (int)(commitContent[i].find_first_of(')'));
            index++;
            if (commitContent[i][index] == '+')
            {
                continue;
            }
            string trimmedString;
            trim(commitContent[i], trimmedString);

            LineNode *current = lines;
            LineNode *prev = NULL;

            while (current != NULL)
            {
                if (current->data == trimmedString)
                {
                    if (prev == NULL)
                    {
                        lines = lines->nextLine;
                    } else {
                        prev->nextLine = current->nextLine;
                    }
                    break;
                }
                prev = current;
                current = current->nextLine;
            }
        }


        for(int i = fileStartIndex; i <= fileEndIndex; i++)
        {
            int j = 0;
            string trimmedLine;
            string refLineString = commitContent[i].substr(commitContent[i].find_first_of('(')+1, commitContent[i].find_first_of(')')-1);
            int refLine = VersionManager::convertCharToInt(refLineString.c_str());

            trim(commitContent[i], trimmedLine);

            if (refLine == 0)
            {
                // we insert the trimmed line at the end;

                LineNode *temp = new LineNode(trimmedLine); 
                
                if (lines == NULL)
                {
                    cout << "This was fine" << endl;
                    lines = temp;
                    continue;
                }

                LineNode *current = lines;

                while (current->nextLine != NULL)
                {
                    current = current->nextLine;
                }

                current->nextLine = temp;
            }
        }

    }

    static void trim(string s, string &writeString)
    {
        int startIndex  = (int)(s.find_first_of(')'));
        startIndex += 2;

        for(string::iterator it = s.begin()+startIndex; it != s.end(); it++)
        {
            writeString.push_back(*it);
        }

        return;
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
    } else if (argument == "show")
    {
        string d;
        cout << "lol";
        FileNode f = FileNode("todo.txt");
        
        f.data(d);

        cout << d << endl;
    }
    return 0;
}