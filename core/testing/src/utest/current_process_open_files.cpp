#include <userver/utest/current_process_open_files.hpp>

#include <fmt/format.h>
#include <sys/param.h>
#include <boost/filesystem.hpp>

#if defined(__APPLE__)
#include <libproc.h>
#include <sys/proc_info.h>
#endif

USERVER_NAMESPACE_BEGIN

namespace utest {

std::vector<std::string> CurrentProcessOpenFiles() {
  std::vector<std::string> result;

  const auto pid = ::getpid();

#if defined(__APPLE__)
  // Figure out the size of the buffer needed to hold the list of open FDs
  int buffer_size = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, nullptr, 0);
  if (buffer_size == -1) {
    throw std::runtime_error("proc_pidinfo call failed");
  }

  // Get the list of open FDs
  auto proc_fd_info = std::make_unique<struct proc_fdinfo[]>(
      buffer_size / sizeof(struct proc_fdinfo));

  buffer_size =
      proc_pidinfo(pid, PROC_PIDLISTFDS, 0, proc_fd_info.get(), buffer_size);
  if (buffer_size < 0) {
    throw std::runtime_error("proc_pidinfo call with buffer failed");
  }
  int num_of_fds = buffer_size / PROC_PIDLISTFD_SIZE;

  for (int i = 0; i < num_of_fds; ++i) {
    if (proc_fd_info[i].proc_fdtype == PROX_FDTYPE_VNODE) {
      struct vnode_fdinfowithpath vnode_info {};
      int bytes_used =
          proc_pidfdinfo(pid, proc_fd_info[i].proc_fd, PROC_PIDFDVNODEPATHINFO,
                         &vnode_info, PROC_PIDFDVNODEPATHINFO_SIZE);
      if (bytes_used == PROC_PIDFDVNODEPATHINFO_SIZE) {
        result.emplace_back(vnode_info.pvip.vip_path);
      } else if (bytes_used < 0) {
        throw std::runtime_error("proc_pidinfo call for fd failed");
      }
    }
  }

#else

#ifdef BSD
  boost::filesystem::path open_files_dir = "/dev/fd";
#else
  boost::filesystem::path open_files_dir = fmt::format("/proc/{}/fd", pid);
#endif
  boost::filesystem::directory_iterator it{open_files_dir};
  boost::filesystem::directory_iterator end;
  for (; it != end; ++it) {
    boost::system::error_code err;
    const auto path = boost::filesystem::read_symlink(it->path(), err);
    if (!err) {
      result.push_back(path.string());
    }
  }

#endif
  return result;
}

}  // namespace utest

USERVER_NAMESPACE_END
