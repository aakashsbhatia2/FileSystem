#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pagemap.h> 	
#include <linux/fs.h>     	
#include <asm/atomic.h>
#include <asm/uaccess.h>	
#include <linux/init_task.h>
#include <linux/sched.h>
#include <linux/pid.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aakash Bhatia");

#define S2FS_MAGIC 0x19980122
struct task_struct *task;
struct task_struct *task_child;
struct task_struct *task2;
int p_id;

void gettask(struct task_struct *task){
	
	struct list_head *list;
	
	if (p_id == task_child->pid){
                task2 = task_child;
        }

	list_for_each(list, &task->children){
                task_child = list_entry(list, struct task_struct, sibling);
		gettask(task_child);
        }
}


static int get_task_info(int pid, char* data){
	p_id = pid;
	char data_buf[1024]="";

	for(task = current; task!=&init_task; task = task->parent);
	gettask(task);

	if(task2){
	
		sprintf(data_buf, "Task Name: %s\nTask State: %ld\nProcess ID: %d\nCPU ID: %d\nThread Group ID: %d\nParent's PID: %d\nStart Time: %llu\nDynamic Priority: %d\nStatic Priority: %d\nNormal Priority: %d\nReal-time Priority: %d\n", task2->comm, task2->state, task2->pid, task2->cpu, task2->tgid, task2->parent->pid,task2->start_time, task2->prio, task2->static_prio, task2->normal_prio, task2->rt_priority);
	
		strcpy(data, data_buf);
	
		if(task2->mm!=0 && task2->mm->mmap_base!=0){
        		sprintf(data_buf, "Memory Map Base: %ld\n", task2->mm->mmap_base);
			strcat(data, data_buf);
        	}
		else{
			sprintf(data_buf, "Memory Map Base is empty");
			strcat(data, data_buf);
		}
		if(task2->mm!=0 && task2->mm->task_size!=0){
			sprintf(data_buf, "Virtual Memory Space: %lu\n", task2->mm->task_size);
        		strcat(data, data_buf);
		}
		else{
			sprintf(data_buf, "Virtual Memory Space is empty");
			strcat(data, data_buf);
		}
		if(task2->mm!=0 && task2->acct_vm_mem1!=0){
			sprintf(data_buf, "Virtual Memory Usage: %llu\n", task2->acct_vm_mem1);
        		strcat(data, data_buf);
		}
		else{
			sprintf(data_buf, "Virtual Memory Usage is empty");
			strcat(data, data_buf);
		}
		if(task2->mm!=0 && task2->mm->map_count!=0){
			sprintf(data_buf, "Number of Virtual Memory Addresses: %d\n" ,task2->mm->map_count);
			strcat(data, data_buf);
		}
		else{
			sprintf(data_buf, "Number of Virtual Memory Addresses is empty");
			strcat(data, data_buf);
		}
		if(task2->mm!=0 && task2->mm->total_vm!=0){
			sprintf(data_buf, "Total Pages Mapped: %lu\n", task2->mm->total_vm);
			strcat(data, data_buf);
		}
		else{
			sprintf(data_buf, "Total Pages Mapped is empty");
			strcat(data, data_buf);
		}
		return sizeof(data);
	}
	return -1;
}

static struct inode *s2fs_make_inode(struct super_block *sb, int mode)
{
	struct inode *ret = new_inode(sb);

	if (ret) {
		ret->i_ino = get_next_ino();
		ret->i_mode = mode;
		ret->i_uid.val = ret->i_gid.val = 0;
		ret->i_blocks = 0;
		ret->i_atime = ret->i_mtime = ret->i_ctime = current_kernel_time64();
	}
	return ret;
}

static int s2fs_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;
	return 0;
}

#define TMPSIZE 2000

static ssize_t s2fs_read_file(struct file *filp, char *buf,
		size_t count, loff_t *offset)
{
	int len;
	char tmp[TMPSIZE];
	int j;
	char data_buf2[1024];
	int k;

	k= (int)(filp->private_data);		
	j = get_task_info(k, data_buf2);

	if(j>=0){
		len = snprintf(tmp, TMPSIZE, "%s\n",data_buf2);
	}
	else{
		len = snprintf(tmp, TMPSIZE, "%s\n", "Task no longer exists");
	}
	if (*offset > len)
		return 0;
	if (count > len - *offset)
		count = len - *offset;

	if (copy_to_user(buf, tmp + *offset, count))
		return -EFAULT;
	*offset += count;
	return count;
}


static ssize_t s2fs_write_file(struct file *filp, const char *buf,
		size_t count, loff_t *offset)
{
	return 0;
}


static struct file_operations s2fs_file_ops = {
	.open	= s2fs_open,
	.read 	= s2fs_read_file,
	.write  = s2fs_write_file,
};

static struct dentry *s2fs_create_file (struct super_block *sb,
		struct dentry *dir, const char *name,
		atomic_t *counter)
{
	struct dentry *dentry;
	struct inode *inode;
	struct qstr qname;
	int process_id;

	qname.name = name;
	qname.len = strlen (name);
	qname.hash = full_name_hash(dir, name, qname.len);

	dentry = d_alloc(dir, &qname);
	if (! dentry)
		goto out;
	inode = s2fs_make_inode(sb, S_IFREG | 0644);
	if (! inode)
		goto out_dput;
	inode->i_fop = &s2fs_file_ops;
	kstrtoint(name, 10, &process_id);
	inode->i_private = process_id;

	d_add(dentry, inode);
	return dentry;

  out_dput:
	dput(dentry);
  out:
	return 0;
}

static struct dentry *s2fs_create_dir (struct super_block *sb,
		struct dentry *parent, const char *name)
{
	struct dentry *dentry;
	struct inode *inode;
	struct qstr qname;

	qname.name = name;
	qname.len = strlen (name);
	qname.hash = full_name_hash(parent, name, qname.len);
	dentry = d_alloc(parent, &qname);
	if (! dentry)
		goto out;

	inode = s2fs_make_inode(sb, S_IFDIR | 0644);
	if (! inode)
		goto out_dput;
	inode->i_op = &simple_dir_inode_operations;
	inode->i_fop = &simple_dir_operations;

	d_add(dentry, inode);
	return dentry;

  out_dput:
	dput(dentry);
  out:
	return 0;
}

static struct super_operations s2fs_s_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};

static atomic_t  subcounter;

static void s2fs_create_files (struct super_block *sb, struct dentry *root, struct task_struct *task)
{
	struct dentry *subdir;
	struct list_head *list;
	char ch[100];
	int x = 0;
	sprintf(ch, "%d", task->pid);
	
	list_for_each(list, &task->children){
		x=x+1;
	}
	
	if(x>0){
		subdir = s2fs_create_dir(sb, root, ch);
                s2fs_create_file(sb, subdir, ch, &subcounter);
        }
        else{
        	s2fs_create_file(sb, root, ch, &subcounter);
        }
	
	list_for_each(list, &task->children){
                task_child = list_entry(list, struct task_struct, sibling);	
		if(subdir){
			s2fs_create_files(sb, subdir, task_child);
		}
	}
}

static int s2fs_fill_super (struct super_block *sb, void *data, int silent){
        struct inode *root;
        struct dentry *root_dentry;

        sb->s_magic = S2FS_MAGIC;
        sb->s_op = &s2fs_s_ops;

        root = s2fs_make_inode (sb, S_IFDIR | 0755);
        if (! root)
                goto out;
        root->i_op = &simple_dir_inode_operations;
        root->i_fop = &simple_dir_operations;

        root_dentry = d_make_root(root);
        if (! root_dentry)
                goto out_iput;
        sb->s_root = root_dentry;
	
	for(task = current; task != &init_task; task = task->parent);
	
        s2fs_create_files(sb, root_dentry,task);
        return 0;

  out_iput:
        iput(root);
  out:
        return -ENOMEM;
}

 
static struct dentry *s2fs_get_super(struct file_system_type *fst, int flags, const char *devname, void *data)
{
	return mount_nodev(fst, flags, data, s2fs_fill_super);
}

static struct file_system_type s2fs_type = {
	.owner 		= THIS_MODULE,
	.name		= "s2fs",
	.mount		= s2fs_get_super,
	.kill_sb	= kill_litter_super,
};

static int __init s2fs_init(void)
{
	return register_filesystem(&s2fs_type);
}

static void __exit s2fs_exit(void)
{
	unregister_filesystem(&s2fs_type);
}

module_init(s2fs_init);
module_exit(s2fs_exit);
