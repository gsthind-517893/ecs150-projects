#include <iostream>
#include <string>
#include <vector>
#include <assert.h>

#include "LocalFileSystem.h"
#include "ufs.h"
#include <cstring>

using namespace std;


LocalFileSystem::LocalFileSystem(Disk *disk) {
  this->disk = disk;
}

void LocalFileSystem::readSuperBlock(super_t *super) {

  //UFS_BLOCK_SIZE is 4096 according to header ufs.h we can also just use 4096 directly
  unsigned char buffer[UFS_BLOCK_SIZE];

  disk->readBlock(0,buffer);

  if(sizeof(*super) > UFS_BLOCK_SIZE){
    cerr << "Error: super_t exceeds block size" << endl;
    exit(1);
  }

  memcpy(super,buffer,sizeof(*super));

}

void LocalFileSystem::readInodeBitmap(super_t *super, unsigned char *inodeBitmap) {

  //grab the number of blocks using the function from disk class
  int numBlocks = disk->numberOfBlocks();
  unsigned char buffer[UFS_BLOCK_SIZE];

  //do error checking
  if(super->inode_bitmap_len <= 0){

    cerr << "Error: bitmap len can not be negative" << endl;
    exit(1);
  }

  if(super->inode_bitmap_addr + super->inode_bitmap_len > numBlocks){
    cerr << "Error: Bitmap exceeds disk size " << endl;
    exit(1);
  }

  if(super->inode_bitmap_addr >= numBlocks || super->inode_bitmap_addr < 0 ){
    cerr << "Error: addr is out of bounds (less than 0 or above number of blocks) " << endl;
    exit(1);
  }

  //iterate through each block of the bitmap and basically do the same thing we did readSuperBlock
  for(int i = 0; i < super->inode_bitmap_len; i++){
    int current = super->inode_bitmap_addr + i;
    disk->readBlock(current, buffer);

    memcpy(inodeBitmap + (i * UFS_BLOCK_SIZE),buffer,UFS_BLOCK_SIZE);
  }
}

void LocalFileSystem::writeInodeBitmap(super_t *super, unsigned char *inodeBitmap) {

}

void LocalFileSystem::readDataBitmap(super_t *super, unsigned char *dataBitmap) {
  //grab the number of blocks using the function from disk class
  int numBlocks = disk->numberOfBlocks();
  unsigned char buffer[UFS_BLOCK_SIZE];

  //do error checking
  if(super->data_bitmap_len <= 0){

    cerr << "Error: bitmap len can not be negative" << endl;
    exit(1);
  }

  if(super->data_bitmap_addr + super->data_bitmap_len > numBlocks){
    cerr << "Error: Bitmap exceeds disk size " << endl;
    exit(1);
  }

  if(super->data_bitmap_addr >= numBlocks || super->data_bitmap_addr < 0 ){
    cerr << "Error: addr is out of bounds (less than 0 or above number of blocks) " << endl;
    exit(1);
  }

  //iterate through each block of the bitmap and basically do the same thing we did readSuperBlock
  for(int i = 0; i < super->data_bitmap_len; i++){
    int current = super->data_bitmap_addr + i;
    disk->readBlock(current, buffer);

    memcpy(dataBitmap + (i * UFS_BLOCK_SIZE),buffer,UFS_BLOCK_SIZE);
  }
}

void LocalFileSystem::writeDataBitmap(super_t *super, unsigned char *dataBitmap) {

}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {

  //get number of blocks we have and also create a buffer
  int numBlocks = disk->numberOfBlocks();
  unsigned char buffer[UFS_BLOCK_SIZE];

  //do error checking
  if(super->inode_region_len <= 0){
    cerr << "Error: Inode region length is less than 0 invalid"<< endl;
    exit(1);
  }
  if(super->inode_region_addr < 0 || super->inode_region_addr > numBlocks){
    cerr << "Error: Inode region address is out of bounds under 0 or above number of blocks avaliable" << endl;
    exit(1);
  }
  if (super->inode_region_addr + super->inode_region_len > numBlocks) {
    cerr << "Error: Inode region exceeds disk size" << endl;
    exit(1);
  }

  //calculate the amount of inodes in a block
  int totNodesRead = 0;
  int inodePerBlock  = UFS_BLOCK_SIZE/sizeof(inode_t);

  for(int i = 0; i < super->inode_region_len && totNodesRead < super->num_inodes; i++){
    int currentBlock = super->inode_region_addr + i;
    disk->readBlock(currentBlock,buffer);

    int  byteOffSet = 0;
    int minBlock = min(inodePerBlock, super->num_inodes - totNodesRead);

    memcpy(&inodes[totNodesRead], buffer + byteOffSet,minBlock * sizeof(inode_t));
    totNodesRead += minBlock;
  }

}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {

}

int LocalFileSystem::lookup(int parentInodeNumber, string name) {
  
  //use the function  we already have
  super_t super;
  readSuperBlock(&super);

  if(parentInodeNumber >= super.num_inodes || parentInodeNumber < 0){
    return -EINVALIDINODE;
  }

  int mapSize = super.inode_bitmap_len * UFS_BLOCK_SIZE;
  unsigned char *bitMap = new unsigned char[mapSize];
  readInodeBitmap(&super,bitMap);

  int index = parentInodeNumber/8;
  int bitIndex = parentInodeNumber % 8;
  bool isAlocated = bitMap[index] & (1 << bitIndex);

  delete[] bitMap;

  if(isAlocated){
    return -ENOTALLOCATED;
  }

  //use the function readInodeRegion do read it
  inode_t *inode = new inode_t [super.num_inodes];
  readInodeRegion(&super,inode);

  //grab parent using its number and the inode variable we made
  inode_t parent = inode[parentInodeNumber];


  //do error checks

  //do typical out of bounds check
  if(parentInodeNumber >= super.num_inodes || parentInodeNumber < 0){
    delete[] inode;
    return -EINVALIDINODE;
  }

  //check if the parent we grabbed earlier is valid or not, basically do a if statment
  if(parent.type != UFS_DIRECTORY){

    delete[] inode;
    return -EINVALIDINODE;
  }


  /*
  basically grab size bytes of parent, amount of entries using parent,create
  new buffer, and read bytes using parent node and the new size and buffer we got
  */
  int size = parent.size;
  int entries = size/sizeof(dir_ent_t);
  char *buff = new char[size];
  int bytesRead = read(parentInodeNumber,buff,size);

  //error check and clean up, depending on if we get a -1 from a failed read
  if(bytesRead < 0){


    delete[] inode;
    delete[] buff;
    return bytesRead;
  }
  
  //create variables needed to check the entries 
  string nameOfEntry = "";
  dir_ent_t *allEntries  = (dir_ent_t*)buff;

  //loop through each entries name and check if its the one we want
  for(int i = 0; i < entries; i++){
    nameOfEntry = allEntries[i].name;
    if(nameOfEntry == name){
      int foundName = allEntries[i].inum;
      delete[] buff;
      delete[] inode;
      return foundName;
    }
  }

  //if it fails and we don't find it just delete stuff and return
  delete[] inode;
  delete[] buff;
  return -ENOTFOUND;
}

int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
  //read using function
  super_t super;
  readSuperBlock(&super);

  //error check for inodeNumber just in case
  if(inodeNumber < 0 || inodeNumber >= super.num_inodes){
    return -EINVALIDINODE;
  }

  int mapSize = super.inode_bitmap_len * UFS_BLOCK_SIZE;
  unsigned char *bitMap = new unsigned char[mapSize];
  readInodeBitmap(&super,bitMap);

  int index = inodeNumber/8;
  int bitIndex = inodeNumber % 8;
  bool isAlocated = bitMap[index] & (1 << bitIndex);

  delete[] bitMap;

  if(isAlocated){
    return -ENOTALLOCATED;
  }

  
  inode_t *inodes = new inode_t[super.num_inodes];
  readInodeRegion(&super,inodes);

  *inode = inodes[inodeNumber];

  delete[] inodes;
  return 0;
}

int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {
  
  //like before we read the superBlock
  super_t super;
  readSuperBlock(&super);

  //error checks just in case we're tested on it
  if(inodeNumber >= super.num_inodes || inodeNumber < 0){
    return -EINVALIDINODE;
  }

  if(size < 0 ){
    return -EINVALIDSIZE;
  }

  int mapSize = super.inode_bitmap_len * UFS_BLOCK_SIZE;
  unsigned char *bitMap = new unsigned char[mapSize];
  readInodeBitmap(&super,bitMap);

  int index = inodeNumber/8;
  int bitIndex = inodeNumber % 8;
  bool isAlocated = bitMap[index] & (1 << bitIndex);

  delete[] bitMap;

  if(isAlocated){
    return -ENOTALLOCATED;
  }



  inode_t *inodes = new inode_t[super.num_inodes];
  readInodeRegion(&super,inodes);

  inode_t inode = inodes[inodeNumber];

  unsigned char *buff = (unsigned char *)buffer;
  int bytesRead = 0;

  int bytesNeedRead = min(size,inode.size);
  for(int i = 0; i < DIRECT_PTRS && bytesRead < bytesNeedRead; i++){
    if(inode.direct[i] == 0){
      break;
    }

    unsigned char block[UFS_BLOCK_SIZE];

    int numBlock = inode.direct[i] + super.data_region_addr;

    disk->readBlock(numBlock,block);
    int byteLeftOver = bytesNeedRead - bytesRead;
    int byteBlocks = min(UFS_BLOCK_SIZE,byteLeftOver);

    memcpy(buff + bytesRead,block,byteBlocks);

    bytesRead += byteBlocks;
  }

  //clean up
  delete[] inodes;

  return bytesRead;
}

int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
  return 0;
}

int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {
  return 0;
}

int LocalFileSystem::unlink(int parentInodeNumber, string name) {
  return 0;
}

