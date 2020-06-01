//==========================================================================
// Name:            eth_ar_mac.h
//
// Purpose:         Encode ITU callsigns in MAC address.
// Authors:         Jeroen Vreeken
// 
// License:
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License version 2.1, as
//  published by the Free Software Foundation.  This program is
//  distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
//  License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with this program; if not, see <http://www.gnu.org/licenses/>.
//
//==========================================================================
#ifndef __ETH_AR_MAC__
#define __ETH_AR_MAC__


/**********************************************************
	Encoding an ITU callsign (and 4 bit secondary station ID to a valid MAC address.
	http://dmlinking.net/eth_ar.html
 */


// Encode a callsign and ssid into a valid MAC address
static inline int eth_ar_call2mac(uint8_t mac[6], const char *callsign, int ssid, bool multicast)
{
    // Lookup table for valid callsign characters
    static char alnum2code[37] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 
	0
    };
    
    uint64_t add = 0;
    int i;
	
    if (ssid > 15 || ssid < 0)
        return -1;
	
    for (i = 7; i >= 0; i--) {
        char c;
		
        if ((size_t)i >= strlen(callsign)) {
            c = 0;
        } else {
            c = toupper(callsign[i]);
        }
        	
        size_t j;
	
        for (j = 0; j < sizeof(alnum2code); j++) {
            if (alnum2code[j] == c)
                break;
        }
        if (j == sizeof(alnum2code))
            return -1;

        add *= 37;
        add += j;
    }
	
    mac[0] = ((add >> (40 - 6)) & 0xc0) | (ssid << 2) | 0x02 | multicast;
    mac[1] = (add >> 32) & 0xff;
    mac[2] = (add >> 24) & 0xff;
    mac[3] = (add >> 16) & 0xff;
    mac[4] = (add >> 8) & 0xff;
    mac[5] = add & 0xff;

    return 0;
}

// convenience function for splitting call and ssid
static inline int eth_ar_callssid2mac(uint8_t mac[6], const char *callsign, bool multicast)
{
	int ssid = 0;
	char call[9];
	int i;
	
	for (i = 0; i < 8; i++) {
		if (callsign[i] == '-')
			break;
		if (callsign[i] == 0)
			break;
		if (callsign[i] == ' ')
			break;
		call[i] = callsign[i];
	}
	call[i] = 0;
	
	if (callsign[i] == '-') {
		ssid = atoi(callsign + i + 1);
	}
	
	return eth_ar_call2mac(mac, call, ssid, multicast);
}

// Decode the callsign in a MAC address
static inline int eth_ar_mac2call(char *callsign, int *ssid, bool *multicast, uint8_t mac[6])
{
    // Lookup table for valid callsign characters
    static char alnum2code[37] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 
	0
    };
    uint8_t bcast[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint64_t add;
    int i;

    if (!memcmp(mac, bcast, 6)) {
        *ssid = 0;
        *multicast = true;
        strcpy(callsign, "*");
        return 0;
    }
    *multicast = mac[0] & 0x01;
    *ssid = (mac[0] & 0x3c) >> 2;
    add = (uint64_t)(mac[0] & 0xc0) << (40 - 6);
    add |= (uint64_t)mac[1] << 32;
    add |= (uint64_t)mac[2] << 24;
    add |= (uint64_t)mac[3] << 16;
    add |= (uint64_t)mac[4] << 8;
    add |= (uint64_t)mac[5];

    for (i = 0; i < 8; i++) {
        int c = add % 37;
        callsign[i] = alnum2code[c];
        add /= 37;
    }
    callsign[i] = 0;

    return 0;
}


#endif
