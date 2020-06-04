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

struct tap_packet {
	struct tap_packet *next;
	size_t size;
	unsigned char data[];
};

struct tap {
	int fd;
	
	struct tap_packet *queue;
};

/* Create a TAP device */
struct tap *tap_alloc(const char *dev, uint8_t mac[6])
{
	struct ifreq ifr = { };
	int fd;
	int flags;
	const char *tundev = "/dev/net/tun";
	struct tap *tap = (struct tap *)calloc(1, sizeof(struct tap));

	if (!tap) {
		return NULL;
	}

	/* open the tun device */
	if((fd = open(tundev, O_RDWR)) < 0 ) {
		perror("TAP: Opening tun device failed");
		goto err;
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
		perror("TAP: Creating tap device failed");
		goto err_fd;
	}
	
	ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	memcpy(ifr.ifr_hwaddr.sa_data, mac, 6);

	if (ioctl(fd, SIOCSIFHWADDR, &ifr) < 0) {
		fprintf(stderr, "TAP: Setting HWADDR %02x:%02x:%02x:%02x:%02x:%02x failed\n",
		    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

		goto err_fd;
	}

	// We want a non blocking descriptor
	flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	int sock;
	
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("TAP: socket() failed");
		goto err_fd;
	}

	if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
		perror("TAP: Getting flags failed");
		close(sock);
		goto err_fd;
	}
	ifr.ifr_flags |= IFF_UP || IFF_RUNNING;
	if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
		perror("TAP:Setting flags failed");
		close(sock);
		goto err_fd;
	}

	close(sock);

	tap->fd = fd;

	return tap;
err_fd:
	close(fd);
err:
	free(tap);
	return NULL;
}

void tap_destroy(struct tap *tap)
{
	if (tap) {
		close(tap->fd);
		free(tap);
	}
}

/* transmit packet on tap device */
int tap_rx(struct tap *tap, unsigned char *packet, size_t size)
{
	ssize_t r = write(tap->fd, packet, size);
	if (r == (ssize_t)size)
		return 0;
	return -1;
}

/* check if new packets to transmit are available */
int tap_tx_check(struct tap *tap)
{
	unsigned char buffer[2048];
	ssize_t r;
	
	r = read(tap->fd, buffer, sizeof(buffer));

	if (r > 0) {
		struct tap_packet *packet = (struct tap_packet *)malloc(sizeof(tap_packet) + r);
		
		if (packet) {
			packet->size = r;
			memcpy(packet->data, buffer, packet->size);
			packet->next = NULL;
			
			struct tap_packet **q;
			for (q = &tap->queue; (*q); q = &(*q)->next);
			*q = packet;
		}
	}
	
	return (tap->queue != NULL);
}

int tap_tx_get(struct tap *tap, unsigned char *data, size_t *size)
{
	if (tap->queue) {
		struct tap_packet *packet = tap->queue;
		tap->queue = packet->next;
		
		if (*size >= packet->size) {
			memcpy(data, packet->data, packet->size);
			*size = packet->size;
		} else {
			*size = 0;
		}
		
		free(packet);
	} else {
		*size = 0;
	}

	return 0;
}

#else // not __linux__

// Defaults for OS without TAP support

struct tap *tap_alloc(const char *dev, uint8_t mac[6])
{
	return NULL;
}

void tap_destroy(struct tap *tap)
{
}

int tap_rx(struct tap *tap, unsigned char *packet, size_t size)
{
	return 0;
}

int tap_tx_check(struct tap *tap)
{
	return 0;
}

int tap_tx_get(struct tap *tap, unsigned char *data, size_t *size)
{
	*size = 0;
	return 0;
}

#endif
