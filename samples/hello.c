int main () {
    int i = 0;
    int sum = 0;

    for (; i <= 10; i++)
        sum += i;

    return sum;
}

void _start() {

    /* main body of program: call main(), etc */
    main ();
    /* exit system call */
    asm("mov %rax ,%rdi;"
        "mov $60,%rax;"
        "syscall"
    );
}

