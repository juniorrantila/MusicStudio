#pragma once
#include <Ty/Base.h>

namespace Core {

enum class Syscall : iptr {
#ifdef __linux__ // FIXME: Assumes x86_64
    read = 0,
    write = 1,
    open = 2,
    close = 3,
    stat = 4,
    fstat = 5,
    lstat = 6,
    poll = 7,
    lseek = 8,
    mmap = 9,
    mprotect = 10,
    munmap = 11,
    brk = 12,
    rt_sigaction = 13,
    rt_sigprocmask = 14,
    rt_sigreturn = 15,
    ioctl = 16,
    pread64 = 17,
    pwrite64 = 18,
    readv = 19,
    writev = 20,
    access = 21,
    pipe = 22,
    select = 23,
    sched_yield = 24,
    mremap = 25,
    msync = 26,
    mincore = 27,
    madvise = 28,
    shmget = 29,
    shmat = 30,
    shmctl = 31,
    dup = 32,
    dup2 = 33,
    pause = 34,
    nanosleep = 35,
    getitimer = 36,
    alarm = 37,
    setitimer = 38,
    getpid = 39,
    sendfile = 40,
    socket = 41,
    connect = 42,
    accept = 43,
    sendto = 44,
    recvfrom = 45,
    sendmsg = 46,
    recvmsg = 47,
    shutdown = 48,
    bind = 49,
    listen = 50,
    getsockname = 51,
    getpeername = 52,
    socketpair = 53,
    setsockopt = 54,
    getsockopt = 55,
    clone = 56,
    fork = 57,
    vfork = 58,
    execve = 59,
    exit = 60,
    wait4 = 61,
    kill = 62,
    uname = 63,
    semget = 64,
    semop = 65,
    semctl = 66,
    shmdt = 67,
    msgget = 68,
    msgsnd = 69,
    msgrcv = 70,
    msgctl = 71,
    fcntl = 72,
    flock = 73,
    fsync = 74,
    fdatasync = 75,
    truncate = 76,
    ftruncate = 77,
    getdents = 78,
    getcwd = 79,
    chdir = 80,
    fchdir = 81,
    rename = 82,
    mkdir = 83,
    rmdir = 84,
    creat = 85,
    link = 86,
    unlink = 87,
    symlink = 88,
    readlink = 89,
    chmod = 90,
    fchmod = 91,
    chown = 92,
    fchown = 93,
    lchown = 94,
    umask = 95,
    gettimeofday = 96,
    getrlimit = 97,
    getrusage = 98,
    sysinfo = 99,
    times = 100,
    ptrace = 101,
    getuid = 102,
    syslog = 103,
    getgid = 104,
    setuid = 105,
    setgid = 106,
    geteuid = 107,
    getegid = 108,
    setpgid = 109,
    getppid = 110,
    getpgrp = 111,
    setsid = 112,
    setreuid = 113,
    setregid = 114,
    getgroups = 115,
    setgroups = 116,
    setresuid = 117,
    getresuid = 118,
    setresgid = 119,
    getresgid = 120,
    getpgid = 121,
    setfsuid = 122,
    setfsgid = 123,
    getsid = 124,
    capget = 125,
    capset = 126,
    rt_sigpending = 127,
    rt_sigtimedwait = 128,
    rt_sigqueueinfo = 129,
    rt_sigsuspend = 130,
    sigaltstack = 131,
    utime = 132,
    mknod = 133,
    uselib = 134,
    personality = 135,
    ustat = 136,
    statfs = 137,
    fstatfs = 138,
    sysfs = 139,
    getpriority = 140,
    setpriority = 141,
    sched_setparam = 142,
    sched_getparam = 143,
    sched_setscheduler = 144,
    sched_getscheduler = 145,
    sched_get_priority_max = 146,
    sched_get_priority_min = 147,
    sched_rr_get_interval = 148,
    mlock = 149,
    munlock = 150,
    mlockall = 151,
    munlockall = 152,
    vhangup = 153,
    modify_ldt = 154,
    pivot_root = 155,
    _sysctl = 156,
    prctl = 157,
    arch_prctl = 158,
    adjtimex = 159,
    setrlimit = 160,
    chroot = 161,
    sync = 162,
    acct = 163,
    settimeofday = 164,
    mount = 165,
    umount2 = 166,
    swapon = 167,
    swapoff = 168,
    reboot = 169,
    sethostname = 170,
    setdomainname = 171,
    iopl = 172,
    ioperm = 173,
    create_module = 174,
    init_module = 175,
    delete_module = 176,
    get_kernel_syms = 177,
    query_module = 178,
    quotactl = 179,
    nfsservctl = 180,
    getpmsg = 181,
    putpmsg = 182,
    afs_syscall = 183,
    tuxcall = 184,
    security = 185,
    gettid = 186,
    readahead = 187,
    setxattr = 188,
    lsetxattr = 189,
    fsetxattr = 190,
    getxattr = 191,
    lgetxattr = 192,
    fgetxattr = 193,
    listxattr = 194,
    llistxattr = 195,
    flistxattr = 196,
    removexattr = 197,
    lremovexattr = 198,
    fremovexattr = 199,
    tkill = 200,
    time = 201,
    futex = 202,
    sched_setaffinity = 203,
    sched_getaffinity = 204,
    set_thread_area = 205,
    io_setup = 206,
    io_destroy = 207,
    io_getevents = 208,
    io_submit = 209,
    io_cancel = 210,
    get_thread_area = 211,
    lookup_dcookie = 212,
    epoll_create = 213,
    epoll_ctl_old = 214,
    epoll_wait_old = 215,
    remap_file_pages = 216,
    getdents64 = 217,
    set_tid_address = 218,
    restart_syscall = 219,
    semtimedop = 220,
    fadvise64 = 221,
    timer_create = 222,
    timer_settime = 223,
    timer_gettime = 224,
    timer_getoverrun = 225,
    timer_delete = 226,
    clock_settime = 227,
    clock_gettime = 228,
    clock_getres = 229,
    clock_nanosleep = 230,
    exit_group = 231,
    epoll_wait = 232,
    epoll_ctl = 233,
    tgkill = 234,
    utimes = 235,
    vserver = 236,
    mbind = 237,
    set_mempolicy = 238,
    get_mempolicy = 239,
    mq_open = 240,
    mq_unlink = 241,
    mq_timedsend = 242,
    mq_timedreceive = 243,
    mq_notify = 244,
    mq_getsetattr = 245,
    kexec_load = 246,
    waitid = 247,
    add_key = 248,
    request_key = 249,
    keyctl = 250,
    ioprio_set = 251,
    ioprio_get = 252,
    inotify_init = 253,
    inotify_add_watch = 254,
    inotify_rm_watch = 255,
    migrate_pages = 256,
    openat = 257,
    mkdirat = 258,
    mknodat = 259,
    fchownat = 260,
    futimesat = 261,
    newfstatat = 262,
    unlinkat = 263,
    renameat = 264,
    linkat = 265,
    symlinkat = 266,
    readlinkat = 267,
    fchmodat = 268,
    faccessat = 269,
    pselect6 = 270,
    ppoll = 271,
    unshare = 272,
    set_robust_list = 273,
    get_robust_list = 274,
    splice = 275,
    tee = 276,
    sync_file_range = 277,
    vmsplice = 278,
    move_pages = 279,
    utimensat = 280,
    epoll_pwait = 281,
    signalfd = 282,
    timerfd_create = 283,
    eventfd = 284,
    fallocate = 285,
    timerfd_settime = 286,
    timerfd_gettime = 287,
    accept4 = 288,
    signalfd4 = 289,
    eventfd2 = 290,
    epoll_create1 = 291,
    dup3 = 292,
    pipe2 = 293,
    inotify_init1 = 294,
    preadv = 295,
    pwritev = 296,
    rt_tgsigqueueinfo = 297,
    perf_event_open = 298,
    recvmmsg = 299,
    fanotify_init = 300,
    fanotify_mark = 301,
    prlimit64 = 302,
    name_to_handle_at = 303,
    open_by_handle_at = 304,
    clock_adjtime = 305,
    syncfs = 306,
    sendmmsg = 307,
    setns = 308,
    getcpu = 309,
    process_vm_readv = 310,
    process_vm_writev = 311,
    kcmp = 312,
    finit_module = 313,
    sched_setattr = 314,
    sched_getattr = 315,
    renameat2 = 316,
    seccomp = 317,
    getrandom = 318,
    memfd_create = 319,
    kexec_file_load = 320,
    bpf = 321,
    execveat = 322,
    userfaultfd = 323,
    membarrier = 324,
    mlock2 = 325,
    copy_file_range = 326,
    preadv2 = 327,
    pwritev2 = 328,
    pkey_mprotect = 329,
    pkey_alloc = 330,
    pkey_free = 331,
    statx = 332,
    io_pgetevents = 333,
    rseq = 334,
    pidfd_send_signal = 424,
    io_uring_setup = 425,
    io_uring_enter = 426,
    io_uring_register = 427,
    open_tree = 428,
    move_mount = 429,
    fsopen = 430,
    fsconfig = 431,
    fsmount = 432,
    fspick = 433,
    pidfd_open = 434,
    clone3 = 435,
    close_range = 436,
    openat2 = 437,
    pidfd_getfd = 438,
    faccessat2 = 439,
    process_madvise = 440,
    epoll_pwait2 = 441,
    mount_setattr = 442,
    quotactl_fd = 443,
    landlock_create_ruleset = 444,
    landlock_add_rule = 445,
    landlock_restrict_self = 446,
    memfd_secret = 447,
    process_mrelease = 448,
    futex_waitv = 449,
    set_mempolicy_home_node = 450,
#elif __APPLE__
    syscall = 0,
    exit = 1,
    fork = 2,
    read = 3,
    write = 4,
    open = 5,
    close = 6,
    wait4 = 7,
    // 8  old creat
    link = 9,
    unlink = 10,
    // 11  old execv
    chdir = 12,
    fchdir = 13,
    mknod = 14,
    chmod = 15,
    chown = 16,
    // 17  old break
    getfsstat = 18,
    // 19  old lseek
    getpid = 20,
    // 21  old mount
    // 22  old umount
    setuid = 23,
    getuid = 24,
    geteuid = 25,
    ptrace = 26,
    recvmsg = 27,
    sendmsg = 28,
    recvfrom = 29,
    accept = 30,
    getpeername = 31,
    getsockname = 32,
    access = 33,
    chflags = 34,
    fchflags = 35,
    sync = 36,
    kill = 37,
    // 38  old stat
    getppid = 39,
    // 40  old lstat
    dup = 41,
    pipe = 42,
    getegid = 43,
    // 44  old profil
    // 45  old ktrace
    sigaction = 46,
    getgid = 47,
    sigprocmask = 48,
    getlogin = 49,
    setlogin = 50,
    acct = 51,
    sigpending = 52,
    sigaltstack = 53,
    ioctl = 54,
    reboot = 55,
    revoke = 56,
    symlink = 57,
    readlink = 58,
    execve = 59,
    umask = 60,
    chroot = 61,
    // 62  old fstat
    // 63  used internally and reserved
    // 64  old getpagesize
    msync = 65,
    vfork = 66,
    // 67  old vread
    // 68  old vwrite
    // 69  old sbrk
    // 70  old sstk
    // 71  old mmap
    // 72  old vadvise
    munmap = 73,
    mprotect = 74,
    madvise = 75,
    // 76  old vhangup
    // 77  old vlimit
    mincore = 78,
    getgroups = 79,
    setgroups = 80,
    getpgrp = 81,
    setpgid = 82,
    setitimer = 83,
    // 84  old wait
    swapon = 85,
    getitimer = 86,
    // 87  old gethostname
    // 88  old sethostname
    getdtablesize = 89,
    dup2 = 90,
    // 91  old getdopt
    fcntl = 92,
    select = 93,
    // 94  old setdopt
    fsync = 95,
    setpriority = 96,
    socket = 97,
    connect = 98,
    // 99  old accept
    getpriority = 100,
    // 101  old send
    // 102  old recv
    // 103  old sigreturn
    bind = 104,
    setsockopt = 105,
    listen = 106,
    // 107  old vtimes
    // 108  old sigvec
    // 109  old sigblock
    // 110  old sigsetmask
    sigsuspend = 111,
    // 112  old sigstack
    // 113  old recvmsg
    // 114  old sendmsg
    // 115  old vtrace
    gettimeofday = 116,
    getrusage = 117,
    getsockopt = 118,
    // 119  old resuba
    readv = 120,
    writev = 121,
    settimeofday = 122,
    fchown = 123,
    fchmod = 124,
    // 125  old recvfrom
    setreuid = 126,
    setregid = 127,
    rename = 128,
    // 129  old truncate
    // 130  old ftruncate
    flock = 131,
    mkfifo = 132,
    sendto = 133,
    shutdown = 134,
    socketpair = 135,
    mkdir = 136,
    rmdir = 137,
    utimes = 138,
    futimes = 139,
    adjtime = 140,
    // 141  old getpeername
    gethostuuid = 142,
    // 143  old sethostid
    // 144  old getrlimit
    // 145  old setrlimit
    // 146  old killpg
    setsid = 147,
    // 148  old setquota
    // 149  old qquota
    // 150  old getsockname
    getpgid = 151,
    setprivexec = 152,
    pread = 153,
    pwrite = 154,
    nfssvc = 155,
    // 156  old getdirentries
    statfs = 157,
    fstatfs = 158,
    unmount = 159,
    // 160  old async_daemon
    getfh = 161,
    // 162  old getdomainname
    // 163  old setdomainname
    // 164
    quotactl = 165,
    // 166  old exportfs
    mount = 167,
    // 168  old ustat
    csops = 169,
    csops_audittoken = 170,
    // 171  old wait3
    // 172  old rpause
    waitid = 173,
    // 174  old getdents
    // 175  old gc_control
    // 176  old add_profil
    kdebug_typefilter = 177,
    kdebug_trace_string = 178,
    kdebug_trace64 = 179,
    kdebug_trace = 180,
    setgid = 181,
    setegid = 182,
    seteuid = 183,
    sigreturn = 184,
    // 185  old chud
    thread_selfcounts = 186,
    fdatasync = 187,
    stat = 188,
    fstat = 189,
    lstat = 190,
    pathconf = 191,
    fpathconf = 192,
    // 193  old getfsstat
    getrlimit = 194,
    setrlimit = 195,
    getdirentries = 196,
    mmap = 197,
    // 198  old __syscall
    lseek = 199,
    truncate = 200,
    ftruncate = 201,
    sysctl = 202,
    mlock = 203,
    munlock = 204,
    undelete = 205,
    // 206  old ATsocket
    // 207  old ATgetmsg
    // 208  old ATputmsg
    // 209  old ATsndreq
    // 210  old ATsndrsp
    // 211  old ATgetreq
    // 212  old ATgetrsp
    // 213  Reserved for AppleTalk
    // 214
    // 215
    open_dprotected_np = 216,
    fsgetpath_ext = 217,
    // 218  old lstatv
    // 219  old fstatv
    getattrlist = 220,
    setattrlist = 221,
    getdirentriesattr = 222,
    exchangedata = 223,
    // 224  old checkuseraccess or fsgetpath
    searchfs = 225,
    delete_ = 226,
    copyfile = 227,
    fgetattrlist = 228,
    fsetattrlist = 229,
    poll = 230,
    // 231  old watchevent
    // 232  old waitevent
    // 233  old modwatch
    getxattr = 234,
    fgetxattr = 235,
    setxattr = 236,
    fsetxattr = 237,
    removexattr = 238,
    fremovexattr = 239,
    listxattr = 240,
    flistxattr = 241,
    fsctl = 242,
    initgroups = 243,
    posix_spawn = 244,
    ffsctl = 245,
    // 246
    nfsclnt = 247,
    fhopen = 248,
    // 249
    minherit = 250,
    semsys = 251,
    msgsys = 252,
    shmsys = 253,
    semctl = 254,
    semget = 255,
    semop = 256,
    // 257  old semconfig
    msgctl = 258,
    msgget = 259,
    msgsnd = 260,
    msgrcv = 261,
    shmat = 262,
    shmctl = 263,
    shmdt = 264,
    shmget = 265,
    shm_open = 266,
    shm_unlink = 267,
    sem_open = 268,
    sem_close = 269,
    sem_unlink = 270,
    sem_wait = 271,
    sem_trywait = 272,
    sem_post = 273,
    sysctlbyname = 274,
    // 275  old sem_init
    // 276  old sem_destroy
    open_extended = 277,
    umask_extended = 278,
    stat_extended = 279,
    lstat_extended = 280,
    fstat_extended = 281,
    chmod_extended = 282,
    fchmod_extended = 283,
    access_extended = 284,
    settid = 285,
    gettid = 286,
    setsgroups = 287,
    getsgroups = 288,
    setwgroups = 289,
    getwgroups = 290,
    mkfifo_extended = 291,
    mkdir_extended = 292,
    identitysvc = 293,
    shared_region_check_np = 294,
    // 295  old shared_region_map_np
    vm_pressure_monitor = 296,
    psynch_rw_longrdlock = 297,
    psynch_rw_yieldwrlock = 298,
    psynch_rw_downgrade = 299,
    psynch_rw_upgrade = 300,
    psynch_mutexwait = 301,
    psynch_mutexdrop = 302,
    psynch_cvbroad = 303,
    psynch_cvsignal = 304,
    psynch_cvwait = 305,
    psynch_rw_rdlock = 306,
    psynch_rw_wrlock = 307,
    psynch_rw_unlock = 308,
    psynch_rw_unlock2 = 309,
    getsid = 310,
    settid_with_pid = 311,
    psynch_cvclrprepost = 312,
    aio_fsync = 313,
    aio_return = 314,
    aio_suspend = 315,
    aio_cancel = 316,
    aio_error = 317,
    aio_read = 318,
    aio_write = 319,
    lio_listio = 320,
    // 321  old __pthread_cond_wait
    iopolicysys = 322,
    process_policy = 323,
    mlockall = 324,
    munlockall = 325,
    // 326
    issetugid = 327,
    __pthread_kill = 328,
    __pthread_sigmask = 329,
    __sigwait = 330,
    __disable_threadsignal = 331,
    __pthread_markcancel = 332,
    __pthread_canceled = 333,
    __semwait_signal = 334,
    // 335  old utrace
    proc_info = 336,
    sendfile = 337,
    stat64 = 338,
    fstat64 = 339,
    lstat64 = 340,
    stat64_extended = 341,
    lstat64_extended = 342,
    fstat64_extended = 343,
    getdirentries64 = 344,
    statfs64 = 345,
    fstatfs64 = 346,
    getfsstat64 = 347,
    __pthread_chdir = 348,
    __pthread_fchdir = 349,
    audit = 350,
    auditon = 351,
    // 352
    getauid = 353,
    setauid = 354,
    // 355  old getaudit
    // 356  old setaudit
    getaudit_addr = 357,
    setaudit_addr = 358,
    auditctl = 359,
    bsdthread_create = 360,
    bsdthread_terminate = 361,
    kqueue = 362,
    kevent = 363,
    lchown = 364,
    // 365  old stack_snapshot
    bsdthread_register = 366,
    workq_open = 367,
    workq_kernreturn = 368,
    kevent64 = 369,
    __old_semwait_signal = 370,
    __old_semwait_signal_nocancel = 371,
    thread_selfid = 372,
    ledger = 373,
    kevent_qos = 374,
    kevent_id = 375,
    // 376
    // 377
    // 378
    // 379
    __mac_execve = 380,
    __mac_syscall = 381,
    __mac_get_file = 382,
    __mac_set_file = 383,
    __mac_get_link = 384,
    __mac_set_link = 385,
    __mac_get_proc = 386,
    __mac_set_proc = 387,
    __mac_get_fd = 388,
    __mac_set_fd = 389,
    __mac_get_pid = 390,
    // 391
    // 392
    // 393
    pselect = 394,
    pselect_nocancel = 395,
    read_nocancel = 396,
    write_nocancel = 397,
    open_nocancel = 398,
    close_nocancel = 399,
    wait4_nocancel = 400,
    recvmsg_nocancel = 401,
    sendmsg_nocancel = 402,
    recvfrom_nocancel = 403,
    accept_nocancel = 404,
    msync_nocancel = 405,
    fcntl_nocancel = 406,
    select_nocancel = 407,
    fsync_nocancel = 408,
    connect_nocancel = 409,
    sigsuspend_nocancel = 410,
    readv_nocancel = 411,
    writev_nocancel = 412,
    sendto_nocancel = 413,
    pread_nocancel = 414,
    pwrite_nocancel = 415,
    waitid_nocancel = 416,
    poll_nocancel = 417,
    msgsnd_nocancel = 418,
    msgrcv_nocancel = 419,
    sem_wait_nocancel = 420,
    aio_suspend_nocancel = 421,
    __sigwait_nocancel = 422,
    __semwait_signal_nocancel = 423,
    __mac_mount = 424,
    __mac_get_mount = 425,
    __mac_getfsstat = 426,
    fsgetpath = 427,
    audit_session_self = 428,
    audit_session_join = 429,
    fileport_makeport = 430,
    fileport_makefd = 431,
    audit_session_port = 432,
    pid_suspend = 433,
    pid_resume = 434,
    pid_hibernate = 435,
    pid_shutdown_sockets = 436,
    // 437  old shared_region_slide_np
    shared_region_map_and_slide_np = 438,
    kas_info = 439,
    memorystatus_control = 440,
    guarded_open_np = 441,
    guarded_close_np = 442,
    guarded_kqueue_np = 443,
    change_fdguard_np = 444,
    usrctl = 445,
    proc_rlimit_control = 446,
    connectx = 447,
    disconnectx = 448,
    peeloff = 449,
    socket_delegate = 450,
    telemetry = 451,
    proc_uuid_policy = 452,
    memorystatus_get_level = 453,
    system_override = 454,
    vfs_purge = 455,
    sfi_ctl = 456,
    sfi_pidctl = 457,
    coalition = 458,
    coalition_info = 459,
    necp_match_policy = 460,
    getattrlistbulk = 461,
    clonefileat = 462,
    openat = 463,
    openat_nocancel = 464,
    renameat = 465,
    faccessat = 466,
    fchmodat = 467,
    fchownat = 468,
    fstatat = 469,
    fstatat64 = 470,
    linkat = 471,
    unlinkat = 472,
    readlinkat = 473,
    symlinkat = 474,
    mkdirat = 475,
    getattrlistat = 476,
    proc_trace_log = 477,
    bsdthread_ctl = 478,
    openbyid_np = 479,
    recvmsg_x = 480,
    sendmsg_x = 481,
    thread_selfusage = 482,
    csrctl = 483,
    guarded_open_dprotected_np = 484,
    guarded_write_np = 485,
    guarded_pwrite_np = 486,
    guarded_writev_np = 487,
    renameatx_np = 488,
    mremap_encrypted = 489,
    netagent_trigger = 490,
    stack_snapshot_with_config = 491,
    microstackshot = 492,
    grab_pgo_data = 493,
    persona = 494,
    // 495
    mach_eventlink_signal = 496,
    mach_eventlink_wait_until = 497,
    mach_eventlink_signal_wait_until = 498,
    work_interval_ctl = 499,
    getentropy = 500,
    necp_open = 501,
    necp_client_action = 502,
    __nexus_open = 503,
    __nexus_register = 504,
    __nexus_deregister = 505,
    __nexus_create = 506,
    __nexus_destroy = 507,
    __nexus_get_opt = 508,
    __nexus_set_opt = 509,
    __channel_open = 510,
    __channel_get_info = 511,
    __channel_sync = 512,
    __channel_get_opt = 513,
    __channel_set_opt = 514,
    ulock_wait = 515,
    ulock_wake = 516,
    fclonefileat = 517,
    fs_snapshot = 518,
    register_uexc_handler = 519,
    terminate_with_payload = 520,
    abort_with_payload = 521,
    necp_session_open = 522,
    necp_session_action = 523,
    setattrlistat = 524,
    net_qos_guideline = 525,
    fmount = 526,
    ntp_adjtime = 527,
    ntp_gettime = 528,
    os_fault_with_payload = 529,
    kqueue_workloop_ctl = 530,
    __mach_bridge_remote_time = 531,
    coalition_ledger = 532,
    log_data = 533,
    memorystatus_available_memory = 534,
    objc_bp_assist_cfg_np = 535,
    shared_region_map_and_slide_2_np = 536,
    pivot_root = 537,
    task_inspect_for_pid = 538,
    task_read_for_pid = 539,
    preadv = 540,
    pwritev = 541,
    preadv_nocancel = 542,
    pwritev_nocancel = 543,
    ulock_wait2 = 544,
    proc_info_extended_id = 545,
    tracker_action = 546,
    debug_syscall_reject = 547,
    // 548
    // 549
    // 550
    freadlink = 551,
    invalid = 63,
    __MAX = 552,
#else
#    error "unimplemented"
#endif
};

#ifndef ALWAYS_INLINE
#    define ALWAYS_INLINE __attribute__((always_inline)) inline
#endif

#define SYSCALL_CLOBBER_LIST "rcx", "r11", "memory"

ALWAYS_INLINE iptr syscall_impl(iptr __number)
{
    iptr retcode;
    asm volatile("syscall"
                 : "=a"(retcode)
                 : "a"(__number)
                 : SYSCALL_CLOBBER_LIST);
    return retcode;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1)
{
    iptr retcode;
    asm volatile("syscall"
                 : "=a"(retcode)
                 : "a"(__number), "D"(__arg1)
                 : SYSCALL_CLOBBER_LIST);
    return retcode;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1,
    iptr __arg2)
{
    iptr retcode;
    asm volatile("syscall"
                 : "=a"(retcode)
                 : "a"(__number), "D"(__arg1), "S"(__arg2)
                 : SYSCALL_CLOBBER_LIST);
    return retcode;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1,
    iptr __arg2, iptr __arg3)
{
    iptr retcode;
    asm volatile("syscall"
                 : "=a"(retcode)
                 : "a"(__number), "D"(__arg1), "S"(__arg2),
                 "d"(__arg3)
                 : SYSCALL_CLOBBER_LIST);
    return retcode;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1,
    iptr __arg2, iptr __arg3, iptr __arg4)
{
    iptr retcode;
    register iptr r10 __asm__("r10") = __arg4;
    asm volatile("syscall"
                 : "=a"(retcode)
                 : "a"(__number), "D"(__arg1), "S"(__arg2),
                 "d"(__arg3), "r"(r10)
                 : SYSCALL_CLOBBER_LIST);
    return retcode;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1,
    iptr __arg2, iptr __arg3, iptr __arg4, iptr __arg5)
{
    iptr retcode;
    register iptr r10 __asm__("r10") = __arg4;
    register iptr r8 __asm__("r8") = __arg5;
    asm volatile("syscall"
                 : "=a"(retcode)
                 : "a"(__number), "D"(__arg1), "S"(__arg2),
                 "d"(__arg3), "r"(r10), "r"(r8)
                 : SYSCALL_CLOBBER_LIST);
    return retcode;
}

ALWAYS_INLINE iptr syscall_impl(iptr __number, iptr __arg1,
    iptr __arg2, iptr __arg3, iptr __arg4, iptr __arg5, iptr __arg6)
{
    iptr retcode;
    register iptr r10 __asm__("r10") = __arg4;
    register iptr r8 __asm__("r8") = __arg5;
    register iptr r9 __asm__("r9") = __arg6;
    asm volatile("syscall"
                 : "=a"(retcode)
                 : "a"(__number), "D"(__arg1), "S"(__arg2),
                 "d"(__arg3), "r"(r10), "r"(r8), "r"(r9)
                 : SYSCALL_CLOBBER_LIST);
    return retcode;
}

#undef SYSCALL_CLOBBER_LIST

template <typename... Args>
iptr syscall(Syscall call, Args... args)
{
    auto number = (iptr)call;
#if __APPLE__
    number += 0x02000000; // Still incorrect. Currently assumes all
                          // syscalls are from BSD. CLASS_NONE = 0
                          // CLASS_MACH = 1
                          // CLASS_BSD  = 2
                          // CLASS_MDEP = 3
                          // CLASS_DIAG = 4
                          //
                          // number = ((CLASS_<X> << CLASS_SHIFT) |
                          // SYSCALL_NUMBER_MASK & number)
#endif
    return syscall_impl(number, (iptr)args...);
}

}
