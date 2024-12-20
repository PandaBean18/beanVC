#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <chrono>

#include "versionManager.h"

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

class MissingCommitMessageError : public exception 
{
    public:
    const char* what() const noexcept override 
    {
        return "A commit message is required";
    }
};

class FileNotFoundError : public exception 
{
    public: 
    const char* what() const noexcept override 
    {
        return "The file that was requested was not found in the directory. Incase this error was thrown during rollback command please check filename.\n";
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

    FileNode(const char* f, const char* c)
    {
        // this constructor is to be used when you want the latest version of the file.
        filename = string(f);
        lines = NULL;
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

            if (!exists(path(p)))
            {
                throw FileNotFoundError();
            }
            
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

        if (fileEndIndex < fileStartIndex)
        {
            return ;
        }

        for(int i = fileStartIndex; i <= fileEndIndex; i++)
        {   
            // code for deleting a line from the file
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
            string refLineString = commitContent[i].substr(commitContent[i].find_first_of('(')+1, commitContent[i].find_first_of(')')-1);
            int refLine = VersionManager::convertCharToInt(refLineString.c_str());
            int a = 1;

            while (a != refLine)
            {
                current = current->nextLine;
                a++;
            }

            current->nextLine = current->nextLine->nextLine;
        }


        map<int, int> numLinesAdded;
        for(int i = fileStartIndex; i <= fileEndIndex; i++)
        {
            // code for adding lines
            int index = (int)(commitContent[i].find_first_of(')'));
            index++;
            if (commitContent[i][index] == '-')
            {
                continue;
            }

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
                    lines = temp;
                    continue;
                }

                LineNode *current = lines;

                while (current->nextLine != NULL)
                {
                    current = current->nextLine;
                }

                current->nextLine = temp;
            } else if (refLine == -1) {
                if (numLinesAdded[-1] == 0) {
                    LineNode *temp = new LineNode(trimmedLine);

                    temp->nextLine = lines;
                    lines = temp;
                } else {
                    int i = 0;
                    LineNode *current = lines;
                    LineNode *temp = new LineNode(trimmedLine);
                    while (i < numLinesAdded[-1]-1)
                    {
                        current = current->nextLine;
                        i++;
                    }

                    temp->nextLine = current->nextLine;
                    current->nextLine = temp;
                }
                numLinesAdded[-1] += 1;
            } else {
                int j = 1;
                LineNode *current = lines;
                LineNode *prev = NULL;
                LineNode *temp = new LineNode(trimmedLine);

                while (j <= (refLine+numLinesAdded[refLine]))
                {
                    prev = current;
                    current = current->nextLine;
                    j++;
                }

                if (prev == NULL)
                {
                    temp->nextLine = lines;
                    lines = temp;
                    continue;
                }

                temp->nextLine = current;
                prev->nextLine = temp;
                numLinesAdded[refLine] += 1;
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

    static time_t findLastWriteTime(const char* filePath)
    {
        path pathToFile = path(filePath);
        const auto t = filesystem::last_write_time(pathToFile);
        const auto tp = chrono::time_point_cast<chrono::system_clock::duration>(t - file_time_type::clock::now() + chrono::system_clock::now());

        time_t val = chrono::system_clock::to_time_t(tp);
        return val;
    }
};


void VersionManager::findLatestCommitVersion(char* c)
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

time_t VersionManager::findLastCommitTime()
{
    char c[2];
    VersionManager::findLatestCommitVersion(c);
    string filename = "/.beanVC/logs/";
    filename.push_back(c[0]);
    filename.push_back(c[1]);
    filename += "_log.txt";

    string path = current_path().string() + filename;
    ifstream logFile = ifstream(path, ios_base::in);
    string line;

    getline(logFile, line);
    time_t t = 0;
    
    for(string::iterator it = line.begin(); it != line.end(); it++)
    {
        if (*it == ' ' || *it == '\n') 
        {
            continue;
        }
        int c = *it - '0';
        t *= 10;
        t += c;
    }

    return t;
}

int VersionManager::convertCharToInt(const char* c)
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

void VersionManager::initialize()
{
    path pathname = current_path();
    create_directory(".beanVC");
    create_directory(".beanVC/logs");
    create_directory(".beanVC/objects");
    ofstream lastComitVersion = ofstream(".beanVC/lastCommitVersion.txt", ios_base::out);
    lastComitVersion.write("-1", 2);
    lastComitVersion.close();
    cout << "Initialized empty repository in " << pathname << "..." << endl << endl; 
    cout << "beanVC is a small scale version control management system that is built as a mini clone of git." << endl;
    cout << "It uses delta comparisons to find changes that have been made from one version of the file to the next." << endl;
    cout << "This project uses a lineNode, a node that represents a sentence. A file is represented as linkedList of lineNodes." << endl;
    cout << "A directory is considered to be a multi list with multiple files." << endl;
    cout << "run `./vc help` to see list of commands." << endl;
    return;
}

int VersionManager::checkDuplicatedNewLine(LineNode* node, const char* filename, int lineNumber)
{
    // we start at the start of the file, set i to 1 and keep checking all the lines till i == lineNumber
    // if even one of the lines do not match, then it is a duplicated new line, ie a new line that gave a false positive due it being
    // duplicated.
    // return 1 if it is a new line
    // return 0 if it is not a new line
    int i = 1;
    string line;
    LineNode *current = node;
    ifstream file = ifstream(filename, ios_base::in);
    while (i <= lineNumber)
    {
        getline(file, line);
        if (line != current->data) 
        {
            file.close();
            return i;
        }
        i++;
        current = current->nextLine;
    }

    file.close();
    return 0;
}

void VersionManager::stageChanges()
{
    char c[2];
    findLatestCommitVersion(c);
    int commitVersion = convertCharToInt(c);
    recursive_directory_iterator it = recursive_directory_iterator(current_path());
    string lastCommitLog = "/logs/";
    lastCommitLog += string(c);
    lastCommitLog += "_log.txt";
    ifstream lastCommitLogFile = ifstream(lastCommitLog.c_str(), ios_base::in);

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
                if (it == e)
                {
                    stop = 1;
                } 
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
    } else {
        // if it is not the first commit then we take the file name and create a fileNode of that file
        // first we find the deleted lines, we do this by checking each line in the fileNode and then see if they are present in 
        // in the current file. if they arent we put them in the temp staging file with a -

        // then we check each line in the file and see if it is present in the file node. If not, then it is a new line. In this case, 
        // we search for a line above this line that is closest to this line and present in the fileNode. the index of this file will be
        // the order decider for the newly added line.
        recursive_directory_iterator e = end(it);
        int stop = 0;
        ofstream stagingFile = ofstream(".beanVC/objects/tempStaging.bin", ios_base::out);
        time_t lastCommitTime = VersionManager::findLastCommitTime();

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
                if (it == e)
                {
                    stop = 1;
                }
                continue;
            }

            if (filename.find_first_of(".", 1) == string::npos && (status(it->path()).type() != file_type::directory))
            {
                it++;
                if (it == e)
                {
                    stop = 1;
                }
                continue;
            }

            if (FileNode::findLastWriteTime(filename.c_str()) < lastCommitTime) 
            {
                it++;
                if (it == e)
                {
                    stop = 1;
                }
                continue;
            }

            cout << "Adding Contents of " << filename << endl;

            FileNode fileTillPreviousCommit = FileNode(filename.c_str());


            if (fileTillPreviousCommit.lines == NULL)
            {
                // if this is the case then this file is newly added
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
            } else {
                // first we find the lines that have been deleted.
                // inorder to find the chunks that have been deleted, we take the data of fileNode and then scan each line of currentFile
                // we take the current line and then check the fileNode from starting line. If the match of the line is found, two cases
                // are possible:
                // 1. the node at which the match is found is the next node of the node with which we started 
                // 2. the node at which the match is found is not the next node of the node at which we started
                // if it is case 2, we assume that the lines from startingNode to matchedNode (both exclusive) have been deleted.
                // there is another case which is that we fo not find the line that we were searching for. this would imply that this
                // line is newly added in that case we just ignore it.  
                LineNode *lastMatchedLine = NULL;
                ifstream file = ifstream(filename.c_str(), ios_base::in);
                string currentLine;
                int i = 1;

                stagingFile.write("--START--\n", 10);
                stagingFile.write(filename.c_str(), filename.size());
                stagingFile.write("\n", 1);

                while (getline(file, currentLine))
                {
                    LineNode *current = NULL;

                    if (lastMatchedLine == NULL)
                    {
                        current = fileTillPreviousCommit.lines;
                    } else {
                        current = lastMatchedLine;
                    }
                    int j = i;
                    while (current != NULL)
                    {

                        if (current->data == currentLine)
                        {
                            if (lastMatchedLine == NULL)
                            {
                                LineNode *node = fileTillPreviousCommit.lines;
                                int diff = j-i;
                                while (node != current)
                                {
                                    string s = to_string(j-diff);
                                    stagingFile.write("(", 1);
                                    stagingFile.write(s.c_str(), s.size());
                                    stagingFile.write(")-", 2);
                                    stagingFile.write(node->data.c_str(), node->data.size());
                                    stagingFile.write("\n", 1);
                                    node = node->nextLine;
                                }
                                fileTillPreviousCommit.lines = current;
                            } else if (current != lastMatchedLine) {
                                // if this is the case, we start at lastMatchedLine->nextLine, till node->nextLine == current, and
                                // put all those lines as deleted in our staging file. 
                                LineNode *node = lastMatchedLine->nextLine;
                                int diff = j-i;
                                while (node != current)
                                {
                                    string s = to_string(j-diff);
                                    stagingFile.write("(", 1);
                                    stagingFile.write(s.c_str(), s.size());
                                    stagingFile.write(")-", 2);
                                    stagingFile.write(node->data.c_str(), node->data.size());
                                    stagingFile.write("\n", 1);
                                    node = node->nextLine;
                                } 
                                lastMatchedLine->nextLine = current;
                            }
                            lastMatchedLine = current;
                            i = j;
                            break;
                        }
                        j++;
                        current = current->nextLine;
                    }
                }
                file.close();
                // now that the deleted lines have been added, we check the newly added lines
                // for this we do the opposite of what we did for deleting
                // we check each line of the file and then check if that exists in fileNode
                // if it does, we dont do anything, if it doesnt, then we find store it in staging file with the order decider as 
                // the nearest line to it that exists in the fileNode

                // UPDATED APPROACH
                // if a line is repeated, we need to ensure that the program is able to figure out that the line is still newly inserted
                // for this we do this:
                // if a line is non-repeating and is a new line (ie this line does not exist in fileNode.lines) then we flag it as new line
                // put it in our staging file. Once that is done, we update the fileNode and add the newly inserted line.

                // If a line IS repeating, we need to ensure that the match that we find of the line in fileNode is actually the current
                // line and not some other line much much below the current line. 
                // to ensure this, once a match is found in fileNode for the current line, we check both fileNode AND current file from 
                // the start till that point and ensure that all the lines match. as we are deleting and adding the lines simultaneously
                // in fileNode, they should match. If they dont then that means that we overshot while searching and that it is incfact
                // a newly inserted line.

                // example:
                // fileNode          file currently
                // A                 A
                // D                 E
                // E                 D
                //                   E
                // Here the E is duplicated 
                // When our code reaches the first E in file, it will check and see if this line exists in fileNode. 
                // This will be true as E exists at the end of fileNode.
                // However this is a newly added line, so we compare contents of fileNode and of file till the matched E of fileNode
                // we see that the contents dont match (AD vs AE)
                // so we know that this is a new line
                // once we know that it is a new line, we add it to fileNode
                // fileNode          file currently
                // A                 A
                // E                 E
                // D                 D
                // E                 E

                // Once our code reaches the second E, it recompares the files and finds no inconsisties 
                // This works because we are going top to bottom. If a new line is encountered, we add it to the fileNode hence
                // when we move to the next line, the previous lines of both fileNode and of file currently will match
                // same is true for deleted lines.

                ifstream currentFile = ifstream(filename.c_str(), ios_base::in);
                string line;
                int lastMatchIndex = -1; // this variable stores the line number of last line that was matched in prev commit.
                while (getline(currentFile, line))
                {
                    int isNewLine = 1;

                    LineNode *current = fileTillPreviousCommit.lines;
                    int i = 1;
                    while (current != NULL)
                    {
                        if (current->data == line && (i > lastMatchIndex))
                        {
                            int a = checkDuplicatedNewLine(fileTillPreviousCommit.lines, filename.c_str(), i);
                            if (!a)
                            {
                                lastMatchIndex = i;
                                isNewLine = 0;
                            } else 
                            {
                                lastMatchIndex = a-1;
                            }
                            break;
                        }
                        i++;
                        current = current->nextLine;
                    }

                    if (isNewLine)
                    {
                        // if it is new line, we find the index of the last match and then save it in staging file.
                        stagingFile.write("(", 1);
                        stagingFile.write(to_string(lastMatchIndex).c_str(), to_string(lastMatchIndex).size());
                        stagingFile.write(")", 1);
                        stagingFile.write("+", 1);
                        stagingFile.write(line.c_str(), line.size());
                        stagingFile.write("\n", 1);

                        LineNode *current = fileTillPreviousCommit.lines;
                        int count = 1;
                        while (current != NULL)
                        {
                            if (count == lastMatchIndex)
                            {
                                LineNode *temp = new LineNode(line);
                                temp->nextLine = current->nextLine;
                                current->nextLine = temp;
                                break;
                            }
                            current = current->nextLine;
                            count++;
                        }
                        lastMatchIndex++;
                    }
                }

                stagingFile.write("--END--\n", 8);
            }


            it++;
            if (it == e)
            {
                stop = 1;
            }
        }

    }
}

void VersionManager::commitStagedChanges(const char* m)
{
    string commitMessage = string(m);
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
    int filesChanged = 0;
    int linesAdded = 0;
    int linesDeleted = 0;
    int fileUpdated = 0;
    while(getline(tempStagingFile, line))
    {

        if (line.find_first_of(')') != string::npos)
        {
            fileUpdated = 1;
            if (line[line.find_first_of(')')+1] == '-')
            {
                linesDeleted++;
            } else {
                linesAdded++;
            }
        }

        if ((line == "--END--") && (fileUpdated))
        {
            filesChanged++;
            fileUpdated = 0;
        }

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

    cout << filesChanged << " files updated, " << linesAdded << " lines added(+) and " << linesDeleted << " lines deleted(-)." << endl;

}

void VersionManager::rollback(const char *filename, const char *cv)
{
    string d;
    FileNode f = FileNode(filename, cv);
        
    f.data(d);

    ofstream file = ofstream(filename, ios_base::out);

    file.write(d.c_str(), d.size());
}

void VersionManager::showStatus()
{
    char c[2];

    VersionManager::findLatestCommitVersion(c);

    int i = 0;
    int cv = VersionManager::convertCharToInt(c);

    while (i <= cv) 
    {
        string p = current_path().string();
        p += "/.beanVC/logs/";

        if (i < 10) 
        {
            p.push_back('0');
        }

        p += to_string(i);
        p += "_log.txt";

        string t;
        string m;

        ifstream logfile = ifstream(p, ios_base::in);
        getline(logfile, t);
        getline(logfile, m);

        time_t time = 0;
    
        for(string::iterator it = t.begin(); it != t.end(); it++)
        {
            if (*it == ' ' || *it == '\n') 
            {
                continue;
            }
            int c = *it - '0';
            time *= 10;
            time += c;
        }

        cout << "Commit Version: " << i << endl;
        cout << "Commit message: " << m << endl;
        tm* local_time = localtime(&time);

        // Use std::put_time to format and print the time
        cout << "Time: " << put_time(local_time, "%Y-%m-%d %H:%M:%S") << endl << endl << endl;
        i++;
    }
}

void VersionManager::help()
{
    cout << "Command: init" << endl << "Usage: Initializes an empty beanVC repository." << endl << "Syntax: `./vc init`" << endl << endl;
    cout << "Command: add" << endl << "Usage: Used to stage files for committing." << endl << "Syntax: `./vc add`" << endl << endl;
    cout << "Command: commit" << endl << "Usage: Used to commit staged files." << endl << "Syntax: `./vc commit <commitMessage>`" << endl << endl;
    cout << "Command: log" << endl << "Usage: Used to view all the commit messages along with their version and datetime." << endl << "Syntax: `./vc log`" << endl << endl;
    cout << "Command: show" << endl << "Usage: Used to view the condition of file in a particular commit version." << endl << "Syntax: `./vc show`" << endl << endl;
    cout << "Command: rollback" << endl << "Usage: Used to rollback a file to one of its previous commit versions." << endl << "Syntax: `./vc rollback <filename> <commitVersion>`" << endl << endl;
}

void VersionManager::incrementCommitVersion()
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
        char* message = argv[2];
        if (message == NULL)
        {
            throw MissingCommitMessageError();
        }
        VersionManager::commitStagedChanges(message);
    } else if (argument == "show")
    {
        char *cv = argv[2];
        string d;
        FileNode f = FileNode("test.py", cv);
        
        f.data(d);

        cout << d << endl;

        cout << FileNode::findLastWriteTime("test.py") << endl; 
    } else if (argument == "rollback")
    {
        char *filename = argv[2];
        char *cv = argv[3];

        VersionManager::rollback(filename, cv);
        
    } else if (argument == "log") 
    {
        VersionManager::showStatus();
    } else if (argument == "help") {
        VersionManager::help();
    }
    return 0;
}