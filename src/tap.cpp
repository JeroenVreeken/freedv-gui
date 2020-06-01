//==========================================================================
// Name:            tap.cpp
// Purpose:         create and handle TAP device for data channel
// Date:            June 1, 2020
// Authors:         Jeroen Vreeken
// 
// License:
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 2.1 of the License, or
//  (at your option) any later version.
//
//  distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
//  License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, see <http://www.gnu.org/licenses/>.
//
//==========================================================================

#include "tap.h"

#if defined(__linux__)

#include <cstdio>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/if_tun.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>



/* Create a TAP device */
int tap_alloc(const char *dev, uint8_t mac[6])
{
	struct ifreq ifr = { };
	int fd;
	const char *tundev = "/dev/net/tun";

	/* open the tun device */
	if((fd = open(tundev, O_RDWR)) < 0 ) {
		perror("Opening tun device failed");
		return -1;
	}

	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

	if (*dev) {
		/* if a device name was specified, put it in the structure; otherwise,
		 * the kernel will try to allocate the "next" device of the
		 * specified type */
		strncpy(ifr.ifr_name, dev, IFNAMSIZ-1);
	}

	/* try to create the device */
	if(ioctl(fd, TUNSETIFF, &ifr) < 0 ) {
		printf("Creating tap device failed\n");
		close(fd);
		return -1;
	}
	
	ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	memcpy(ifr.ifr_hwaddr.sa_data, mac, 6);

	if (ioctl(fd, SIOCSIFHWADDR, &ifr) < 0) {
		printf("Setting HWADDR %02x:%02x:%02x:%02x:%02x:%02x failed\n",
		    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

		close(fd);
		return -1;
	}

	int sock;
	
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return -1;

	if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
		perror("Getting flags failed");
		close(sock);
		close(fd);
		return -1;
	}
	ifr.ifr_flags |= IFF_UP || IFF_RUNNING;
	if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
		perror("Setting flags failed");
		close(sock);
		close(fd);
		return -1;
	}

	close(sock);

	return fd;
}

void tap_destroy(int fd)
{
	close(fd);
}

int tap_rx(int tap, void *packet, size_t size)
{
	ssize_t r = write(tap, packet, size);
	if (r == (ssize_t)size)
		return 0;
	return -1;
}


#else // not __linux__

// Defaults for OS without TAP support

int tap_alloc(const char *dev, uint8_t mac[6])
{
	return -1;
}

void tap_destroy(int fd)
{
}

int tap_rx(int tap, void *packet, size_t size)
{
	return 0;
}

#endif
