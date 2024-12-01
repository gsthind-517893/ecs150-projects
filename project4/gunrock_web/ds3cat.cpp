#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;


int main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << argv[0] << ": diskImageFile inodeNumber" << endl;
    return 1;
  }

  

  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  int inodeNumber = stoi(argv[2]);

  inode_t inode;

  int stat = fileSystem->stat(inodeNumber,&inode);
  if(stat < 0 || inode.type != UFS_REGULAR_FILE){
    cerr << "Error reading file" << endl;
    delete fileSystem;
    delete disk;
    return 1;
  }

  std::cout << "File blocks" << std::endl;
  super_t super;
  fileSystem->readSuperBlock(&super);

  for(int i = 0; i < DIRECT_PTRS; i++){
    if(inode.direct[i] == 0){
      break;
    }
    int blockNum = inode.direct[i] + super.data_region_addr;
    cout << blockNum << endl;
  }
  cout << endl;
  std::cout << "File data" << std::endl;

  char* buffer = new char[inode.size];
  int rd = fileSystem->read(inodeNumber, buffer, inode.size);
  if (rd < 0) {
     cerr << "Error reading file" << endl;
    delete[] buffer;
    delete fileSystem;
    delete disk;
    return 1;
  }
  
  
  std::cout.write(buffer, rd);
  delete[] buffer;


  return 0;
}
