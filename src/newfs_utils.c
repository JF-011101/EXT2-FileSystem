#include "../include/newfs.h"
/**
 * @brief 获取文件名
 * 
 * @param path 
 * @return char* 
 */
char* newfs_get_fname(const char* path) {
    char ch = '/';
    char *q = strrchr(path, ch) + 1;
    return q;
}

/**
 * @brief 计算路径的层级
 * exm: /av/c/d/f
 * -> lvl = 4
 * @param path 
 * @return int 
 */
int newfs_calc_lvl(const char * path) {
    // char* path_cpy = (char *)malloc(strlen(path));
    // strcpy(path_cpy, path);
    char* str = (char*)path;
    int   lvl = 0;
    if (strcmp(path, "/") == 0) {
        return lvl;
    }
    while (*str != '\0') {
        if (*str == '/') {
            lvl++;
        }
        str++;
    }
    return lvl;
}

/**
 * @brief 驱动读
 * 
 * @param offset 
 * @param out_content 
 * @param size 
 * @return int 
 */
int newfs_driver_read(int offset, uint8_t *out_content, int size) {
    int      offset_aligned = NEWFS_ROUND_DOWN(offset, NEWFS_IO_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = NEWFS_ROUND_UP((size + bias), NEWFS_IO_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    // lseek(NEWFS_DRIVER(), offset_aligned, SEEK_SET);
    ddriver_seek(NEWFS_DRIVER(), offset_aligned, SEEK_SET);
    while (size_aligned != 0)
    {
        // read(NEWFS_DRIVER(), cur, NEWFS_IO_SZ());
        ddriver_read(NEWFS_DRIVER(), (char*)cur, NEWFS_IO_SZ());
        cur          += NEWFS_IO_SZ();
        size_aligned -= NEWFS_IO_SZ();   
    }
    memcpy(out_content, temp_content + bias, size);
    free(temp_content);
    return NEWFS_ERROR_NONE;
}


/**
 * @brief 驱动写
 * 
 * @param offset 
 * @param in_content 
 * @param size 
 * @return int 
 */
int newfs_driver_write(int offset, uint8_t *in_content, int size) {
    int      offset_aligned = NEWFS_ROUND_DOWN(offset, NEWFS_IO_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = NEWFS_ROUND_UP((size + bias), NEWFS_IO_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    newfs_driver_read(offset_aligned, temp_content, size_aligned);
    memcpy(temp_content + bias, in_content, size);
    
    // lseek(NEWFS_DRIVER(), offset_aligned, SEEK_SET);
    ddriver_seek(NEWFS_DRIVER(), offset_aligned, SEEK_SET);
    while (size_aligned != 0)
    {
        // write(NEWFS_DRIVER(), cur, NEWFS_IO_SZ());
        ddriver_write(NEWFS_DRIVER(), (char*)cur, NEWFS_IO_SZ());
        cur          += NEWFS_IO_SZ();
        size_aligned -= NEWFS_IO_SZ();   
    }

    free(temp_content);
    return NEWFS_ERROR_NONE;
}

/**
 * @brief 为一个inode分配dentry，采用头插法
 * 
 * @param inode 
 * @param dentry 
 * @return int 
 */
int newfs_alloc_dentry(struct newfs_inode* inode, struct newfs_dentry* dentry) {
    if (inode->dentrys == NULL) {
        inode->dentrys = dentry;
    }
    else {
        dentry->brother = inode->dentrys;
        inode->dentrys = dentry;
    }
    inode->dir_cnt++;
    return inode->dir_cnt;
}
/**
 * @brief 分配一个数据块
 * 
 * @return 返回块号
 */
int
newfs_alloc_data_blk(){
    
    int byte_cursor = 0; 
    int bit_cursor  = 0; 
    int data_blk_cursor  = 0;
    boolean is_find_free_blk = FALSE;
    for (byte_cursor = 0; byte_cursor < NEWFS_MAX_DATA()/UINT8_BITS;          byte_cursor++)
    {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if((newfs_super.map_data[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                                                      /* 当前data_blk_cursor位置空闲 */
                newfs_super.map_data[byte_cursor] |= (0x1 << bit_cursor);
                is_find_free_blk = TRUE;           
                break;
            }
            data_blk_cursor++;
        }
        if (is_find_free_blk) {
            return data_blk_cursor;
        }
    }
    return -NEWFS_ERROR_NOSPACE;
}
/**
 * @brief 为dentry分配一个inode，占用位图
 * 
 * @param dentry 该dentry指向分配的inode
 * @return newfs_inode
 */
struct newfs_inode* newfs_alloc_inode(struct newfs_dentry * dentry) {
    struct newfs_inode* inode;
    int byte_cursor = 0; 
    int bit_cursor  = 0; 
    int ino_cursor  = 0;
    boolean is_find_free_entry = FALSE;

    // for (byte_cursor = 0; byte_cursor < NEWFS_BLKS_SZ(newfs_super.map_inode_blks);          byte_cursor++)
    for (byte_cursor = 0; byte_cursor < NEWFS_MAX_INO()/UINT8_BITS;          byte_cursor++)
    {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if((newfs_super.map_inode[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                                                      /* 当前ino_cursor位置空闲 */
                newfs_super.map_inode[byte_cursor] |= (0x1 << bit_cursor);
                is_find_free_entry = TRUE;           
                break;
            }
            ino_cursor++;
        }
        if (is_find_free_entry) {
            break;
        }
    }

    // if (!is_find_free_entry || ino_cursor == newfs_super.max_ino)
    //     return -NEWFS_ERROR_NOSPACE;

    inode = (struct newfs_inode*)malloc(sizeof(struct newfs_inode));
    inode->ino  = ino_cursor; 
    inode->size = 0;
                                                      /* dentry指向inode */
    dentry->inode = inode;
    dentry->ino   = inode->ino;
                                                      /* inode指回dentry */
    inode->dentry = dentry;
    
    inode->dir_cnt = 0;
    inode->dentrys = NULL;
    inode->data_blk[0] = newfs_alloc_data_blk();
    
    if (NEWFS_IS_REG(inode)) {
        inode->data = (uint8_t *)malloc(NEWFS_BLKS_SZ(1));
    }

    return inode;
}


/**
 * @brief 将内存inode及其下方结构全部刷回磁盘
 * 
 * @param inode 
 * @return int 
 */
int newfs_sync_inode(struct newfs_inode * inode) {
    struct newfs_inode_d  inode_d;
    struct newfs_dentry*  dentry_cursor;
    struct newfs_dentry_d dentry_d;
    int ino             = inode->ino;
    int offset;
    inode_d.ino         = ino;
    inode_d.size        = inode->size;
    inode_d.ftype       = inode->dentry->ftype;
    inode_d.dir_cnt     = inode->dir_cnt;
    
    for(int i = 0;i<NEWFS_DATA_PER_FILE;i++)
        inode_d.data_blk[i] = (uint16_t)inode->data_blk[i];
    // int offset;
    // int a = NEWFS_INO_OFS(ino);
    // int c = sizeof(struct newfs_inode_d);
    // uint8_t* b = (uint8_t *)&inode_d;
    // printf("a,b,c:%d %d %d",a,b,c);
    // int flag = newfs_driver_write(NEWFS_INO_OFS(ino), (uint8_t *)&inode_d, sizeof(struct newfs_inode_d)) ;
    // int flag = newfs_driver_write(a, b,c) ;
    if (newfs_driver_write(NEWFS_INO_OFS(ino), (uint8_t *)&inode_d, sizeof(struct newfs_inode_d)) != (NEWFS_ERROR_NONE)){
        NEWFS_DBG("[%s] io error\n", __func__);
        return -NEWFS_ERROR_IO;
    }
                                                      /* Cycle 1: 写 INODE */
                                                      /* Cycle 2: 写 数据 */
    if (NEWFS_IS_DIR(inode)) {  
        // 目录文件                        
        dentry_cursor = inode->dentrys;
        // 不再使用固定数据块
        // 改为使用inode指示数据块
        offset        = NEWFS_DATA_OFS(inode->data_blk[0]);
        // offset        = inode(ino);
        while (dentry_cursor != NULL)
        {
            memcpy(dentry_d.fname, dentry_cursor->fname, NEWFS_MAX_FILE_NAME);
            dentry_d.ftype = dentry_cursor->ftype;
            dentry_d.ino = dentry_cursor->ino;
            if (newfs_driver_write(offset, (uint8_t *)&dentry_d, 
                                 sizeof(struct newfs_dentry_d)) != NEWFS_ERROR_NONE) {
                NEWFS_DBG("[%s] io error\n", __func__);
                return -NEWFS_ERROR_IO;                     
            }
            // 递归写回各个子目录项的inode
            if (dentry_cursor->inode != NULL) {
                newfs_sync_inode(dentry_cursor->inode);
            }

            dentry_cursor = dentry_cursor->brother;
            offset += sizeof(struct newfs_dentry_d);
        }
    }
    else if (NEWFS_IS_REG(inode)) {
        // 数据文件             
        if (newfs_driver_write(offset, inode->data, 
                             NEWFS_BLKS_SZ(1)) != NEWFS_ERROR_NONE) {
            NEWFS_DBG("[%s] io error\n", __func__);
            return -NEWFS_ERROR_IO;
        }
    }
    return NEWFS_ERROR_NONE;
}


/**
 * @brief 
 * 
 * @param dentry dentry指向ino，读取该inode
 * @param ino inode唯一编号
 * @return struct newfs_inode* 
 */
struct newfs_inode* newfs_read_inode(struct newfs_dentry * dentry, int ino) {
    struct newfs_inode* inode = (struct newfs_inode*)malloc(sizeof(struct newfs_inode));
    struct newfs_inode_d inode_d;
    struct newfs_dentry* sub_dentry;
    struct newfs_dentry_d dentry_d;
    int    dir_cnt = 0, i;
    if (newfs_driver_read(NEWFS_INO_OFS(ino), (uint8_t *)&inode_d, sizeof(struct newfs_inode_d)) != NEWFS_ERROR_NONE) {
        NEWFS_DBG("[%s] io error\n", __func__);
        return NULL;                    
    }
    inode->dir_cnt = 0;
    inode->ino = inode_d.ino;
    inode->size = inode_d.size;
    inode->dentry = dentry;
    inode->dentrys = NULL;
    for(int i = 0;i<NEWFS_DATA_PER_FILE;i++)
        inode->data_blk[i] = (int)(inode_d.data_blk[i]);
    if (NEWFS_IS_DIR(inode)) {
        dir_cnt = inode_d.dir_cnt;
        for (i = 0; i < dir_cnt; i++){
            if (newfs_driver_read(NEWFS_DATA_OFS(inode->data_blk[0]) + i * sizeof(struct newfs_dentry_d), 
                                (uint8_t *)&dentry_d, 
                                sizeof(struct newfs_dentry_d)) != NEWFS_ERROR_NONE) {
                NEWFS_DBG("[%s] io error\n", __func__);
                return NULL;                    
            }
            sub_dentry = new_dentry(dentry_d.fname, dentry_d.ftype);
            sub_dentry->parent = inode->dentry;
            sub_dentry->ino    = dentry_d.ino; 
            newfs_alloc_dentry(inode, sub_dentry);
        }
    }
    else if (NEWFS_IS_REG(inode)) {
        inode->data = (uint8_t *)malloc(NEWFS_BLKS_SZ(1));
        if (newfs_driver_read(NEWFS_DATA_OFS(inode->data_blk[0]), (uint8_t *)inode->data, NEWFS_BLKS_SZ(1)) != NEWFS_ERROR_NONE) {
            NEWFS_DBG("[%s] io error\n", __func__);
            return NULL;                    
        }
    }
    return inode;
}

/**
 * @brief 找到inode的第dir条目录项
 * 
 * @param inode 
 * @param dir [0...]
 * @return struct newfs_dentry* 
 */
struct newfs_dentry* newfs_get_dentry(struct newfs_inode * inode, int dir) {
    struct newfs_dentry* dentry_cursor = inode->dentrys;
    int    cnt = 0;
    while (dentry_cursor)
    {
        if (dir == cnt) {
            return dentry_cursor;
        }
        cnt++;
        dentry_cursor = dentry_cursor->brother;
    }
    return NULL;
}

/**
 * @brief 跟据path找到目录项
 * path: /qwe/ad  total_lvl = 2,
 *      1) find /'s inode       lvl = 1
 *      2) find qwe's dentry 
 *      3) find qwe's inode     lvl = 2
 *      4) find ad's dentry
 *
 * path: /qwe     total_lvl = 1,
 *      1) find /'s inode       lvl = 1
 *      2) find qwe's dentry
 *  
 * @param path 
 * @return struct newfs_inode* 
 */
struct newfs_dentry* newfs_lookup(const char * path, boolean* is_find, boolean* is_root) {
    struct newfs_dentry* dentry_cursor = newfs_super.root_dentry;
    struct newfs_dentry* dentry_ret = NULL;
    struct newfs_inode*  inode; 
    int   total_lvl = newfs_calc_lvl(path);
    int   lvl = 0;
    boolean is_hit;
    char* fname = NULL;
    char* path_cpy = (char*)malloc(sizeof(path));
    *is_root = FALSE;
    strcpy(path_cpy, path);

    if (total_lvl == 0) {                           /* 根目录 */
        *is_find = TRUE;
        *is_root = TRUE;
        dentry_ret = newfs_super.root_dentry;
    }
    fname = strtok(path_cpy, "/");       
    while (fname)
    {   // 按目录层级深入
        lvl++;
        if (dentry_cursor->inode == NULL) {           /* Cache机制 */
            newfs_read_inode(dentry_cursor, dentry_cursor->ino);
        }

        inode = dentry_cursor->inode;
        // 到了某个层级发现不是文件夹而是文件，返回这个文件的dentry
        if (NEWFS_IS_REG(inode) && lvl < total_lvl) {
            NEWFS_DBG("[%s] not a dir\n", __func__);
            dentry_ret = inode->dentry;
            break;
        }
        if (NEWFS_IS_DIR(inode)) {
            // 是文件夹，进入
            dentry_cursor = inode->dentrys;
            is_hit        = FALSE;

            while (dentry_cursor)
            {   
                // 遍历同级目录，找到名字匹配的文件夹
                if (memcmp(dentry_cursor->fname, fname, strlen(fname)) == 0) {
                    is_hit = TRUE;
                    break;
                }
                dentry_cursor = dentry_cursor->brother;
            }
            // 没有找到匹配的文件（夹）名，返回上一级的dentry
            if (!is_hit) {
                *is_find = FALSE;
                NEWFS_DBG("[%s] not found %s\n", __func__, fname);
                dentry_ret = inode->dentry;
                break;
            }
            // 找到了，层级数也是最终的，返回dentry游标
            if (is_hit && lvl == total_lvl) {
                *is_find = TRUE;
                dentry_ret = dentry_cursor;
                break;
            }
        }
        fname = strtok(NULL, "/"); 
    }

    if (dentry_ret->inode == NULL) {
        dentry_ret->inode = newfs_read_inode(dentry_ret, dentry_ret->ino);
    }
    
    return dentry_ret;
}


/**
 * @brief 
 * 
 * @return int 
 */
int newfs_umount() {
    struct newfs_super_d  newfs_super_d; 

    if (!newfs_super.is_mounted) {
        return NEWFS_ERROR_NONE;
    }

    newfs_sync_inode(newfs_super.root_dentry->inode);     /* 从根节点向下刷写节点 */
                                                    
    newfs_super_d.magic_num         = NEWFS_MAGIC_NUM;
    newfs_super_d.map_inode_blks    = newfs_super.map_inode_blks;
    newfs_super_d.map_inode_offset  = newfs_super.map_inode_offset;

    // 数据位图
    newfs_super_d.map_data_blks     = newfs_super.map_data_blks;
    newfs_super_d.map_data_offset   = newfs_super.map_data_offset;


    newfs_super_d.inode_offset      = newfs_super.inode_offset;
    newfs_super_d.data_offset       = newfs_super.data_offset;
    newfs_super_d.sz_usage          = newfs_super.sz_usage;

    newfs_super_d.max_ino           = newfs_super.max_ino;
    newfs_super_d.max_data          = newfs_super.max_data; 

    if (newfs_driver_write(NEWFS_SUPER_OFS, (uint8_t *)&newfs_super_d, 
                     sizeof(struct newfs_super_d)) != NEWFS_ERROR_NONE) {
        return -NEWFS_ERROR_IO;
    }
    // 写回inode位图
    if (newfs_driver_write(newfs_super_d.map_inode_offset, (uint8_t *)(newfs_super.map_inode), 
                         NEWFS_BLKS_SZ(newfs_super_d.map_inode_blks)) != NEWFS_ERROR_NONE) {
        return -NEWFS_ERROR_IO;
    }
    // 写回data位图
    if (newfs_driver_write(newfs_super_d.map_data_offset, (uint8_t *)(newfs_super.map_data), 
                         NEWFS_BLKS_SZ(newfs_super_d.map_data_blks)) != NEWFS_ERROR_NONE) {
        return -NEWFS_ERROR_IO;
    }

    free(newfs_super.map_inode);
    ddriver_close(NEWFS_DRIVER());

    return NEWFS_ERROR_NONE;
}

/**
 * @brief 挂载newfs, Layout 如下
 * 
 * Layout
 * | Super | Inode Map | Data |
 * 
 * IO_SZ = BLK_SZ
 * 
 * 每个Inode占用一个Blk
 * @param options 
 * @return int 
 */
int newfs_mount(struct custom_options options){
    int                 ret = NEWFS_ERROR_NONE;
    int                 driver_fd;
    struct newfs_super_d  newfs_super_d; 
    struct newfs_dentry*  root_dentry;
    struct newfs_inode*   root_inode;

    int                 inode_num;
    int                 map_inode_blks;
    int                 map_data_blks;
    int                 inode_blks;
    
    int                 super_blks;
    boolean             is_init = FALSE;

    newfs_super.is_mounted = FALSE;

    // driver_fd = open(options.device, O_RDWR);
    driver_fd = ddriver_open(options.device);

    if (driver_fd < 0) {
        return driver_fd;
    }

    newfs_super.driver_fd = driver_fd;
    ddriver_ioctl(NEWFS_DRIVER(), IOC_REQ_DEVICE_SIZE,  &newfs_super.sz_disk);
    ddriver_ioctl(NEWFS_DRIVER(), IOC_REQ_DEVICE_IO_SZ, &newfs_super.sz_io);
    printf("!!!!io size:%d!!!!",newfs_super.sz_io);
    // 块大小1k
    newfs_super.sz_blk = 1024;
    
    root_dentry = new_dentry("/", NEWFS_DIR);
    int a = sizeof(struct newfs_inode_d);
    int a1 = sizeof(int);
    // int a2 = sizeof(enum);
    int b = sizeof(struct newfs_super_d);
    int c = sizeof(struct newfs_dentry_d);
    // 读取super块
    if (newfs_driver_read(NEWFS_SUPER_OFS, (uint8_t *)(&newfs_super_d), sizeof(struct newfs_super_d)) != NEWFS_ERROR_NONE) {
        return -NEWFS_ERROR_IO;
    }   

    // 幻数判断
    if (newfs_super_d.magic_num != NEWFS_MAGIC_NUM) {     
       
        // super_blks块数
        // super_blks = NEWFS_ROUND_UP(sizeof(struct newfs_super_d), NEWFS_BLK_SZ()) / NEWFS_BLK_SZ();
        super_blks = NEWFS_ROUND_UP(sizeof(struct newfs_super_d), NEWFS_BLK_SZ()) / NEWFS_BLK_SZ();

        // inode_num  =  NEWFS_DISK_SZ() / NEWFS_BLK_SZ();
        inode_num  =  512;
        map_inode_blks = NEWFS_ROUND_UP(NEWFS_ROUND_UP(inode_num, UINT32_BITS), NEWFS_BLK_SZ()) / NEWFS_BLK_SZ();
        
        
        // 新增数据位图
        // map_data_blks = NEWFS_ROUND_UP(NEWFS_ROUND_UP(NEWFS_DATA_PER_FILE * inode_num, UINT32_BITS), NEWFS_BLK_SZ())  / NEWFS_BLK_SZ();
        map_data_blks = 1;

        // inode块数
        inode_blks = NEWFS_ROUND_UP(sizeof(struct newfs_inode_d) * inode_num, NEWFS_BLK_SZ()) / NEWFS_BLK_SZ();
        
                                                      /* 布局layout */
        // 最多支持的文件数
        newfs_super_d.max_ino           = inode_num;
        // 最多的数据块数 
        newfs_super_d.max_data          = NEWFS_DISK_SZ()/NEWFS_BLK_SZ() - super_blks - map_data_blks - map_inode_blks - inode_blks; 
        newfs_super_d.map_inode_offset  = NEWFS_SUPER_OFS + NEWFS_BLKS_SZ(super_blks);
        newfs_super_d.map_data_offset   = newfs_super_d.map_inode_offset + NEWFS_BLKS_SZ(map_inode_blks);
        
        newfs_super_d.inode_offset      = newfs_super_d.map_data_offset + NEWFS_BLKS_SZ(map_data_blks);
        newfs_super_d.data_offset       = newfs_super_d.inode_offset + NEWFS_BLKS_SZ(inode_blks);

        newfs_super_d.map_inode_blks    = map_inode_blks;
        newfs_super_d.map_data_blks     = map_data_blks;
        newfs_super_d.sz_usage          = 0;

        is_init = TRUE;
    }
    newfs_super.sz_usage            = newfs_super_d.sz_usage;   

    newfs_super.map_inode           = (uint8_t *)malloc(NEWFS_BLKS_SZ(newfs_super_d.map_inode_blks));
    newfs_super.map_inode_blks      = newfs_super_d.map_inode_blks;
    newfs_super.map_inode_offset    = newfs_super_d.map_inode_offset;

    newfs_super.map_data            = (uint8_t *)malloc(NEWFS_BLKS_SZ(newfs_super_d.map_data_blks));
    newfs_super.map_data_blks       = newfs_super_d.map_data_blks;
    newfs_super.map_data_offset     = newfs_super_d.map_data_offset;

    newfs_super.inode_offset        = newfs_super_d.inode_offset;
    newfs_super.data_offset         = newfs_super_d.data_offset;
    // 最多支持的文件数
    newfs_super.max_ino             = newfs_super_d.max_ino  ;
    // 最多的数据块数 
    newfs_super.max_data            = newfs_super_d.max_data   ; 
    
    // newfs_dump_map(0);
    if (is_init) {
        // 初始化位图
        memset(newfs_super.map_inode, 0, NEWFS_BLKS_SZ(newfs_super_d.map_inode_blks));
        memset(newfs_super.map_data, 0, NEWFS_BLKS_SZ(newfs_super_d.map_data_blks));
    } else {
        // 读入位图
        if (newfs_driver_read(newfs_super_d.map_inode_offset, (uint8_t *)(newfs_super.map_inode), 
        NEWFS_BLKS_SZ(newfs_super_d.map_inode_blks)) != NEWFS_ERROR_NONE){
            return -NEWFS_ERROR_IO;
        }
        if (newfs_driver_read(newfs_super_d.map_data_offset, (uint8_t *)(newfs_super.map_data),
                NEWFS_BLKS_SZ(newfs_super_d.map_data_blks)) != NEWFS_ERROR_NONE){
            return -NEWFS_ERROR_IO;
        }
    }

     newfs_dump_map(0);
    if (is_init) {                                    /* 分配根节点 */
        root_inode = newfs_alloc_inode(root_dentry);
        newfs_sync_inode(root_inode);
    }

    // newfs_dump_map(0);
    root_inode              = newfs_read_inode(root_dentry, NEWFS_ROOT_INO);
    root_dentry->inode      = root_inode;
    newfs_super.root_dentry = root_dentry;
    newfs_super.is_mounted  = TRUE;

    // newfs_dump_map(0);
    // newfs_dump_map(1);
    return ret;
}
