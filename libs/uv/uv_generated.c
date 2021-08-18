HL_PRIM int HL_NAME(async_init_with_cb)( uv_loop_t* loop, uv_async_t* async ) {
	return uv_async_init(loop, async, on_uv_async_cb);
}
DEFINE_PRIM(_I32, async_init_with_cb, _LOOP _ASYNC);

DEFINE_PRIM(_I32, async_send, _ASYNC);

DEFINE_PRIM(_I32, check_init, _LOOP _CHECK);

HL_PRIM int HL_NAME(check_start_with_cb)( uv_check_t* check ) {
	return uv_check_start(check, on_uv_check_cb);
}
DEFINE_PRIM(_I32, check_start_with_cb, _CHECK);

DEFINE_PRIM(_I32, check_stop, _CHECK);

HL_PRIM int HL_NAME(getaddrinfo_with_cb)( uv_loop_t* loop, uv_getaddrinfo_t* req, const char* node, const char* service, const struct addrinfo* hints ) {
	return uv_getaddrinfo(loop, req, on_uv_getaddrinfo_cb, node, service, hints);
}
DEFINE_PRIM(_I32, getaddrinfo_with_cb, _LOOP _GETADDRINFO _BYTES _BYTES _ADDRINFO);

DEFINE_PRIM(_VOID, freeaddrinfo, _ADDRINFO);

HL_PRIM int HL_NAME(getnameinfo_with_cb)( uv_loop_t* loop, uv_getnameinfo_t* req, const struct sockaddr* addr, int flags ) {
	return uv_getnameinfo(loop, req, on_uv_getnameinfo_cb, addr, flags);
}
DEFINE_PRIM(_I32, getnameinfo_with_cb, _LOOP _GETNAMEINFO _SOCKADDR _I32);

DEFINE_PRIM(_BYTES, strerror, _I32);

DEFINE_PRIM(_BYTES, strerror_r, _I32 _BYTES _U64);

DEFINE_PRIM(_BYTES, err_name, _I32);

DEFINE_PRIM(_BYTES, err_name_r, _I32 _BYTES _U64);

DEFINE_PRIM(_I32, translate_sys_error, _I32);

DEFINE_PRIM(_VOID, fs_req_cleanup, _FS);

HL_PRIM int HL_NAME(fs_close_with_cb)( uv_loop_t* loop, uv_fs_t* req, uv_file file, bool use_uv_fs_cb ) {
	return uv_fs_close(loop, req, file, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_close_with_cb, _LOOP _FS _FILE _BOOL);

HL_PRIM int HL_NAME(fs_open_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, int flags, int mode, bool use_uv_fs_cb ) {
	return uv_fs_open(loop, req, path, flags, mode, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_open_with_cb, _LOOP _FS _BYTES _I32 _I32 _BOOL);

HL_PRIM int HL_NAME(fs_read_with_cb)( uv_loop_t* loop, uv_fs_t* req, uv_file file, const uv_buf_t bufs[], unsigned int nbufs, int64_t offset, bool use_uv_fs_cb ) {
	return uv_fs_read(loop, req, file, bufs, nbufs, offset, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_read_with_cb, _LOOP _FS _FILE _REF(_BUF) _U32 _I64 _BOOL);

HL_PRIM int HL_NAME(fs_unlink_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, bool use_uv_fs_cb ) {
	return uv_fs_unlink(loop, req, path, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_unlink_with_cb, _LOOP _FS _BYTES _BOOL);

HL_PRIM int HL_NAME(fs_write_with_cb)( uv_loop_t* loop, uv_fs_t* req, uv_file file, const uv_buf_t bufs[], unsigned int nbufs, int64_t offset, bool use_uv_fs_cb ) {
	return uv_fs_write(loop, req, file, bufs, nbufs, offset, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_write_with_cb, _LOOP _FS _FILE _REF(_BUF) _U32 _I64 _BOOL);

HL_PRIM int HL_NAME(fs_mkdir_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, int mode, bool use_uv_fs_cb ) {
	return uv_fs_mkdir(loop, req, path, mode, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_mkdir_with_cb, _LOOP _FS _BYTES _I32 _BOOL);

HL_PRIM int HL_NAME(fs_mkdtemp_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* tpl, bool use_uv_fs_cb ) {
	return uv_fs_mkdtemp(loop, req, tpl, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_mkdtemp_with_cb, _LOOP _FS _BYTES _BOOL);

HL_PRIM int HL_NAME(fs_mkstemp_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* tpl, bool use_uv_fs_cb ) {
	return uv_fs_mkstemp(loop, req, tpl, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_mkstemp_with_cb, _LOOP _FS _BYTES _BOOL);

HL_PRIM int HL_NAME(fs_rmdir_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, bool use_uv_fs_cb ) {
	return uv_fs_rmdir(loop, req, path, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_rmdir_with_cb, _LOOP _FS _BYTES _BOOL);

HL_PRIM int HL_NAME(fs_opendir_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, bool use_uv_fs_cb ) {
	return uv_fs_opendir(loop, req, path, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_opendir_with_cb, _LOOP _FS _BYTES _BOOL);

HL_PRIM int HL_NAME(fs_closedir_with_cb)( uv_loop_t* loop, uv_fs_t* req, uv_dir_t* dir, bool use_uv_fs_cb ) {
	return uv_fs_closedir(loop, req, dir, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_closedir_with_cb, _LOOP _FS _DIR _BOOL);

HL_PRIM int HL_NAME(fs_readdir_with_cb)( uv_loop_t* loop, uv_fs_t* req, uv_dir_t* dir, bool use_uv_fs_cb ) {
	return uv_fs_readdir(loop, req, dir, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_readdir_with_cb, _LOOP _FS _DIR _BOOL);

HL_PRIM int HL_NAME(fs_scandir_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, int flags, bool use_uv_fs_cb ) {
	return uv_fs_scandir(loop, req, path, flags, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_scandir_with_cb, _LOOP _FS _BYTES _I32 _BOOL);

DEFINE_PRIM(_I32, fs_scandir_next, _FS _DIRENT);

HL_PRIM int HL_NAME(fs_stat_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, bool use_uv_fs_cb ) {
	return uv_fs_stat(loop, req, path, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_stat_with_cb, _LOOP _FS _BYTES _BOOL);

HL_PRIM int HL_NAME(fs_fstat_with_cb)( uv_loop_t* loop, uv_fs_t* req, uv_file file, bool use_uv_fs_cb ) {
	return uv_fs_fstat(loop, req, file, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_fstat_with_cb, _LOOP _FS _FILE _BOOL);

HL_PRIM int HL_NAME(fs_lstat_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, bool use_uv_fs_cb ) {
	return uv_fs_lstat(loop, req, path, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_lstat_with_cb, _LOOP _FS _BYTES _BOOL);

HL_PRIM int HL_NAME(fs_statfs_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, bool use_uv_fs_cb ) {
	return uv_fs_statfs(loop, req, path, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_statfs_with_cb, _LOOP _FS _BYTES _BOOL);

HL_PRIM int HL_NAME(fs_rename_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, const char* new_path, bool use_uv_fs_cb ) {
	return uv_fs_rename(loop, req, path, new_path, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_rename_with_cb, _LOOP _FS _BYTES _BYTES _BOOL);

HL_PRIM int HL_NAME(fs_fsync_with_cb)( uv_loop_t* loop, uv_fs_t* req, uv_file file, bool use_uv_fs_cb ) {
	return uv_fs_fsync(loop, req, file, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_fsync_with_cb, _LOOP _FS _FILE _BOOL);

HL_PRIM int HL_NAME(fs_fdatasync_with_cb)( uv_loop_t* loop, uv_fs_t* req, uv_file file, bool use_uv_fs_cb ) {
	return uv_fs_fdatasync(loop, req, file, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_fdatasync_with_cb, _LOOP _FS _FILE _BOOL);

HL_PRIM int HL_NAME(fs_ftruncate_with_cb)( uv_loop_t* loop, uv_fs_t* req, uv_file file, int64_t offset, bool use_uv_fs_cb ) {
	return uv_fs_ftruncate(loop, req, file, offset, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_ftruncate_with_cb, _LOOP _FS _FILE _I64 _BOOL);

HL_PRIM int HL_NAME(fs_copyfile_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, const char* new_path, int flags, bool use_uv_fs_cb ) {
	return uv_fs_copyfile(loop, req, path, new_path, flags, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_copyfile_with_cb, _LOOP _FS _BYTES _BYTES _I32 _BOOL);

HL_PRIM int HL_NAME(fs_sendfile_with_cb)( uv_loop_t* loop, uv_fs_t* req, uv_file out_fd, uv_file in_fd, int64_t in_offset, size_t length, bool use_uv_fs_cb ) {
	return uv_fs_sendfile(loop, req, out_fd, in_fd, in_offset, length, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_sendfile_with_cb, _LOOP _FS _FILE _FILE _I64 _U64 _BOOL);

HL_PRIM int HL_NAME(fs_access_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, int mode, bool use_uv_fs_cb ) {
	return uv_fs_access(loop, req, path, mode, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_access_with_cb, _LOOP _FS _BYTES _I32 _BOOL);

HL_PRIM int HL_NAME(fs_chmod_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, int mode, bool use_uv_fs_cb ) {
	return uv_fs_chmod(loop, req, path, mode, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_chmod_with_cb, _LOOP _FS _BYTES _I32 _BOOL);

HL_PRIM int HL_NAME(fs_fchmod_with_cb)( uv_loop_t* loop, uv_fs_t* req, uv_file file, int mode, bool use_uv_fs_cb ) {
	return uv_fs_fchmod(loop, req, file, mode, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_fchmod_with_cb, _LOOP _FS _FILE _I32 _BOOL);

HL_PRIM int HL_NAME(fs_utime_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, double atime, double mtime, bool use_uv_fs_cb ) {
	return uv_fs_utime(loop, req, path, atime, mtime, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_utime_with_cb, _LOOP _FS _BYTES _F64 _F64 _BOOL);

HL_PRIM int HL_NAME(fs_futime_with_cb)( uv_loop_t* loop, uv_fs_t* req, uv_file file, double atime, double mtime, bool use_uv_fs_cb ) {
	return uv_fs_futime(loop, req, file, atime, mtime, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_futime_with_cb, _LOOP _FS _FILE _F64 _F64 _BOOL);

HL_PRIM int HL_NAME(fs_lutime_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, double atime, double mtime, bool use_uv_fs_cb ) {
	return uv_fs_lutime(loop, req, path, atime, mtime, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_lutime_with_cb, _LOOP _FS _BYTES _F64 _F64 _BOOL);

HL_PRIM int HL_NAME(fs_link_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, const char* new_path, bool use_uv_fs_cb ) {
	return uv_fs_link(loop, req, path, new_path, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_link_with_cb, _LOOP _FS _BYTES _BYTES _BOOL);

HL_PRIM int HL_NAME(fs_symlink_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, const char* new_path, int flags, bool use_uv_fs_cb ) {
	return uv_fs_symlink(loop, req, path, new_path, flags, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_symlink_with_cb, _LOOP _FS _BYTES _BYTES _I32 _BOOL);

HL_PRIM int HL_NAME(fs_readlink_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, bool use_uv_fs_cb ) {
	return uv_fs_readlink(loop, req, path, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_readlink_with_cb, _LOOP _FS _BYTES _BOOL);

HL_PRIM int HL_NAME(fs_realpath_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, bool use_uv_fs_cb ) {
	return uv_fs_realpath(loop, req, path, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_realpath_with_cb, _LOOP _FS _BYTES _BOOL);

HL_PRIM int HL_NAME(fs_chown_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, uv_uid_t uid, uv_gid_t gid, bool use_uv_fs_cb ) {
	return uv_fs_chown(loop, req, path, uid, gid, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_chown_with_cb, _LOOP _FS _BYTES _UID_T _GID_T _BOOL);

HL_PRIM int HL_NAME(fs_fchown_with_cb)( uv_loop_t* loop, uv_fs_t* req, uv_file file, uv_uid_t uid, uv_gid_t gid, bool use_uv_fs_cb ) {
	return uv_fs_fchown(loop, req, file, uid, gid, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_fchown_with_cb, _LOOP _FS _FILE _UID_T _GID_T _BOOL);

HL_PRIM int HL_NAME(fs_lchown_with_cb)( uv_loop_t* loop, uv_fs_t* req, const char* path, uv_uid_t uid, uv_gid_t gid, bool use_uv_fs_cb ) {
	return uv_fs_lchown(loop, req, path, uid, gid, use_uv_fs_cb?on_uv_fs_cb:NULL);
}
DEFINE_PRIM(_I32, fs_lchown_with_cb, _LOOP _FS _BYTES _UID_T _GID_T _BOOL);

DEFINE_PRIM(_FS_TYPE, fs_get_type, _FS);

DEFINE_PRIM(_I64, fs_get_result, _FS);

DEFINE_PRIM(_I32, fs_get_system_error, _FS);

DEFINE_PRIM(_POINTER, fs_get_ptr, _FS);

DEFINE_PRIM(_BYTES, fs_get_path, _FS);

DEFINE_PRIM(_STAT, fs_get_statbuf, _FS);

DEFINE_PRIM(_OS_FD_T, get_osfhandle, _I32);

DEFINE_PRIM(_I32, open_osfhandle, _OS_FD_T);

DEFINE_PRIM(_I32, fs_event_init, _LOOP _FS_EVENT);

HL_PRIM int HL_NAME(fs_event_start_with_cb)( uv_fs_event_t* handle, const char* path, unsigned int flags ) {
	return uv_fs_event_start(handle, on_uv_fs_event_cb, path, flags);
}
DEFINE_PRIM(_I32, fs_event_start_with_cb, _FS_EVENT _BYTES _U32);

DEFINE_PRIM(_I32, fs_event_stop, _FS_EVENT);

DEFINE_PRIM(_I32, fs_event_getpath, _FS_EVENT _BYTES _REF(_I64));

DEFINE_PRIM(_I32, fs_poll_init, _LOOP _FS_POLL);

HL_PRIM int HL_NAME(fs_poll_start_with_cb)( uv_fs_poll_t* handle, const char* path, unsigned int interval ) {
	return uv_fs_poll_start(handle, on_uv_fs_poll_cb, path, interval);
}
DEFINE_PRIM(_I32, fs_poll_start_with_cb, _FS_POLL _BYTES _U32);

DEFINE_PRIM(_I32, fs_poll_stop, _FS_POLL);

DEFINE_PRIM(_I32, fs_poll_getpath, _FS_POLL _BYTES _REF(_I64));

DEFINE_PRIM(_I32, is_active, _HANDLE);

DEFINE_PRIM(_I32, is_closing, _HANDLE);

HL_PRIM void HL_NAME(close_with_cb)( uv_handle_t* handle ) {
	uv_close(handle, on_uv_close_cb);
}
DEFINE_PRIM(_VOID, close_with_cb, _HANDLE);

DEFINE_PRIM(_VOID, ref, _HANDLE);

DEFINE_PRIM(_VOID, unref, _HANDLE);

DEFINE_PRIM(_I32, has_ref, _HANDLE);

DEFINE_PRIM(_U64, handle_size, _HANDLE_TYPE);

DEFINE_PRIM(_I32, send_buffer_size, _HANDLE _REF(_I32));

DEFINE_PRIM(_I32, recv_buffer_size, _HANDLE _REF(_I32));

DEFINE_PRIM(_I32, fileno, _HANDLE _OS_FD);

DEFINE_PRIM(_LOOP, handle_get_loop, _HANDLE);

DEFINE_PRIM(_POINTER, handle_get_data, _HANDLE);

DEFINE_PRIM(_POINTER, handle_set_data, _HANDLE _POINTER);

DEFINE_PRIM(_HANDLE_TYPE, handle_get_type, _HANDLE);

DEFINE_PRIM(_BYTES, handle_type_name, _HANDLE_TYPE);

DEFINE_PRIM(_I32, idle_init, _LOOP _IDLE);

HL_PRIM int HL_NAME(idle_start_with_cb)( uv_idle_t* idle ) {
	return uv_idle_start(idle, on_uv_idle_cb);
}
DEFINE_PRIM(_I32, idle_start_with_cb, _IDLE);

DEFINE_PRIM(_I32, idle_stop, _IDLE);

DEFINE_PRIM(_I32, loop_init, _LOOP);

DEFINE_PRIM(_I32, loop_close, _LOOP);

DEFINE_PRIM(_LOOP, default_loop, _NO_ARG);

DEFINE_PRIM(_I32, run, _LOOP _RUN_MODE);

DEFINE_PRIM(_I32, loop_alive, _LOOP);

DEFINE_PRIM(_VOID, stop, _LOOP);

DEFINE_PRIM(_U64, loop_size, _NO_ARG);

DEFINE_PRIM(_I32, backend_fd, _LOOP);

DEFINE_PRIM(_I32, backend_timeout, _LOOP);

DEFINE_PRIM(_I64, now, _LOOP);

DEFINE_PRIM(_VOID, update_time, _LOOP);

HL_PRIM void HL_NAME(walk_with_cb)( uv_loop_t* loop, void* arg ) {
	uv_walk(loop, on_uv_walk_cb, arg);
}
DEFINE_PRIM(_VOID, walk_with_cb, _LOOP _POINTER);

DEFINE_PRIM(_I32, loop_fork, _LOOP);

DEFINE_PRIM(_POINTER, loop_get_data, _LOOP);

DEFINE_PRIM(_POINTER, loop_set_data, _LOOP _POINTER);

DEFINE_PRIM(_I64, metrics_idle_time, _LOOP);

DEFINE_PRIM(_HANDLE_TYPE, guess_handle, _FILE);

DEFINE_PRIM(_I32, replace_allocator, _MALLOC_FUNC _REALLOC_FUNC _CALLOC_FUNC _FREE_FUNC);

DEFINE_PRIM(_VOID, library_shutdown, _NO_ARG);

DEFINE_PRIM(_BUF, buf_init, _BYTES _U32);

DEFINE_PRIM(_REF(_BYTES), setup_args, _I32 _REF(_BYTES));

DEFINE_PRIM(_I32, get_process_title, _BYTES _U64);

DEFINE_PRIM(_I32, set_process_title, _BYTES);

DEFINE_PRIM(_I32, resident_set_memory, _REF(_I64));

DEFINE_PRIM(_I32, uptime, _REF(_F64));

DEFINE_PRIM(_I32, getrusage, _RUSAGE);

DEFINE_PRIM(_I32, os_getpid, _NO_ARG);

DEFINE_PRIM(_I32, os_getppid, _NO_ARG);

DEFINE_PRIM(_I32, cpu_info, _REF(_CPU_INFO) _REF(_I32));

DEFINE_PRIM(_VOID, free_cpu_info, _CPU_INFO _I32);

DEFINE_PRIM(_I32, interface_addresses, _REF(_INTERFACE_ADDRESS) _REF(_I32));

DEFINE_PRIM(_VOID, free_interface_addresses, _INTERFACE_ADDRESS _I32);

DEFINE_PRIM(_VOID, loadavg, _REF(_F64));

DEFINE_PRIM(_I32, ip4_addr, _BYTES _I32 _SOCKADDR_IN);

DEFINE_PRIM(_I32, ip6_addr, _BYTES _I32 _SOCKADDR_IN6);

DEFINE_PRIM(_I32, ip4_name, _SOCKADDR_IN _BYTES _U64);

DEFINE_PRIM(_I32, ip6_name, _SOCKADDR_IN6 _BYTES _U64);

DEFINE_PRIM(_I32, inet_ntop, _I32 _POINTER _BYTES _U64);

DEFINE_PRIM(_I32, inet_pton, _I32 _BYTES _POINTER);

DEFINE_PRIM(_I32, if_indextoname, _U32 _BYTES _REF(_I64));

DEFINE_PRIM(_I32, if_indextoiid, _U32 _BYTES _REF(_I64));

DEFINE_PRIM(_I32, exepath, _BYTES _REF(_I64));

DEFINE_PRIM(_I32, cwd, _BYTES _REF(_I64));

DEFINE_PRIM(_I32, chdir, _BYTES);

DEFINE_PRIM(_I32, os_homedir, _BYTES _REF(_I64));

DEFINE_PRIM(_I32, os_tmpdir, _BYTES _REF(_I64));

DEFINE_PRIM(_I32, os_get_passwd, _PASSWD);

DEFINE_PRIM(_VOID, os_free_passwd, _PASSWD);

DEFINE_PRIM(_I64, get_free_memory, _NO_ARG);

DEFINE_PRIM(_I64, get_total_memory, _NO_ARG);

DEFINE_PRIM(_I64, get_constrained_memory, _NO_ARG);

DEFINE_PRIM(_I64, hrtime, _NO_ARG);

DEFINE_PRIM(_VOID, print_all_handles, _LOOP _FILE);

DEFINE_PRIM(_VOID, print_active_handles, _LOOP _FILE);

DEFINE_PRIM(_I32, os_environ, _REF(_ENV_ITEM) _REF(_I32));

DEFINE_PRIM(_VOID, os_free_environ, _ENV_ITEM _I32);

DEFINE_PRIM(_I32, os_getenv, _BYTES _BYTES _REF(_I64));

DEFINE_PRIM(_I32, os_setenv, _BYTES _BYTES);

DEFINE_PRIM(_I32, os_unsetenv, _BYTES);

DEFINE_PRIM(_I32, os_gethostname, _BYTES _REF(_I64));

DEFINE_PRIM(_I32, os_getpriority, _I32 _REF(_I32));

DEFINE_PRIM(_I32, os_setpriority, _I32 _I32);

DEFINE_PRIM(_I32, os_uname, _UTSNAME);

DEFINE_PRIM(_I32, gettimeofday, _TIMEVAL64);

HL_PRIM int HL_NAME(random_with_cb)( uv_loop_t* loop, uv_random_t* req, void* buf, size_t buflen, unsigned int flags ) {
	return uv_random(loop, req, buf, buflen, flags, on_uv_random_cb);
}
DEFINE_PRIM(_I32, random_with_cb, _LOOP _RANDOM _POINTER _U64 _U32);

DEFINE_PRIM(_VOID, sleep, _U32);

DEFINE_PRIM(_I32, pipe_init, _LOOP _PIPE _I32);

DEFINE_PRIM(_I32, pipe_open, _PIPE _FILE);

DEFINE_PRIM(_I32, pipe_bind, _PIPE _BYTES);

HL_PRIM void HL_NAME(pipe_connect_with_cb)( uv_connect_t* req, uv_pipe_t* handle, const char* name ) {
	uv_pipe_connect(req, handle, name, on_uv_connect_cb);
}
DEFINE_PRIM(_VOID, pipe_connect_with_cb, _CONNECT _PIPE _BYTES);

DEFINE_PRIM(_I32, pipe_getsockname, _PIPE _BYTES _REF(_I64));

DEFINE_PRIM(_I32, pipe_getpeername, _PIPE _BYTES _REF(_I64));

DEFINE_PRIM(_VOID, pipe_pending_instances, _PIPE _I32);

DEFINE_PRIM(_I32, pipe_pending_count, _PIPE);

DEFINE_PRIM(_HANDLE_TYPE, pipe_pending_type, _PIPE);

DEFINE_PRIM(_I32, pipe_chmod, _PIPE _I32);

DEFINE_PRIM(_I32, pipe, _REF(_FILE) _I32 _I32);

DEFINE_PRIM(_I32, prepare_init, _LOOP _PREPARE);

HL_PRIM int HL_NAME(prepare_start_with_cb)( uv_prepare_t* prepare ) {
	return uv_prepare_start(prepare, on_uv_prepare_cb);
}
DEFINE_PRIM(_I32, prepare_start_with_cb, _PREPARE);

DEFINE_PRIM(_I32, prepare_stop, _PREPARE);

DEFINE_PRIM(_VOID, disable_stdio_inheritance, _NO_ARG);

DEFINE_PRIM(_I32, spawn, _LOOP _PROCESS _PROCESS_OPTIONS);

DEFINE_PRIM(_I32, process_kill, _PROCESS _I32);

DEFINE_PRIM(_I32, kill, _I32 _I32);

DEFINE_PRIM(_I32, process_get_pid, _PROCESS);

DEFINE_PRIM(_I32, cancel, _REQ);

DEFINE_PRIM(_U64, req_size, _REQ_TYPE);

DEFINE_PRIM(_POINTER, req_get_data, _REQ);

DEFINE_PRIM(_POINTER, req_set_data, _REQ _POINTER);

DEFINE_PRIM(_REQ_TYPE, req_get_type, _REQ);

DEFINE_PRIM(_BYTES, req_type_name, _REQ_TYPE);

DEFINE_PRIM(_I32, signal_init, _LOOP _SIGNAL);

HL_PRIM int HL_NAME(signal_start_with_cb)( uv_signal_t* signal, int signum ) {
	return uv_signal_start(signal, on_uv_signal_cb, signum);
}
DEFINE_PRIM(_I32, signal_start_with_cb, _SIGNAL _I32);

HL_PRIM int HL_NAME(signal_start_oneshot_with_cb)( uv_signal_t* signal, int signum ) {
	return uv_signal_start_oneshot(signal, on_uv_signal_cb, signum);
}
DEFINE_PRIM(_I32, signal_start_oneshot_with_cb, _SIGNAL _I32);

DEFINE_PRIM(_I32, signal_stop, _SIGNAL);

HL_PRIM int HL_NAME(shutdown_with_cb)( uv_shutdown_t* req, uv_stream_t* handle ) {
	return uv_shutdown(req, handle, on_uv_shutdown_cb);
}
DEFINE_PRIM(_I32, shutdown_with_cb, _SHUTDOWN _STREAM);

HL_PRIM int HL_NAME(listen_with_cb)( uv_stream_t* stream, int backlog ) {
	return uv_listen(stream, backlog, on_uv_connection_cb);
}
DEFINE_PRIM(_I32, listen_with_cb, _STREAM _I32);

DEFINE_PRIM(_I32, accept, _STREAM _STREAM);

HL_PRIM int HL_NAME(read_start_with_cb)( uv_stream_t* stream ) {
	return uv_read_start(stream, on_uv_alloc_cb, on_uv_read_cb);
}
DEFINE_PRIM(_I32, read_start_with_cb, _STREAM);

DEFINE_PRIM(_I32, read_stop, _STREAM);

HL_PRIM int HL_NAME(write_with_cb)( uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs ) {
	return uv_write(req, handle, bufs, nbufs, on_uv_write_cb);
}
DEFINE_PRIM(_I32, write_with_cb, _WRITE _STREAM _REF(_BUF) _U32);

HL_PRIM int HL_NAME(write2_with_cb)( uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs, uv_stream_t* send_handle ) {
	return uv_write2(req, handle, bufs, nbufs, send_handle, on_uv_write_cb);
}
DEFINE_PRIM(_I32, write2_with_cb, _WRITE _STREAM _REF(_BUF) _U32 _STREAM);

DEFINE_PRIM(_I32, try_write, _STREAM _REF(_BUF) _U32);

DEFINE_PRIM(_I32, try_write2, _STREAM _REF(_BUF) _U32 _STREAM);

DEFINE_PRIM(_I32, is_readable, _STREAM);

DEFINE_PRIM(_I32, is_writable, _STREAM);

DEFINE_PRIM(_I32, stream_set_blocking, _STREAM _I32);

DEFINE_PRIM(_U64, stream_get_write_queue_size, _STREAM);

DEFINE_PRIM(_I32, tcp_init, _LOOP _TCP);

DEFINE_PRIM(_I32, tcp_init_ex, _LOOP _TCP _U32);

DEFINE_PRIM(_I32, tcp_open, _TCP _OS_SOCK_T);

DEFINE_PRIM(_I32, tcp_nodelay, _TCP _I32);

DEFINE_PRIM(_I32, tcp_keepalive, _TCP _I32 _U32);

DEFINE_PRIM(_I32, tcp_simultaneous_accepts, _TCP _I32);

DEFINE_PRIM(_I32, tcp_bind, _TCP _SOCKADDR _U32);

DEFINE_PRIM(_I32, tcp_getsockname, _TCP _SOCKADDR _REF(_I32));

DEFINE_PRIM(_I32, tcp_getpeername, _TCP _SOCKADDR _REF(_I32));

HL_PRIM int HL_NAME(tcp_connect_with_cb)( uv_connect_t* req, uv_tcp_t* handle, const struct sockaddr* addr ) {
	return uv_tcp_connect(req, handle, addr, on_uv_connect_cb);
}
DEFINE_PRIM(_I32, tcp_connect_with_cb, _CONNECT _TCP _SOCKADDR);

HL_PRIM int HL_NAME(tcp_close_reset_with_cb)( uv_tcp_t* handle ) {
	return uv_tcp_close_reset(handle, on_uv_close_cb);
}
DEFINE_PRIM(_I32, tcp_close_reset_with_cb, _TCP);

DEFINE_PRIM(_I32, socketpair, _I32 _I32 _REF(_OS_SOCK_T) _I32 _I32);

DEFINE_PRIM(_I32, timer_init, _LOOP _TIMER);

HL_PRIM int HL_NAME(timer_start_with_cb)( uv_timer_t* handle, uint64_t timeout, uint64_t repeat ) {
	return uv_timer_start(handle, on_uv_timer_cb, timeout, repeat);
}
DEFINE_PRIM(_I32, timer_start_with_cb, _TIMER _I64 _I64);

DEFINE_PRIM(_I32, timer_stop, _TIMER);

DEFINE_PRIM(_I32, timer_again, _TIMER);

DEFINE_PRIM(_VOID, timer_set_repeat, _TIMER _I64);

DEFINE_PRIM(_I64, timer_get_repeat, _TIMER);

DEFINE_PRIM(_I64, timer_get_due_in, _TIMER);

DEFINE_PRIM(_I32, tty_init, _LOOP _TTY _FILE _I32);

DEFINE_PRIM(_I32, tty_set_mode, _TTY _TTY_MODE_T);

DEFINE_PRIM(_I32, tty_reset_mode, _NO_ARG);

DEFINE_PRIM(_I32, tty_get_winsize, _TTY _REF(_I32) _REF(_I32));

DEFINE_PRIM(_VOID, tty_set_vterm_state, _TTY_VTERMSTATE_T);

DEFINE_PRIM(_I32, tty_get_vterm_state, _TTY_VTERMSTATE);

DEFINE_PRIM(_I32, udp_init, _LOOP _UDP);

DEFINE_PRIM(_I32, udp_init_ex, _LOOP _UDP _U32);

DEFINE_PRIM(_I32, udp_open, _UDP _OS_SOCK_T);

DEFINE_PRIM(_I32, udp_bind, _UDP _SOCKADDR _U32);

DEFINE_PRIM(_I32, udp_connect, _UDP _SOCKADDR);

DEFINE_PRIM(_I32, udp_getpeername, _UDP _SOCKADDR _REF(_I32));

DEFINE_PRIM(_I32, udp_getsockname, _UDP _SOCKADDR _REF(_I32));

DEFINE_PRIM(_I32, udp_set_membership, _UDP _BYTES _BYTES _MEMBERSHIP);

DEFINE_PRIM(_I32, udp_set_source_membership, _UDP _BYTES _BYTES _BYTES _MEMBERSHIP);

DEFINE_PRIM(_I32, udp_set_multicast_loop, _UDP _I32);

DEFINE_PRIM(_I32, udp_set_multicast_ttl, _UDP _I32);

DEFINE_PRIM(_I32, udp_set_multicast_interface, _UDP _BYTES);

DEFINE_PRIM(_I32, udp_set_broadcast, _UDP _I32);

DEFINE_PRIM(_I32, udp_set_ttl, _UDP _I32);

HL_PRIM int HL_NAME(udp_send_with_cb)( uv_udp_send_t* req, uv_udp_t* handle, const uv_buf_t bufs[], unsigned int nbufs, const struct sockaddr* addr ) {
	return uv_udp_send(req, handle, bufs, nbufs, addr, on_uv_udp_send_cb);
}
DEFINE_PRIM(_I32, udp_send_with_cb, _UDP_SEND _UDP _REF(_BUF) _U32 _SOCKADDR);

DEFINE_PRIM(_I32, udp_try_send, _UDP _REF(_BUF) _U32 _SOCKADDR);

HL_PRIM int HL_NAME(udp_recv_start_with_cb)( uv_udp_t* handle ) {
	return uv_udp_recv_start(handle, on_uv_alloc_cb, on_uv_udp_recv_cb);
}
DEFINE_PRIM(_I32, udp_recv_start_with_cb, _UDP);

DEFINE_PRIM(_I32, udp_using_recvmmsg, _UDP);

DEFINE_PRIM(_I32, udp_recv_stop, _UDP);

DEFINE_PRIM(_U64, udp_get_send_queue_size, _UDP);

DEFINE_PRIM(_U64, udp_get_send_queue_count, _UDP);

DEFINE_PRIM(_U32, version, _NO_ARG);

DEFINE_PRIM(_BYTES, version_string, _NO_ARG);

