#ifndef _TYPES_H_
#define _TYPES_H_

/******************************************************************************
* SECTION: Type def
*******************************************************************************/
typedef int          boolean;
typedef uint16_t     flag16;

typedef enum newfs_file_type {
    NEWFS_REG_FILE,
    NEWFS_DIR
} NEWFS_FILE_TYPE;
/******************************************************************************
* SECTION: Macro
*******************************************************************************/
#define TRUE                    1
#define FALSE                   0
#define UINT32_BITS             32
#define UINT8_BITS              8

#define NEWFS_MAGIC_NUM             4444544
#define NEWFS_SUPER_OFS             0
#define NEWFS_ROOT_INO              0

#define NEWFS_ERROR_NONE            0
#define NEWFS_ERROR_ACCESS          EACCES
#define NEWFS_ERROR_SEEK            ESPIPE     
#define NEWFS_ERROR_ISDIR           EISDIR
#define NEWFS_ERROR_NOSPACE         ENOSPC
#define NEWFS_ERROR_EXISTS          EEXIST
#define NEWFS_ERROR_NOTFOUND        ENOENT
#define NEWFS_ERROR_UNSUPPORTED     ENXIO
#define NEWFS_ERROR_IO              EIO     /* Error Input/Output */
#define NEWFS_ERROR_INVAL           EINVAL  /* Invalid Args */

#define MAX_NAME_LEN                128     
#define NEWFS_MAX_FILE_NAME         128
#define NEWFS_INODE_PER_FILE        1
#define NEWFS_DATA_PER_FILE         6          // 一个文件不能超过 6个（数据块）
#define NEWFS_DEFAULT_PERM          0777

/******************************************************************************
* SECTION: Macro Function
*******************************************************************************/
#define NEWFS_IO_SZ()                   (newfs_super.sz_io)
#define NEWFS_BLK_SZ()                  (newfs_super.sz_blk)
#define NEWFS_DISK_SZ()                 (newfs_super.sz_disk)
#define NEWFS_DRIVER()                  (newfs_super.driver_fd)
#define NEWFS_MAX_INO()                 (newfs_super.max_ino)
#define NEWFS_MAX_DATA()                (newfs_super.max_data)

#define NEWFS_ROUND_DOWN(value, round)  (value % round == 0 ? value : (value / round) * round)
#define NEWFS_ROUND_UP(value, round)    (value % round == 0 ? value : (value / round + 1) * round)

#define NEWFS_BLKS_SZ(blks)             (blks * NEWFS_BLK_SZ())


#define NEWFS_IS_DIR(pinode)            (pinode->dentry->ftype == NEWFS_DIR)
#define NEWFS_IS_REG(pinode)            (pinode->dentry->ftype == NEWFS_REG_FILE)

#define NEWFS_ASSIGN_FNAME(pnewfs_dentry, _fname)\
                                        memcpy(pnewfs_dentry->fname, _fname, strlen(_fname))
                            
#define NEWFS_INO_SZ()                  (sizeof(struct newfs_inode_d))
#define NEWFS_INO_OFS(ino)              (newfs_super.inode_offset + ino * NEWFS_INO_SZ())
#define NEWFS_DATA_OFS(blk)             (newfs_super.data_offset +  NEWFS_BLKS_SZ((blk)))
/******************************************************************************            
// #define NEWFS_INO_OFS(ino)                (newfs_super.inode_offset + ino * NEWFS_BLKS_SZ((\
//                                          NEWFS_INODE_PER_FILE)))
// #define NEWFS_DATA_OFS(ino)               (newfs_super.data_offset + ino * NEWFS_BLKS_SZ((\
//                                         NEWFS_DATA_PER_FILE)))
*******************************************************************************/





struct custom_options {
	 char*        device;
};

struct newfs_super {
    uint32_t magic;
    int      fd;
    /* TODO: Define yourself */
    
    int                     driver_fd;
    int                     sz_io;
    int                     sz_blk;
    int                     sz_disk;
    int                     sz_usage;
    
    int                     max_ino;                    // 最多支持的文件数
    int                     max_data;                   // 最多数据块
    uint8_t*                map_inode;
    int                     map_inode_blks;
    int                     map_inode_offset;           // inode位图偏移
    uint8_t*                map_data;
    int                     map_data_blks;              // data位图占用的块数
    int                     map_data_offset;

    int                     inode_offset;
    
    int                     data_offset;

    boolean                 is_mounted;

    struct newfs_dentry*    root_dentry;
};



struct newfs_inode {
    // uint32_t ino1;
    /* TODO: Define yourself */
    
    int                         ino;                           /* 在inode位图中的下标 */
    int                         size;                          /* 文件已占用空间 */
    int                         dir_cnt;
    struct newfs_dentry*        dentry;                        /* 指向该inode的dentry */
    struct newfs_dentry*        dentrys;                       /* 所有目录项 */
    uint8_t*                    data;           
    int                         data_blk[6];               // 数据块指针
};  

struct newfs_dentry {
    char                        name[MAX_NAME_LEN];
    // uint32_t ino;
    /* TODO: Define yourself */
    char                        fname[NEWFS_MAX_FILE_NAME];
    struct newfs_dentry*        parent;                        /* 父亲Inode的dentry */
    struct newfs_dentry*        brother;                       /* 兄弟 */
    int                         ino;
    struct newfs_inode*         inode;                         /* 指向inode */
    NEWFS_FILE_TYPE             ftype;
};


static inline struct newfs_dentry* new_dentry(char * fname, NEWFS_FILE_TYPE ftype) {
    struct newfs_dentry * dentry = (struct newfs_dentry *)malloc(sizeof(struct newfs_dentry));
    memset(dentry, 0, sizeof(struct newfs_dentry));
    NEWFS_ASSIGN_FNAME(dentry, fname);
    dentry->ftype   = ftype;
    dentry->ino     = -1;
    dentry->inode   = NULL;
    dentry->parent  = NULL;
    dentry->brother = NULL;                                            
}

/******************************************************************************
* SECTION: FS Specific Structure - Disk structure
*******************************************************************************/
struct newfs_super_d
{
    uint32_t            magic_num;
    int                 sz_usage;
    
    int                 max_ino;
    int                 max_data;                       // 最多数据块
    int                 map_inode_blks;                 // inode位图占用的块数
    int                 map_inode_offset;               // inode位图在磁盘上的偏移
    int                 map_data_blks;                  // data位图占用的块数
    int                 map_data_offset;                // data位图在磁盘上的偏移
    int                 inode_offset;                   // inode在磁盘上的偏移
    int                 data_offset;
};
// 32B
struct newfs_inode_d
{
    int                 ino;                           /* 在inode位图中的下标 */
    int                 size;                          /* 文件已占用空间 */
    int                 dir_cnt;
    NEWFS_FILE_TYPE     ftype;   
    int                 link;               // 链接数
    uint16_t            data_blk[NEWFS_DATA_PER_FILE];
};  

struct newfs_dentry_d
{
    char                fname[NEWFS_MAX_FILE_NAME];
    NEWFS_FILE_TYPE     ftype;
    int                 ino;                           /* 指向的ino号 */
};  


#endif /* _TYPES_H_ */