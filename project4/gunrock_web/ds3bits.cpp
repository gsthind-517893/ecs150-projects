#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;


int main(int argc, char *argv[]) {
  if (argc != 2) {
    cerr << argv[0] << ": diskImageFile" << endl;
    return 1;
  }
  
  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);

  super_t super;
  fileSystem->readSuperBlock(&super);

  cout << "Super" << endl;
  cout << "inode_region_addr " << super.inode_region_addr << endl;
  cout << "inode_region_len " << super.inode_region_len << endl;
  cout << "num_inodes " << super.num_inodes << endl;
  cout << "data_region_addr " << super.data_region_addr << endl;
  cout << "data_region_len " << super.data_region_len << endl;
  cout << "num_data " << super.num_data << endl;
  cout << endl;

  std::cout << "Inode bitmap" << std::endl;
  int inodeBitSize = super.inode_bitmap_len * UFS_BLOCK_SIZE;
  unsigned char* inodeBitmap = new unsigned char[inodeBitSize];
  fileSystem->readInodeBitmap(&super,inodeBitmap);
  for(int i = 0; i < inodeBitSize;i++){
    cout << static_cast<unsigned int>(inodeBitmap[i]) << " ";
  }
  cout << endl << endl;
  delete[] inodeBitmap;

  std::cout << "Data bitmap" << std::endl;
  int dataSize = super.data_bitmap_len * UFS_BLOCK_SIZE;
  unsigned char* dataBitmap = new unsigned char[dataSize];
  fileSystem->readDataBitmap(&super,dataBitmap);
  for(int i = 0; i < dataSize; i++){
    std::cout << static_cast<unsigned int>(dataBitmap[i]) << " ";
  }
  std::cout << std::endl;
  delete[] dataBitmap;
  
  return 0;
}
