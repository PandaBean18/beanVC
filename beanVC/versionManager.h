#ifndef VERSION_MANAGER_H
#define VERSION_MANAGER_H

#include <string>

class FileNode;

class VersionManager {
    public:

    static int convertCharToInt(const char*);
    static void findLatestCommitVersion(char *);
    static void initialize();
    static void stageChanges();
    static void commitStagedChanges(const char *);

    private:
    static void incrementCommitVersion();
};

#endif