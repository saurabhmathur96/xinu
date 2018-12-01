#include <xinu.h>
#include <kernel.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef FS
#include <fs.h>

static struct fsystem fsd;
int dev0_numblocks;
int dev0_blocksize;
char *dev0_blocks;

extern int dev0;

char block_cache[512];

#define SB_BLK 0
#define BM_BLK 1
#define RT_BLK 2

#define NUM_FD 16
struct filetable oft[NUM_FD];
int next_open_fd = 0;

#define INODES_PER_BLOCK (fsd.blocksz / sizeof(struct inode))
#define NUM_INODE_BLOCKS (((fsd.ninodes % INODES_PER_BLOCK) == 0) ? fsd.ninodes / INODES_PER_BLOCK : (fsd.ninodes / INODES_PER_BLOCK) + 1)
#define FIRST_INODE_BLOCK 2

int fs_fileblock_to_diskblock(int dev, int fd, int fileblock);

/* YOUR CODE GOES HERE */

int fs_fileblock_to_diskblock(int dev, int fd, int fileblock)
{
  int diskblock;

  if (fileblock >= INODEBLOCKS - 2)
  {
    printf("No indirect block support\n");
    return SYSERR;
  }

  diskblock = oft[fd].in.blocks[fileblock]; //get the logical block address

  return diskblock;
}

/* read in an inode and fill in the pointer */
int fs_get_inode_by_num(int dev, int inode_number, struct inode *in)
{
  int bl, inn;
  int inode_off;

  if (dev != 0)
  {
    printf("Unsupported device\n");
    return SYSERR;
  }
  if (inode_number > fsd.ninodes)
  {
    printf("fs_get_inode_by_num: inode %d out of range\n", inode_number);
    return SYSERR;
  }

  bl = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  inode_off = inn * sizeof(struct inode);

  /*
  printf("in_no: %d = %d/%d\n", inode_number, bl, inn);
  printf("inn*sizeof(struct inode): %d\n", inode_off);
  */

  bs_bread(dev0, bl, 0, &block_cache[0], fsd.blocksz);
  memcpy(in, &block_cache[inode_off], sizeof(struct inode));

  return OK;
}

int fs_put_inode_by_num(int dev, int inode_number, struct inode *in)
{
  int bl, inn;

  if (dev != 0)
  {
    printf("Unsupported device\n");
    return SYSERR;
  }
  if (inode_number > fsd.ninodes)
  {
    printf("fs_put_inode_by_num: inode %d out of range\n", inode_number);
    return SYSERR;
  }

  bl = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  /*
  printf("in_no: %d = %d/%d\n", inode_number, bl, inn);
  */

  bs_bread(dev0, bl, 0, block_cache, fsd.blocksz);
  memcpy(&block_cache[(inn * sizeof(struct inode))], in, sizeof(struct inode));
  bs_bwrite(dev0, bl, 0, block_cache, fsd.blocksz);

  return OK;
}

int fs_mkfs(int dev, int num_inodes)
{
  int i;

  if (dev == 0)
  {
    fsd.nblocks = dev0_numblocks;
    fsd.blocksz = dev0_blocksize;
  }
  else
  {
    printf("Unsupported device\n");
    return SYSERR;
  }

  if (num_inodes < 1)
  {
    fsd.ninodes = DEFAULT_NUM_INODES;
  }
  else
  {
    fsd.ninodes = num_inodes;
  }

  i = fsd.nblocks;
  while ((i % 8) != 0)
  {
    i++;
  }
  fsd.freemaskbytes = i / 8;

  if ((fsd.freemask = getmem(fsd.freemaskbytes)) == (void *)SYSERR)
  {
    printf("fs_mkfs memget failed.\n");
    return SYSERR;
  }

  /* zero the free mask */
  for (i = 0; i < fsd.freemaskbytes; i++)
  {
    fsd.freemask[i] = '\0';
  }

  fsd.inodes_used = 0;

  /* write the fsystem block to SB_BLK, mark block used */
  fs_setmaskbit(SB_BLK);
  bs_bwrite(dev0, SB_BLK, 0, &fsd, sizeof(struct fsystem));

  /* write the free block bitmask in BM_BLK, mark block used */
  fs_setmaskbit(BM_BLK);
  bs_bwrite(dev0, BM_BLK, 0, fsd.freemask, fsd.freemaskbytes);

  return 1;
}

void fs_print_fsd(void)
{

  printf("fsd.ninodes: %d\n", fsd.ninodes);
  printf("sizeof(struct inode): %d\n", sizeof(struct inode));
  printf("INODES_PER_BLOCK: %d\n", INODES_PER_BLOCK);
  printf("NUM_INODE_BLOCKS: %d\n", NUM_INODE_BLOCKS);
}

/* specify the block number to be set in the mask */
int fs_setmaskbit(int b)
{
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  fsd.freemask[mbyte] |= (0x80 >> mbit);
  return OK;
}

/* specify the block number to be read in the mask */
int fs_getmaskbit(int b)
{
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  return (((fsd.freemask[mbyte] << mbit) & 0x80) >> 7);
  return OK;
}

/* specify the block number to be unset in the mask */
int fs_clearmaskbit(int b)
{
  int mbyte, mbit, invb;
  mbyte = b / 8;
  mbit = b % 8;

  invb = ~(0x80 >> mbit);
  invb &= 0xFF;

  fsd.freemask[mbyte] &= invb;
  return OK;
}

/* This is maybe a little overcomplicated since the lowest-numbered
   block is indicated in the high-order bit.  Shift the byte by j
   positions to make the match in bit7 (the 8th bit) and then shift
   that value 7 times to the low-order bit to print.  Yes, it could be
   the other way...  */
void fs_printfreemask(void)
{
  int i, j;

  for (i = 0; i < fsd.freemaskbytes; i++)
  {
    for (j = 0; j < 8; j++)
    {
      printf("%d", ((fsd.freemask[i] << j) & 0x80) >> 7);
    }
    if ((i % 8) == 7)
    {
      printf("\n");
    }
  }
  printf("\n");
}

int fs_open(char *filename, int flags)
{
  intmask	mask = disable();
  if (flags > 2)
  {
    restore(mask);
    return SYSERR;
  }
  int n = strlen(filename);
  if (n > FILENAMELEN)
  {
    restore(mask);
    return SYSERR;
  }
  int i, j;
  // for each file in root directory
  for (i = 0; i < fsd.root_dir.numentries; i++)
  {
    if (0 == strncmp(fsd.root_dir.entry[i].name, filename, n))
    {
      // for each entry in file table
      for (j = 0; j < NUM_FD; j++)
      {
        if (oft[j].in.id == fsd.root_dir.entry[i].inode_num)
        {
          if (oft[j].state == FSTATE_OPEN)
          {
            // File is already open
            restore(mask);
            return SYSERR;
          }
        }
      }
      // File is not open
      // Create entry in file table
      for (j = 0; j < NUM_FD; j++)
      {
        if (oft[j].state == FSTATE_CLOSED)
        {
          // unused row found
          struct inode in;
          fs_get_inode_by_num(dev0, fsd.root_dir.entry[i].inode_num, &in);
          oft[j] = (struct filetable) { .state = FSTATE_OPEN, .fileptr = 0, .de = &fsd.root_dir.entry[i],
                                      .in = in };
          restore(mask);
          return j;

        }
      }
    }
  }
  // File data not found
  restore(mask);
  return SYSERR;
}

int fs_close(int fd)
{
  intmask	mask = disable();
  if (fd < 0 || fd >= NUM_FD)
  {
    restore(mask);
    return SYSERR;
  }
  oft[fd].state = FSTATE_CLOSED;
  oft[fd].fileptr = 0;

  restore(mask);
  return OK;
}

int fs_create(char *filename, int mode)
{
  intmask	mask = disable();
  if (O_CREAT != mode)
  {
    restore(mask);
    return SYSERR;
  }
  int n = strlen(filename);
  if (n > FILENAMELEN)
  {
    restore(mask);
    return SYSERR;
  }

  if (fsd.inodes_used == fsd.ninodes)
  {
    // No more inodes available
    restore(mask);
    return SYSERR;
  }

  int i;
  for (i = 0; i < fsd.root_dir.numentries; i++)
  {
    if (0 == strncmp(filename, fsd.root_dir.entry[i].name, n))
    {
      // File already exists
      restore(mask);
      return SYSERR;
    }
  }

  struct inode in;
  for (i = 0; i < fsd.ninodes; i++)
  {
    fs_get_inode_by_num(dev0, i, &in);
    if (in.type == INODE_TYPE_EMPTY)
    {
      in = (struct inode){.id = i, .type = INODE_TYPE_FILE, .nlink = 0, .device = dev0, .size = 0};
      fsd.root_dir.entry[fsd.root_dir.numentries].inode_num = i;
      strncpy(fsd.root_dir.entry[fsd.root_dir.numentries].name, filename, n);
      fsd.root_dir.numentries += 1;
      fsd.inodes_used += 1;
      
      restore(mask);
      return fs_open(filename, O_RDWR);
    }
  }
  restore(mask);
  return SYSERR;
}

int fs_seek(int fd, int offset)
{
  intmask	mask = disable();
  if (fd >= NUM_FD || fd < 0)
  {
    // invalid fd
    restore(mask);
    return SYSERR;
  }
  if (oft[fd].fileptr < offset)
  {
    // invalid offset
    restore(mask);
    return SYSERR;
  }
  if (oft[fd].state != FSTATE_OPEN)
  {
    // file not open
    restore(mask);
    return SYSERR;
  }
  oft[fd].fileptr += offset;
  restore(mask);
  return OK;
}

int fs_read(int fd, void *buf, int nbytes)
{
  intmask	mask = disable();
  if (fd >= NUM_FD || fd < 0)
  {
    // invalid fd
    restore(mask);
    return SYSERR;
  }
  if (oft[fd].state != FSTATE_OPEN)
  {
    // file not open
    restore(mask);
    return SYSERR;
  }
  if (nbytes < 0)
  {
    restore(mask);
    return SYSERR;
  }
  int block_size = MDEV_BLOCK_SIZE;
  printf("block size=%d, nbytes=%d\n", block_size, nbytes);
  int current_block = (oft[fd].fileptr / block_size) + 1;
  int total_bytes_read = 0;
  int offset = oft[fd].fileptr % block_size;
  while(nbytes > 0) {
    int bytes_to_read = (nbytes > block_size) ? block_size : nbytes;
    bs_bread(dev0, oft[fd].in.blocks[current_block], offset, buf, bytes_to_read);
    printf("read %d bytes from %d\n", bytes_to_read, oft[fd].in.blocks[current_block]);
    buf += bytes_to_read;
    nbytes -= bytes_to_read;
    total_bytes_read += bytes_to_read;
    oft[fd].fileptr += bytes_to_read;
    current_block += 1;
    offset = 0;
    
  }
  restore(mask);
  return total_bytes_read;
}

int allocate_free_block()
{
  int i;
  for (i=0; i<MDEV_NUM_BLOCKS; i++)
  {
    if (0 == fs_getmaskbit(i))
    {
      fs_setmaskbit(i);
      return i;
    }
  }
  return -1;
}

int fs_write(int fd, void *buf, int nbytes)
{
  intmask	mask = disable();
  if (fd >= NUM_FD || fd < 0)
  {
    // invalid fd
    restore(mask);
    return SYSERR;
  }
  if (nbytes < 0)
  {
    restore(mask);
    return SYSERR;
  } 
  if (oft[fd].state != FSTATE_OPEN)
  {
    // file not open
    restore(mask);
    return SYSERR;
  }
  int block_size = MDEV_BLOCK_SIZE;
  printf("block size=%d, nbytes=%d\n", block_size, nbytes);
  int current_block = (oft[fd].in.size / block_size) + 1;
  int offset = oft[fd].in.size % block_size; // offset for last block
  int total_bytes_written = 0;
  while (nbytes > 0)
  {
    if (offset == 0)
    {
      // allocate new block
      int block_id = allocate_free_block();
      if (block_id == -1)
      {
        // ran out of free blocks
        restore(mask);
        return SYSERR;
      }
      oft[fd].in.blocks[current_block] = block_id;
    }
    int remaining_space = block_size - offset;
    int bytes_to_write = (nbytes < remaining_space) ? nbytes : remaining_space;
    
    bs_bwrite(dev0, oft[fd].in.blocks[current_block], offset, buf, bytes_to_write);
    printf("wrote %d bytes to %d\n", bytes_to_write, oft[fd].in.blocks[current_block]);
    nbytes -= bytes_to_write;
    total_bytes_written += bytes_to_write;
    oft[fd].fileptr += bytes_to_write;
    remaining_space = MDEV_BLOCK_SIZE;
    offset = 0;
    current_block += 1;
  }
  oft[fd].in.size += total_bytes_written;
  
  restore(mask);
  return total_bytes_written;
}

#endif /* FS */
