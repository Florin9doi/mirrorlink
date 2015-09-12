#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>


int conn_open(char *ip, uint16_t port)
{
	struct sockaddr_in addr;
	int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (-1 == fd) {
		return -1;
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
connect_intr:
	if (-1 == connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr))) {
		if (EINPROGRESS == errno) {
			fd_set wfds;
			fd_set efds;
			struct timeval tv;
			int ret;
			FD_ZERO(&efds);
			FD_ZERO(&wfds);
			FD_SET(fd, &wfds);
			FD_SET(fd, &efds);
			tv.tv_sec = 3;
			tv.tv_usec = 0;
select_intr:
			ret = select(1, 0, &wfds, &efds, &tv);
			if (0 == ret) {
				goto fail_ret;
			} else if (-1 == ret) {
				if (EINTR == errno) {
					goto select_intr;
				} else {
					goto fail_ret;
				}
			} else {
				uint32_t i;
				for (i = 0; i < ret; i++) {
					if (FD_ISSET(fd, &efds)) {
						goto fail_ret;
					} else if (FD_ISSET(fd, &wfds)) {
						uint32_t val;
						socklen_t len = sizeof(val);
						if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &val, &len)) {
							goto fail_ret;
						} else {
							if (0 == val) {
								return fd;
							} else {
								goto fail_ret;
							}
						}
					}
				}
			}
		} else if (EINTR == errno) {
			goto connect_intr;
		}
	} else {
		return fd;
	}
fail_ret:
	close(fd);
	return -1;
}

void conn_close(int fd)
{
	close(fd);
}

int conn_read(int fd, uint8_t *buf, uint32_t len)
{
	fd_set rfds;
	fd_set efds;
	struct timeval tv;
	int ret;
	FD_ZERO(&efds);
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	FD_SET(fd, &efds);
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	while (len) {
		ret = select(1, &rfds, 0, &efds, &tv);
		if (0 == ret) {
			return 1;
		} else if (-1 == ret) {
			if (EINTR == errno) {
				continue;
			} else {
				return -1;
			}
		} else {
			uint32_t i;
			for (i = 0; i < ret; i++) {
				if (FD_ISSET(fd, &efds)) {
					return -1;
				} else if (FD_ISSET(fd, &rfds)) {
					int r;
read_intr:
					r = read(fd, buf, len);
					if (-1 == r) {
						if ((EAGAIN == errno) || (EWOULDBLOCK == errno)) {
							continue;
						} else if (EINTR == errno) {
							goto read_intr;
						}
					} else if (0 == r) {
						return 0;
					} else {
						len -= r;
					}
				}
			}
		}
	}
}

int conn_write(int fd, uint8_t *buf, uint32_t len)
{
	fd_set wfds;
	fd_set efds;
	struct timeval tv;
	int ret;
	FD_ZERO(&efds);
	FD_ZERO(&wfds);
	FD_SET(fd, &wfds);
	FD_SET(fd, &efds);
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	while (len) {
		ret = select(1, 0, &wfds, &efds, &tv);
		if (0 == ret) {
			return 1;
		} else if (-1 == ret) {
			if (EINTR == errno) {
				continue;
			} else {
				return -1;
			}
		} else {
			uint32_t i;
			for (i = 0; i < ret; i++) {
				if (FD_ISSET(fd, &efds)) {
					return -1;
				} else if (FD_ISSET(fd, &wfds)) {
					int r;
write_intr:
					r = write(fd, buf, len);
					if (r <= 0) {
						if ((EAGAIN == errno) || (EWOULDBLOCK == errno)) {
							continue;
						} else if (EINTR == errno) {
							goto write_intr;
						} else {
							return -1;
						}
					} else {
						len -= r;
					}
				}
			}
		}
	}
}
