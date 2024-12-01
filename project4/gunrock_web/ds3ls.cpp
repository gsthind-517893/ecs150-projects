#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

#include "StringUtils.h"
#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;


//Use this function with std::sort for directory entries
bool compareByName(const dir_ent_t& a, const dir_ent_t& b) {
  return std::strcmp(a.name, b.name) < 0;
}

vector<string> pathSplit(const string& path){
  vector<string> parts;

  size_t start = 0;
  size_t end = 0;

  while((end = path.find('/', start)) != string::npos){
    if(end != start){
      parts.push_back(path.substr(start,end-start));
    }
    start = end +1;
  }
  if(start < path.length()){
    parts.push_back(path.substr(start));
  }
  return parts;
}


int main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << argv[0] << ": diskImageFile directory" << endl;
    cerr << "For example:" << endl;
    cerr << "    $ " << argv[0] << " tests/disk_images/a.img /a/b" << endl;
    return 1;
  }

  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  string directory = string(argv[2]);

  vector<string> components = pathSplit(directory);
  int inodeNum = UFS_ROOT_DIRECTORY_INODE_NUMBER;
  for(const auto& dir : components){

    int lookupVal = fileSystem->lookup(inodeNum,dir);
    if(lookupVal < 0){
      cerr << "Directory not found" << endl;
      exit(1);
    }
    else{

      inodeNum = lookupVal;
    }
  }

  inode_t inode;

  int stat = fileSystem->stat(inodeNum,&inode);
  if(stat < 0){
     cerr << "Directory not found" << endl;
    delete fileSystem;
    delete disk;
    return 1;
  }
  

  vector<dir_ent_t> entries;
  char* buff = new char[inode.size];
  int rd = fileSystem->read(inodeNum,buff,inode.size);
  if(rd<0){
    cerr << "Error: invalid read return" << endl;
    exit(1);
  }

  int numEntries = rd/ sizeof(dir_ent_t);
  dir_ent_t* dirEntries = reinterpret_cast<dir_ent_t*>(buff);
  for (int i = 0; i < numEntries; ++i) {
    std::string entryName(dirEntries[i].name);
    entries.push_back(dirEntries[i]);
  } 

  delete[] buff;

  std::sort(entries.begin(), entries.end(), [](const dir_ent_t& a, const dir_ent_t& b) {
    return std::strcmp(a.name, b.name) < 0;
  });

  for (const auto& entry : entries) {
    inode_t entryInode;
    fileSystem->stat(entry.inum, &entryInode);
    std::cout << entry.inum << "\t" << entry.name << std::endl;
  }
  return 0;
}
