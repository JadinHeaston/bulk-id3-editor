/*
Figure out how to migrate the pathDB array (maybe this can be used for smaller use cases?) to utilize a temporary file - Allowing the option to save to a more permenant caching area.
The 2D arrays have a limitation of ~5000,~5000 objects. This isn't enough.

*/

//Current DB Format:
//Filename_Ext`Title`Artist`Album`Album Artist`Track`Year`Genre`Comment`Length`Size`Last Modified
//Desired format:
//UUID`PATH`SIZE`Title`Artist`Album`Album Artist`Track`Year`Genre`BPM`Comment`Length`Size`Last Modified
//13 columns



//Directory/File management
#include <filesystem>
//iostream
#include <iostream>
//Long term text file management
#include <fstream>
//Temporary file usage.
#include <stdio.h>
//Strings!
#include <string>
//Allows use of vectors.
#include <vector>

//MP3 Tag manipulation
//Documentation: http://id3lib.sourceforge.net/api/index.html
#include "id3/tag.h"

#include <windows.h>

#include <sstream>
//#include "uuid.h"

//Keeps track of what line is being accessed in the temporary file.
//The number refers to what line is ABLE to be read NEXT.
//e.g "0" means that the first, top line can be read if the LINE is grabbed.
int CURRENT_MASTER_DB_LINE = 0;

//Global buffer for reading/writing to files. 9000 seems to work well, I mostly just grab a single line, which is almost always under 9000 characters.
char BUFFER[9000];

//Sets global delimiter used for reading and writing DB files.
std::string delimitingCharacter = "`";


//These variables hold the options for what storage is used.

//Decide whether file searches recursively search through sub-directories.
bool recursiveSearch = true;
//Determines if the cache will be created.
bool useTMPFileCache = true;
//Determines if the cache will be read from initially
bool initFromOfflineDB = false;
//Determines if the offline cache is created.
bool createOfflineDB = true;

//Determines whether the DB will be stored in the string vector array.
//This could cause issues if too big, but should speed things up at times.
//Ram runs into *potential* issues if too many items are used. - This doesn't seem to be an issue if the items are condensed into a single string entry.
bool useRAMVector = true;
std::vector<std::string> RAMDB;


//Global tag variables.
std::string filePath;
std::string fileSize;
std::string titleTag;
std::string artistTag;
std::string albumTag;
std::string albumArtistTag;
std::string trackTag;
std::string genreTag;
std::string yearTag;
std::string bpmTag;
std::string commentTag;





//Declaring prototype functions
void stripTags(std::string givenFile);
void retrieveCover(ID3_Tag& myTag, std::string givenPath = "");
void interpretLineTags(std::string providedLine);
int nthOccurrence(std::string givenString, std::string delimitingCharacter, int nth);
int countFiles(std::string pathToDir, bool recursiveLookup, bool mp3Count);
int countDir(std::string pathToDir, bool recursiveLookup);
std::string getTags(std::string givenFile);
std::string formatFilePath(std::string givenFile);

//Creates a list of files, and their size, and places it into a DB.
//Requires a path to the directory. File DB to place data into (almost always the temporary DB). Whether to recursively look through directories or not. and whether this data should be mirrored to an offline cache.
void updateFileDB(bool recursiveSearch, std::string path, FILE* givenTempDBFile)
{
    //Making the given path an actual boost-usable path. idk why
    std::filesystem::path dirPath(path);
    std::filesystem::directory_iterator end_itr;

    //Creating a stringstream to hold the file size
    std::stringstream temporaryStringStreamFileAndDirSearch;

    //Checking whether to search recursively or not
    if (recursiveSearch) {
        for (std::filesystem::recursive_directory_iterator end, dir(dirPath); dir != end; ++dir) {
            // If it's not a directory, list it. If you want to list directories too, just remove this check.
            if (std::filesystem::is_regular_file(dir->path())) {
                // assign current file name to current_file for editing/saving.
                std::string current_file = dir->path().string();

                //Checking if file is an mp3 file.
                if (formatFilePath(current_file).find(".mp3") != std::string::npos)
                {
                    std::fputs("\n", givenTempDBFile); CURRENT_MASTER_DB_LINE++;
                    //Putting path into array.
                    std::fputs(formatFilePath(current_file.c_str()).c_str(), givenTempDBFile);
                    std::fputs(delimitingCharacter.c_str(), givenTempDBFile);

                    //Obtaining filezise (in bytes) and placing it in the stringstream
                    temporaryStringStreamFileAndDirSearch << std::filesystem::file_size(current_file);
                    std::fputs(temporaryStringStreamFileAndDirSearch.str().c_str(), givenTempDBFile);
                    std::fputs(delimitingCharacter.c_str(), givenTempDBFile);

                    //Getting big list of tags.
                    std::fputs(getTags(current_file).c_str(), givenTempDBFile);

                    //Clear contents...
                    temporaryStringStreamFileAndDirSearch.str(std::string());
                }
            }

        }
    }
    else {
        // cycle through the directory
        for (std::filesystem::directory_iterator itr(dirPath); itr != end_itr; ++itr)
        {
            // If it's not a directory, list it. If you want to list directories too, just remove this check.
            //if (boost::filesystem::is_regular_file(itr->path())) {
                // assign current file name to current_file and echo it out to the console.
            std::string current_file = itr->path().string();

            //Checking if file is an mp3 file.
            if (formatFilePath(current_file).find(".mp3") != std::string::npos)
            {
                std::fputs("\n", givenTempDBFile); CURRENT_MASTER_DB_LINE++;

                //Putting path into array.
                std::fputs(formatFilePath(current_file.c_str()).c_str(), givenTempDBFile);
                std::fputs(delimitingCharacter.c_str(), givenTempDBFile);

                //Obtaining filezise (in bytes) and placing it in the stringstream
                temporaryStringStreamFileAndDirSearch << std::filesystem::file_size(current_file);
                std::fputs(temporaryStringStreamFileAndDirSearch.str().c_str(), givenTempDBFile);
                std::fputs(delimitingCharacter.c_str(), givenTempDBFile);

                //Getting big list of tags.
                std::fputs(getTags(current_file).c_str(), givenTempDBFile);

                //Clear contents...
                temporaryStringStreamFileAndDirSearch.str(std::string());
            }
            //}
        }
    }
}

//Allows for mirroring to offline DB.
//void updateFileDB(bool recursiveSearch, bool offlineCache, std::string path, FILE* givenTempDBFile, std::ofstream givenOfflineDBFile)
//{
//    //Making the given path an actual boost-usable path. idk why
//    boost::filesystem::path dirPath(path);
//    boost::filesystem::directory_iterator end_itr;
//
//    //Creating a stringstream to hold the file size
//    std::stringstream temporaryStringStreamFileAndDirSearch;
//
//    //Checking whether to search recursively or not
//    if (recursiveSearch) {
//        for (boost::filesystem::recursive_directory_iterator end, dir(dirPath); dir != end; ++dir) {
//            // If it's not a directory, list it. If you want to list directories too, just remove this check.
//            if (boost::filesystem::is_regular_file(dir->path())) {
//                std::fputs("\n", givenTempDBFile); CURRENT_MASTER_DB_LINE++;
//                // assign current file name to current_file and echo it out to the console.
//                std::string current_file = dir->path().string();
//                //Putting path into array.
//                std::fputs(current_file.c_str(), givenTempDBFile);
//                std::fputs(delimitingCharacter.c_str(), givenTempDBFile);
//                //Obtaining filezise (in bytes) and placing it in the stringstream
//                temporaryStringStreamFileAndDirSearch << boost::filesystem::file_size(current_file);
//                std::fputs(temporaryStringStreamFileAndDirSearch.str().c_str(), givenTempDBFile);
//                //Clear contents...
//                temporaryStringStreamFileAndDirSearch.str(std::string());
//            }
//
//        }
//    }
//    else {
//        // cycle through the directory
//        for (boost::filesystem::directory_iterator itr(dirPath); itr != end_itr; ++itr)
//        {
//            // If it's not a directory, list it. If you want to list directories too, just remove this check.
//            if (boost::filesystem::is_regular_file(itr->path())) {
//                std::fputs("\n", givenTempDBFile); CURRENT_MASTER_DB_LINE++;
//                // assign current file name to current_file and echo it out to the console.
//                std::string current_file = itr->path().string();
//                //Putting path into array.
//                std::fputs(current_file.c_str(), givenTempDBFile);
//                std::fputs(delimitingCharacter.c_str(), givenTempDBFile);
//                //Obtaining filezise (in bytes) and placing it in the stringstream
//                temporaryStringStreamFileAndDirSearch << boost::filesystem::file_size(current_file);
//                std::fputs(temporaryStringStreamFileAndDirSearch.str().c_str(), givenTempDBFile);
//                //Clear contents...
//                temporaryStringStreamFileAndDirSearch.str(std::string());
//            }
//        }
//    }
//
//    if (offlineCache)
//    {
//        std::rewind(givenTempDBFile);
//        std::string currentReadLine;
//
//        while (!feof(givenTempDBFile))
//        {
//
//            currentReadLine = std::fgets(BUFFER, sizeof(BUFFER), givenTempDBFile);
//
//            givenOfflineDBFile << currentReadLine;
//        }
//    }
//}



//Copies data from the offline DB to the master tmp DB file.
void copyFromOfflineDB(FILE* givenTMPCacheDBFile, std::string offlineDBPath)
{
    //Holds line being read.
    std::string currentReadLine;

    //FIGURE THIS OUT
    std::ifstream offlineDBFile(offlineDBPath);
    bool isEmpty = offlineDBFile.peek() == EOF;

    //Check if file is not empty/is available. Read each line, until EOF is declared.
    if (!isEmpty)
    {
        while (getline(offlineDBFile, currentReadLine))
        {
            std::fputs(currentReadLine.c_str(), givenTMPCacheDBFile);
            std::fputs("\n", givenTMPCacheDBFile); CURRENT_MASTER_DB_LINE++;
        }
        std::rewind(givenTMPCacheDBFile);
        offlineDBFile.close();
    }
    else
        std::cout << "Failed to open DB: " + offlineDBPath << std::endl;

}

//Copy the contents of the master tmp DB file to the vector stored in RAM
void copyToRAM(FILE* givenTMPCacheDBFile = NULL, std::string offlineDB = "")
{
    //CHECK WHICH (or both) COPY TO RAM. CREATE PRIORITY EVALUATION FUNCTION.



    //Holds line being read.
    std::string currentReadLine;

    if (!givenTMPCacheDBFile == NULL)
    {
        //Copy from temp to ram

        std::rewind(givenTMPCacheDBFile);

        //Read each line, until EOF is declared.
        while (!feof(givenTMPCacheDBFile) && !fgets(BUFFER, sizeof(BUFFER), givenTMPCacheDBFile) == NULL) //Grab the next line from the master tmp DB file. Place it into buffer. Make sure not eof
        {
            //Place buffer into string.
            currentReadLine = BUFFER;

            //Send string to vector.
            RAMDB.push_back(currentReadLine);
        }
        //Clear buffer
        memset(BUFFER, 0, sizeof(BUFFER));
    }
    else if (!offlineDB.empty())
    {
        //Opening offlineDBFile
        std::ifstream offlineDBFile(offlineDB);
        //Check if it is empty.
        bool isEmpty = offlineDBFile.peek() == EOF;

        //Check if file is not empty/is available. Read each line, until EOF is declared.
        if (!isEmpty)
        {
            while (getline(offlineDBFile, currentReadLine))//Get line from offline DB
                RAMDB.push_back(currentReadLine); //Add line to vector
        }
        else
            std::cout << "Failed to open DB (or is empty): " + offlineDB << std::endl;

        //Close offline DB File.
        offlineDBFile.close();

    }
    //Resize vector. Just to be safe.
    RAMDB.resize(RAMDB.size());









    //for (int testIter = 0; testIter < RAMDB.size(); testIter++)
    //{
    //    std::cout << RAMDB[testIter] << std::endl;
    //}
    //

    //std::cout << RAMDB.capacity() << std::endl;
}

//Copies content to temporary file.
void copyToTemp(FILE* givenTMPCacheDBFile, std::string offlineDB = "")
{
    //MIRRORS COPY TO RAM FORMAT
}

//Copies content to offlineDB.
void copyToOfflineDB(FILE* givenTMPCacheDBFile = NULL, std::string offlineDB = "")
{
    if (offlineDB == "")
    {
        std::cout << "copyToOFflineDB Function failure. No offlineDB provided." << std::endl;
        return;
    }
    else
    {
        //Opening offlineDBFile
        std::ofstream offlineDBFile(offlineDB);

        //Holds line being read.
        std::string currentReadLine;

        //Verifying DB is open.
        if (offlineDBFile.is_open())
        {

            //Read each item of vector.
            for (int testIter = 0; testIter < (int)RAMDB.size(); testIter++)
            {
                //Grab item...
                currentReadLine = RAMDB[testIter];

                
                //Potentially a useless check, but when initFromOfflineDB is false an extra blank line between each entry is inserted into offlineDB. - I couldn't find where/why. This fixes it.
                if (initFromOfflineDB == true)
                {
                    if (testIter == (int)RAMDB.size() - 1)
                    {
                        offlineDBFile << currentReadLine;
                    }
                    else
                        offlineDBFile << currentReadLine << std::endl;
                }
                else
                    offlineDBFile << currentReadLine;

            }
            //Close file.
            offlineDBFile.close();
        }
        else
        {
            std::cout << "Failed to open DB: " + offlineDB << std::endl;
        }
    }
    //END
}

//Synchronizes the databases.
//This will evaluate the priority of the databases, and copy across as needed appropriately.
//Optional Paramenter (optParam) will contain what the function is inteded to be doing (Copying from main source to sub-sources, reading from offline DB to populate more prioritized sources, anything else that I think of)
void synchronizeDB(FILE* TMPFile, std::string offlineDB, int optIntParam = NULL, std::string optStringParam = "")
{
    //optParam options:
    //0 - Read from the provided offline DB and populate other in-use options.
    //1 - Synchronize DB's flowing down the priority list.

    //Holds line being read.
    std::string currentReadLine;

    //Occur if told to initialize from an offline DB.
    if (optIntParam == 0 && initFromOfflineDB)
    {
        //SHIT BE GETTING CAUGHT HERE IF THE ! IS MISSING
        if (!useRAMVector && useTMPFileCache)
        {
            //Copy from offline DB to RAM and TMP File
            copyToRAM(TMPFile, offlineDB);
        }
        else if (useRAMVector)
        {
            //Copy from the offline DB to RAM.
            copyToRAM(NULL, offlineDB);
        }
        else if (useTMPFileCache && !TMPFile == NULL)
        {
            //Copy from offline DB to TMP File
            copyToTemp(TMPFile, offlineDB);
        }
        else
            std::cout << "INVALID INITIALIZATION. USE RAMVECTOR OR TEMP FILE FOR NOW. NO FUNCTIONALITY FOR SINGLE OFFLINE CACHE (DANGEROUS?)" << std::endl;
    }
    else if (optIntParam == 1)
    {
        //Copy to ram
        copyToRAM(TMPFile);
        //Copy to offline
        copyToOfflineDB(NULL, offlineDB);
    }



}

int main(int argc, char* argv[]) /*try*/
{
  
    //ofstream hello;
    //hello.open("TEMPGUY.txt");
    //std::cout << getTags("Anyway with David Spekter - FWLR.mp3") << std::endl;
    //stripTags("(bonus track) - ye..mp3");
    //hello.close();
    //system("PAUSE");

    //Provide path desired for searching for items.
    //std::string givenPath = formatFilePath("TESTDIR");
    std::string givenPath = formatFilePath("TESTDIR");

    //Provide location for offline DB cache.
    std::string offlineDBPath = formatFilePath("TESTDIR/DB.txt");

    //Initializing counters.    
    int fileCount, mp3Count, dirCount;

    //CREATE TEMPORARY FILE
    //Handle pointer
    FILE* TMPCacheDBFile;

    //Creating temp file itself.
    tmpfile_s(&TMPCacheDBFile);

    //Verifying no errors occured.
    if (TMPCacheDBFile == NULL)
    {
        perror("Error creating temporary file");
        exit(1);
    }

    //Verifying path is legitimate. Closing program if it is not.
    if (!std::filesystem::is_directory(givenPath))
    {
        std::cout << "Path provided was NOT found. (" << givenPath << ")" << std::endl;
        return 0;
    }

    //Assigning resulting counts.
    fileCount = countFiles(givenPath, recursiveSearch, false);
    mp3Count = countFiles(givenPath, recursiveSearch, true);
    dirCount = countDir(givenPath, recursiveSearch);

    //Temporarily displaying them for debugging in future. Shows TOTAL files in directory.
    std::cout << "File Count: " << fileCount << std::endl;
    std::cout << "MP3 Count: " << mp3Count << std::endl;
    std::cout << "Directory Count: " << dirCount << std::endl;


    //Decide if the master file should be populated based off of offline DB or needs to be properly searched and created.
    if (initFromOfflineDB)
        synchronizeDB(TMPCacheDBFile, offlineDBPath, 0); //Initializing databases, which holds the full string lines in a string vector.
    else
    {
        //Add header columns!
        std::string columnsLine = "PATH`SIZE`TITLE`ARTIST`ALBUM`ALBUMARTIST`TRACKNUM`GENRE`YEAR`BPM`COMMENT";
        std::fputs(columnsLine.c_str(), TMPCacheDBFile);
        updateFileDB(recursiveSearch, givenPath, TMPCacheDBFile);
    }


    //Copying stuff to offline DB file.
    if (createOfflineDB)
        synchronizeDB(TMPCacheDBFile, offlineDBPath, 1);

    //Closing files and program.
    fclose(TMPCacheDBFile);

    

    system("PAUSE");

    return 0;
} /*catch (const boost::filesystem::filesystem_error& e) {
    std::cout << e.what() << '\n';
    system("PAUSE");
}*/


//returns the position of the nth occurance of a given character.
//Asks for a string to be searched, character to find, and what count you desire.
int nthOccurrence(std::string givenString, std::string delimitingCharacter, int nth)
{
    //Init counters.
    int stringPosition = 0;
    int count = 0;


    //Iterate through find calls.
    while (count != nth)
    {
        //Call find, returning the first found matching character. Keep that position, use it for future calls.
        stringPosition = givenString.find(delimitingCharacter, stringPosition);
        //Check if the string has ended. End function, if so.
        if (stringPosition == std::string::npos)
            return -1;
        //Iterate counter
        count++;
    }
    //Return character position.
    return stringPosition;
}


//Functions asks for a path to directory and will return number of files. - Also asks for a T/F bool determining whether a search should be recursive.
int countFiles(std::string pathToDir, bool recursiveLookup, bool mp3Count)
{

    int fileCounter = 0;
    int directoryCounter = 0;

    //Making path a path it likes.
    std::filesystem::path dirPath(pathToDir);

    //Checking if user wanted a recursive lookup.
    if (recursiveLookup) {
        //boost::filesystem::recursive_directory_iterator end_itr;
        //Performing recursive searching...
        for (std::filesystem::recursive_directory_iterator end, dir(pathToDir); dir != end; ++dir) {
            //Count all items.
            directoryCounter++;
            // If it's not a directory, list it. If you want to list directories too, just remove this check.
            if (std::filesystem::is_regular_file(dir->path())) {
                if (mp3Count)
                {
                    // assign current file name to current_file for editing/saving.
                    std::string current_file = dir->path().string();
                    if (formatFilePath(current_file).find(".mp3") != std::string::npos)
                    {
                        //Count the MP3 file
                        fileCounter++;
                    }
                }
                else
                {
                    //Count files specifically.
                    fileCounter++;
                }
            }
        }
    }
    else
    {
        std::filesystem::directory_iterator end_itr;
        //Cycle through the GIVEN directory. No deeper. Any directories are just directories directly viewable.
        for (std::filesystem::directory_iterator itr(dirPath); itr != end_itr; ++itr)
        {
            //Count all items.
            directoryCounter++;
            // If it's not a directory, list it. If you want to list directories too, just remove this check.
            if (std::filesystem::is_regular_file(itr->path())) {
                //Count files specifically.
                fileCounter++;
            }
        }
    }
    //Return files!
    return fileCounter;
}

//Functions asks for a path to directory and will return number of files. - Also asks for a T/F bool determining whether a search should be recursive.
int countDir(std::string pathToDir, bool recursiveLookup)
{

    int fileCounter = 0;
    int directoryCounter = 0;

    //Making path a path it likes.
    std::filesystem::path dirPath(pathToDir);

    //Checking if user wanted a recursive lookup.
    if (recursiveLookup) {
        //Performing recursive searching...
        for (std::filesystem::recursive_directory_iterator end, dir(pathToDir); dir != end; ++dir) {
            //Count all items.
            directoryCounter++;
            // If it's not a directory, list it. If you want to list directories too, just remove this check.
            if (std::filesystem::is_regular_file(dir->path())) {
                //Count files specifically.
                fileCounter++;
            }
        }
    }
    else {
        std::filesystem::directory_iterator end_itr;
        //Cycle through the GIVEN directory. No deeper. Any directories are just directories directly viewable.
        for (std::filesystem::directory_iterator itr(dirPath); itr != end_itr; ++itr)
        {
            //Count all items.
            directoryCounter++;
            // If it's not a directory, list it. If you want to list directories too, just remove this check.
            if (std::filesystem::is_regular_file(itr->path())) {
                //Count files specifically.
                fileCounter++;
            }
        }
    }
    //Finding difference between ALL items, and literal files. Leaves directories.
    directoryCounter = directoryCounter - fileCounter;
    //Return files!
    return directoryCounter;
}

//Takes a string and removes "/" and places "\\".
std::string formatFilePath(std::string givenFile)
{
    //Formating givenFile to have the slashes ALL be \ instead of mixed with / and \.
    for (int i = 0; i < (int)givenFile.length(); ++i)
    {
        if (givenFile[i] == '/')
            givenFile[i] = '\\';
    }

    return givenFile;
}

//Takes a provided line of tags/information and separates them out into individual variables.
void interpretLineTags(std::string providedLine)
{
    std::string temporaryString = providedLine;
    int stringPosition = 0;



    //Init tag holders.
    filePath = "";
    fileSize = "";
    titleTag = "";
    artistTag = "";
    albumTag = "";
    albumArtistTag = "";
    trackTag = "";
    yearTag = "";
    genreTag = "";
    bpmTag = "";
    commentTag = "";

    for (int i = 0; i < 13; i++)
    {
        //std::cout << stringPosition << std::endl;
        //std::cout << temporaryString << std::endl;
        switch (i)
        {
        case 0:
            //Finding where first delimiter is.
            stringPosition = nthOccurrence(temporaryString, delimitingCharacter, 1);
            
            filePath = temporaryString.substr(0, stringPosition);
            break;
        case 1:
            //Finding where first delimiter is.
            stringPosition = nthOccurrence(temporaryString, delimitingCharacter, 1);
            fileSize = temporaryString.substr(0, stringPosition);
            break;
        case 2:
            //Finding where first delimiter is.
            stringPosition = nthOccurrence(temporaryString, delimitingCharacter, 1);
            titleTag = temporaryString.substr(0, stringPosition);
            break;
        case 3:
            //Finding where first delimiter is.
            stringPosition = nthOccurrence(temporaryString, delimitingCharacter, 1);
            artistTag = temporaryString.substr(0, stringPosition);
            break;
        case 4:
            //Finding where first delimiter is.
            stringPosition = nthOccurrence(temporaryString, delimitingCharacter, 1);
            albumTag = temporaryString.substr(0, stringPosition);
            break;
        case 5:
            //Finding where first delimiter is.
            stringPosition = nthOccurrence(temporaryString, delimitingCharacter, 1);
            albumArtistTag = temporaryString.substr(0, stringPosition);
            break;
        case 6:
            //Finding where first delimiter is.
            stringPosition = nthOccurrence(temporaryString, delimitingCharacter, 1);
            trackTag = temporaryString.substr(0, stringPosition);
            break;
        case 7:
            //Finding where first delimiter is.
            stringPosition = nthOccurrence(temporaryString, delimitingCharacter, 1);
            genreTag = temporaryString.substr(0, stringPosition);
            break;
        case 8:
            //Finding where first delimiter is.
            stringPosition = nthOccurrence(temporaryString, delimitingCharacter, 1);
            yearTag = temporaryString.substr(0, stringPosition);
            break;
        case 9:
            //Finding where first delimiter is.
            stringPosition = nthOccurrence(temporaryString, delimitingCharacter, 1);
            bpmTag = temporaryString.substr(0, stringPosition);
            break;
        case 10:
            //Finding where first delimiter is.
            stringPosition = nthOccurrence(temporaryString, delimitingCharacter, 1);
            commentTag = temporaryString.substr(0, stringPosition);
            break;
            //case 11:
            //    //Finding where first delimiter is.
            //    stringPosition = nthOccurrence(temporaryString, delimitingCharacter, 1);
            //    genreTag = temporaryString.substr(0, stringPosition);
            //    break;
            //case 12:
            //    //Finding where first delimiter is.
            //    stringPosition = nthOccurrence(temporaryString, delimitingCharacter, 1);
            //    genreTag = temporaryString.substr(0, stringPosition);
            //    break;
        }
        //Erasing first value grabbed from the string.
        temporaryString.erase(0, stringPosition + 1);
    }

    //return "";
}

//Grabs any desired tags and places them into the temp DB.
std::string getTags(std::string givenFile)
{

    //https://help.mp3tag.de/main_tags.html

    std::string finalString;

    char textBuff[4096];

    //Initializing the array to format it properly
    //Skipping this step results in ID3lib reading bad data, likely due to it not adding a \0 termination at the end of the data stream.
    memset(textBuff, 0, sizeof(textBuff));

    //Tag handle.
    ID3_Tag myTag;

    //Triple check it's an mp3. - Surface level check tbh
    if (givenFile.find(".mp3"))
        myTag.Link(givenFile.c_str(), ID3TT_ID3V2);

    ID3_Frame* myFrame;





    //retrieveCover(myTag);
    //return "hello";




    //Check the tag has V2 tags.
    if (myTag.HasV2Tag())
    {
        //TITLE
        if (myFrame = myTag.Find(ID3FID_TITLE)) { if (NULL != myFrame) { myFrame->Field(ID3FN_TEXT).Get(textBuff, 1024); } }
        finalString.append(textBuff);
        finalString.append(delimitingCharacter.c_str());
        //Emptying character array again...
        memset(textBuff, 0, sizeof(textBuff));

        //LEAD ARTIST
        if (myFrame = myTag.Find(ID3FID_LEADARTIST)) { if (NULL != myFrame) { myFrame->Field(ID3FN_TEXT).Get(textBuff, 1024); } }
        finalString.append(textBuff);
        finalString.append(delimitingCharacter.c_str());
        //Emptying character array again...
        memset(textBuff, 0, sizeof(textBuff));

        //ALBUM
        if (myFrame = myTag.Find(ID3FID_ALBUM)) { if (NULL != myFrame) { myFrame->Field(ID3FN_TEXT).Get(textBuff, 1024); } }
        finalString.append(textBuff);
        finalString.append(delimitingCharacter.c_str());
        //Emptying character array again...
        memset(textBuff, 0, sizeof(textBuff));

        //BAND
        if (myFrame = myTag.Find(ID3FID_BAND)) { if (NULL != myFrame) { myFrame->Field(ID3FN_TEXT).Get(textBuff, 1024); } }
        finalString.append(textBuff);
        finalString.append(delimitingCharacter.c_str());
        //Emptying character array again...
        memset(textBuff, 0, sizeof(textBuff));

        //TRACK NUMBER
        if (myFrame = myTag.Find(ID3FID_TRACKNUM)) { if (NULL != myFrame) { myFrame->Field(ID3FN_TEXT).Get(textBuff, 1024); } }
        finalString.append(textBuff);
        finalString.append(delimitingCharacter.c_str());
        //Emptying character array again...
        memset(textBuff, 0, sizeof(textBuff));

        //GENRE
        if (myFrame = myTag.Find(ID3FID_CONTENTTYPE)) { if (NULL != myFrame) { myFrame->Field(ID3FN_TEXT).Get(textBuff, 1024); } }
        finalString.append(textBuff);
        finalString.append(delimitingCharacter.c_str());
        //Emptying character array again...
        memset(textBuff, 0, sizeof(textBuff));

        //YEAR
        if (myFrame = myTag.Find(ID3FID_YEAR)) { if (NULL != myFrame) { myFrame->Field(ID3FN_TEXT).Get(textBuff, 1024); } }
        finalString.append(textBuff);
        finalString.append(delimitingCharacter.c_str());
        //Emptying character array again...
        memset(textBuff, 0, sizeof(textBuff));

        //BPM
        if (myFrame = myTag.Find(ID3FID_BPM)) { if (NULL != myFrame) { myFrame->Field(ID3FN_TEXT).Get(textBuff, 1024); } }
        finalString.append(textBuff);
        finalString.append(delimitingCharacter.c_str());
        //Emptying character array again...
        memset(textBuff, 0, sizeof(textBuff));

        //COMMENT
        if (myFrame = myTag.Find(ID3FID_COMMENT)) { if (NULL != myFrame) { myFrame->Field(ID3FN_TEXT).Get(textBuff, 1024); } }
        finalString.append(textBuff);

        //FINAL CHARACTER NOT NEEDED.
        //finalString.append(delimitingCharacter.c_str());
        //Emptying character array again...
        memset(textBuff, 0, sizeof(textBuff));

        //NO SHOT THIS SHIT FIND SOMETHIN'
        ////DATE
        //if (myFrame = myTag.Find(ID3FID_DATE)) { if (NULL != myFrame) { myFrame->Field(ID3FN_TEXT).Get(textBuff, 1024); } }
        //finalString.append(textBuff);
        //finalString.append(delimitingCharacter.c_str());
        ////Emptying character array again...
        //memset(textBuff, 0, sizeof(textBuff));
        ////ENCODED BY
        //if (myFrame = myTag.Find(ID3FID_ENCODEDBY)) { if (NULL != myFrame) { myFrame->Field(ID3FN_TEXT).Get(textBuff, 1024); } }
        //finalString.append(textBuff);
        //finalString.append(delimitingCharacter.c_str());
        ////Emptying character array again...
        //memset(textBuff, 0, sizeof(textBuff));
        ////TIME
        //if (myFrame = myTag.Find(ID3FID_TIME)) { if (NULL != myFrame) { myFrame->Field(ID3FN_TEXT).Get(textBuff, 1024); } }
        //finalString.append(textBuff);
        //finalString.append(delimitingCharacter.c_str());
        ////Emptying character array again...
        //memset(textBuff, 0, sizeof(textBuff));
        ////LENGTH
        //if (myFrame = myTag.Find(ID3FID_SONGLEN)) { if (NULL != myFrame) { myFrame->Field(ID3FN_TEXT).Get(textBuff, 1024); } }
        //finalString.append(textBuff);
        //finalString.append(delimitingCharacter.c_str());
        ////Emptying character array again...
        //memset(textBuff, 0, sizeof(textBuff));
        ////MEDIA TYPE
        //if (myFrame = myTag.Find(ID3FID_MEDIATYPE)) { if (NULL != myFrame) { myFrame->Field(ID3FN_TEXT).Get(textBuff, 1024); } }
        //finalString.append(textBuff);
        //finalString.append(delimitingCharacter.c_str());
        ////Emptying character array again...
        //memset(textBuff, 0, sizeof(textBuff));
        ////UNIQUE FILE ID
        //if (myFrame = myTag.Find(ID3FID_UNIQUEFILEID)) { if (NULL != myFrame) { myFrame->Field(ID3FN_TEXT).Get(textBuff, 1024); } }
        //finalString.append(textBuff);
        //finalString.append(delimitingCharacter.c_str());
        ////Emptying character array again...
        //memset(textBuff, 0, sizeof(textBuff));

    }





    //Emptying character array again...
    memset(textBuff, 0, sizeof(textBuff));

    //Unload file.
    myTag.Clear();
    return finalString;
}

//Removes all tags from a provided file. This includes extended tags, and album art information.
void stripTags(std::string givenFile)
{
    //Format the path propery. In case it isn't.
    formatFilePath(givenFile);

    //Create tag.
    ID3_Tag myTag;

    //Triple check it's an mp3 file.
    if (givenFile.find(".mp3"))
    {
        //link with a v1 tag. Check if it has one, strip it if it does.
        myTag.Link(givenFile.c_str(), ID3TT_ID3V1);
        if (myTag.HasV1Tag())
        {
            //Strip that shit.
            myTag.Strip();
            //Unload file.
            myTag.Clear();
        }
        //link with a v2 tag. Check if it has one, strip it if it does.
        myTag.Link(givenFile.c_str(), ID3TT_ID3V2);
        if (myTag.HasV2Tag())
        {
            //Strip that stuff.
            myTag.Strip();
            //Unload file.
            myTag.Clear();
        }
    }


    ////NOT NEEDED WHEN REMOVING TAGS. ONLY WRITING CHANGED TAGS.
    //// SUCCESSFULLY SETS IMAGES/TAGS.
    ////if (myFrame = myTag.Find(ID3FID_PICTURE)) { if (NULL != myFrame) { myFrame->Field(ID3FN_DATA).FromFile("X.jpg"); } }

    ////Setup the rendering parameters
    //myTag.SetUnsync(false);
    //myTag.SetExtendedHeader(true);
    //myTag.SetCompression(true);
    //myTag.SetPadding(true);

    ////Update tag.
    //myTag.Update();
    //myTag.Clear();


}

//Retrieves requested cover, using either a path to the file or a direct tag handle.
void retrieveCover(ID3_Tag& myTag, std::string givenPath)
{
    //Holds the raw binary of the 
    //This path wants a fucking forward slash. IDK.
    const char* temporaryCoverFile = "Cache/AlbumArt Cache/TEMP.bin";

    ID3_Frame* myFrame;
    //PICTURE ***** - File name for the saved picture needs to be determined somehow. - Maybe time for globally unique IDs?
    if (myFrame = myTag.Find(ID3FID_PICTURE)) { if (NULL != myFrame) { myFrame->Field(ID3FN_DATA).ToFile(temporaryCoverFile); } }
}


////NEEDS WORK
//std::string readDBLine(FILE* givenDBFile, int lineAmount, int currentLine)
//{
//    //This string holds the current line being access/last line read.
//    std::string currentlyBeingReadLine;
//    //Init beginningLine to be the current TempDB line.
//    int beginningLine;
//    beginningLine = currentLine;
//
//
//
//
//    //currentReadLine = std::fgets(BUFFER, sizeof(BUFFER), tmpDBFile);
//    //currentReadLine = std::fgets(BUFFER, sizeof(BUFFER), tmpDBFile);
//    //int pos;
//    //std::string words;
//    //while ((pos = currentReadLine.find(delimiterCharacter)) != std::string::npos) {
//    //    //Setting pos to "pos+1" will INCLUDE the delimiter at the end of the string.
//    //    words = currentReadLine.substr(0, pos);
//    //    std::cout << words;
//    //    currentReadLine.erase(0, pos + delimiterCharacter.length());
//    //}
//
//
//
//    //IF LINE AMOUNT IS NOT 0
//    for (; currentLine < beginningLine; currentLine++)
//    {
//        std::fgets(BUFFER, sizeof(BUFFER), givenDBFile);
//    }
//    return 0;
//}
////READ CELL?
////std::string readDBLine(FILE* DBFile, int columnNum, int rowNum, std::string returnThing[])
////{
////    //Close file, assuming it is being written to
////    return 0;
////}
