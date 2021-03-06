#define _GNU_SOURCE		/* To get defines of NI_MAXSERV and NI_MAXHOST */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <netinet/in.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>
#ifdef linux
#include <iwlib.h>
#include <linux/if_link.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#endif

#include <include/network.h>


int hostname()
{
	char hostname[1024];
	hostname[1023] = '\0';
	gethostname(hostname, 1023);
	fprintf(stdout, "hostname: %s\n", hostname);
	return 0;
}

int get_broadcast(char *host_ip, char *netmask)
{
	struct in_addr host, mask, broadcast;
	char broadcast_address[INET_ADDRSTRLEN];

	if (inet_pton(AF_INET, host_ip, &host) == 1 && inet_pton(AF_INET, netmask, &mask) == 1)
		broadcast.s_addr = host.s_addr | ~mask.s_addr;
	else {
		fprintf(stderr, "ERROR : %s\n", strerror(errno));
		return 1;
	}

	if (inet_ntop(AF_INET, &broadcast, broadcast_address, INET_ADDRSTRLEN) != NULL)
		fprintf(stdout, "\tbroadcast: %s\n", broadcast_address);
	else {
		fprintf(stderr, "ERROR : %s\n", strerror(errno));
		return 1;
	}
	return 0;
}

/*
*	function to get mac address of a network interface
*	type is char * it returns a pointer:
*	mac
*	TODO : implement in sysnet - then call from sysnet git module
*/

char *get_mac_addr(char *interface){
	#ifdef linux
	int fd;
	struct ifreq ifr;
	char *mac;
	mac = malloc(sizeof(char) *30);
	unsigned char *mac_digit = NULL;

	memset(&ifr, 0, sizeof(ifr));
	fd = socket(AF_INET, SOCK_DGRAM, 0);

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name , interface , IFNAMSIZ-1);
	if (0 == ioctl(fd, SIOCGIFHWADDR, &ifr)) {
		mac_digit = (unsigned char *)ifr.ifr_hwaddr.sa_data;
		// if interface == "lo"; it prints -> mac : 00:00:00:00:00:00
		if (strcmp(interface, "lo") != 0)
			sprintf(mac, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X" , mac_digit[0], mac_digit[1], mac_digit[2], mac_digit[3], mac_digit[4], mac_digit[5]);
	}
	return (char *)mac;
	#else
	return NULL;
	#endif
}
int network_info(char *interface, int ipv)
{
	struct ifaddrs *ifaddr, *ifa;
	struct sockaddr *netmask;
	int family, s, n, i, suffix;
	char ip_address[NI_MAXHOST];
	char mask[NI_MAXHOST];

	char *mac_addr;
	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
		if (ifa->ifa_addr == NULL)
		continue;

		if  (interface != NULL && !strcmp(interface, "list")){
			for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
				if (ifa->ifa_addr == NULL)
					continue;
				if (ifa->ifa_addr->sa_family == AF_INET){
					printf("IPv4 %s\n", ifa->ifa_name);
				} else if (ifa->ifa_addr->sa_family == AF_INET6){
					printf("IPv6 %s\n", ifa->ifa_name);
				}else {}
			}
			break;
		}

		family = ifa->ifa_addr->sa_family;
		netmask = ifa->ifa_netmask;
		mac_addr = get_mac_addr(ifa->ifa_name);
		//printf("%s\n", ifa->ifa_name); //usefull for debug

		if (family == AF_INET && (ipv == 0 || ipv == 4)) {
			s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), ip_address, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if (s != 0) {
				fprintf(stderr, "getnameinfo() failed: %s\n", gai_strerror(s));
				exit(EXIT_FAILURE);
			}
			if (interface == NULL)
			{
				fprintf(stdout, "%s\n\taddress: %s\n", ifa->ifa_name,ip_address);
			}
		} else if (family == AF_INET6 && (ipv == 0 || ipv == 6)) {
			s = getnameinfo(ifa->ifa_addr,
			(family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), ip_address, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if (s != 0) {
				fprintf(stderr, "getnameinfo() failed: %s\n", gai_strerror(s));
				exit(EXIT_FAILURE);
			}
			if (interface != NULL)
			{
				if (!strcmp(interface, ifa->ifa_name))
				{
					fprintf(stdout, "IPv6 %s\n\taddress: %s\n", ifa->ifa_name,ip_address);
					if (strcmp(ifa->ifa_name, "lo") != 0)
					{
						fprintf(stdout, "\tmac : %s\n", mac_addr);
					}
				}
			} else {
				fprintf(stdout, "IPv6 %s\n\taddress: %s\n", ifa->ifa_name,ip_address);
				if (strcmp(ifa->ifa_name, "lo") != 0)
				{
					fprintf(stdout, "\tmac : %s\n", mac_addr);
				}
			}
		}

		if(family == AF_INET && netmask != NULL)
		{
			i = 0;
			s = getnameinfo(netmask, sizeof(struct sockaddr_in), mask, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			inet_pton(AF_INET, mask, &suffix);
			while (suffix > 0) {
				suffix = suffix >> 1;
				i++;
			}
			if (interface != NULL && !strcmp(interface, ifa->ifa_name))
			{
				fprintf(stdout, "IPv4 %s\n\taddress: %s\n", ifa->ifa_name,ip_address);
				fprintf(stdout, "\tnetmask: %s\t\tsuffix : %d\n", mask, i);
				get_broadcast(ip_address, mask);
				if (strcmp(ifa->ifa_name, "lo") != 0)
				{
					fprintf(stdout, "\tmac : %s\n", mac_addr);
				}
			}
			if (interface == NULL)
			{
				fprintf(stdout, "\tnetmask: %s\t\tsuffix : %d\n", mask, i);
				get_broadcast(ip_address, mask);
				if (strcmp(ifa->ifa_name, "lo") != 0)
				{
					fprintf(stdout, "\tmac : %s\n", mac_addr);
				}
			}
		}
	}
	freeifaddrs(ifaddr);
	return 0;
}

bool is_iface_up(const char *interface) {
	struct ifreq ifr;
	int sock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_IP);
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, interface);
	if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
		perror("SIOCGIFFLAGS");
	}
	close(sock);
	return !!(ifr.ifr_flags & IFF_UP);
}

int up_iface(const char *interface)
{
	int sockfd;
	struct ifreq ifr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sockfd < 0)
		return -1;

	memset(&ifr, 0, sizeof ifr);

	strncpy(ifr.ifr_name, interface, IFNAMSIZ);

	ifr.ifr_flags |= IFF_UP;
	ioctl(sockfd, SIOCSIFFLAGS, &ifr);
	return 0;
}


#ifdef linux
int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId)
{
	struct nlmsghdr *nlHdr;
	int readLen = 0, msgLen = 0;

	do {
		/* Recieve response from the kernel */
		if ((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0) {
			perror("SOCK READ: ");
			return -1;
		}

		nlHdr = (struct nlmsghdr *) bufPtr;

		/* Check if the header is valid */
		if ((NLMSG_OK(nlHdr, readLen) == 0) || (nlHdr->nlmsg_type == NLMSG_ERROR)) {
			perror("Error in recieved packet");
			return -1;
		}

		/* Check if the its the last message */
		if (nlHdr->nlmsg_type == NLMSG_DONE) {
			break;
		} else {
			/* Else move the pointer to buffer appropriately */
			bufPtr += readLen;
			msgLen += readLen;
		}

		/* Check if its a multi part message */
		if ((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0) {
			break;
		}
	} while ((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));
	return msgLen;
}

/* For printing the routes. */
void printRoute(struct route_info *rtInfo)
{
	char tempBuf[512];

	/* Print Destination address */
	if (rtInfo->dstAddr.s_addr != 0)
		strcpy(tempBuf,  inet_ntoa(rtInfo->dstAddr));
	else
		sprintf(tempBuf, "*.*.*.*\t");
	fprintf(stdout, "%s\t", tempBuf);

	/* Print Gateway address */
	if (rtInfo->gateWay.s_addr != 0)
		strcpy(tempBuf, (char *) inet_ntoa(rtInfo->gateWay));
	else
		sprintf(tempBuf, "*.*.*.*\t");
	fprintf(stdout, "%s\t", tempBuf);

	/* Print Interface Name*/
	fprintf(stdout, "%s\t", rtInfo->ifName);

	/* Print Source address */
	if (rtInfo->srcAddr.s_addr != 0)
		strcpy(tempBuf, inet_ntoa(rtInfo->srcAddr));
	else
		sprintf(tempBuf, "*.*.*.*\t");
	fprintf(stdout, "%s\n", tempBuf);
}

void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo)
{
	struct rtmsg *rtMsg;
	struct rtattr *rtAttr;
	int rtLen;

	rtMsg = (struct rtmsg *) NLMSG_DATA(nlHdr);


	if ((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
		return;

	rtAttr = (struct rtattr *) RTM_RTA(rtMsg);
	rtLen = RTM_PAYLOAD(nlHdr);
	for (; RTA_OK(rtAttr, rtLen); rtAttr = RTA_NEXT(rtAttr, rtLen)) {
		switch (rtAttr->rta_type) {
		case RTA_OIF:
			if_indextoname(*(int *) RTA_DATA(rtAttr), rtInfo->ifName);
			break;
		case RTA_GATEWAY:
			rtInfo->gateWay.s_addr= *(u_int *) RTA_DATA(rtAttr);
			break;
		case RTA_PREFSRC:
			rtInfo->srcAddr.s_addr= *(u_int *) RTA_DATA(rtAttr);
			break;
		case RTA_DST:
			rtInfo->dstAddr .s_addr= *(u_int *) RTA_DATA(rtAttr);
			break;
		}
	}
	//printf("%s\n", inet_ntoa(rtInfo->dstAddr));

	if (rtInfo->dstAddr.s_addr == 0)
		sprintf(gateway, "%s", (char *) inet_ntoa(rtInfo->gateWay));
	//printRoute(rtInfo);

	return;
}

int print_gateway()
{
	struct nlmsghdr *nlMsg;
	struct route_info *rtInfo;

	char msgBuf[BUFSIZE];

	int sock, len, msgSeq = 0;

	if ((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
		perror("Socket Creation: ");

	memset(msgBuf, 0, BUFSIZE);

	nlMsg = (struct nlmsghdr *) msgBuf;

	nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
	nlMsg->nlmsg_type = RTM_GETROUTE;

	nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
	nlMsg->nlmsg_seq = msgSeq++;
	nlMsg->nlmsg_pid = getpid();

	if (send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0) {
		fprintf(stderr, "Write To Socket Failed...\n");
		return -1;
	}

	/* Read the response */
	if ((len = readNlSock(sock, msgBuf, msgSeq, getpid())) < 0) {
		fprintf(stderr, "Read From Socket Failed...\n");
		return -1;
	}

	rtInfo = (struct route_info *) malloc(sizeof(struct route_info));

	for (; NLMSG_OK(nlMsg, len); nlMsg = NLMSG_NEXT(nlMsg, len)) {
		memset(rtInfo, 0, sizeof(struct route_info));
		parseRoutes(nlMsg, rtInfo);
	}
	free(rtInfo);
	close(sock);
	if (strcmp(gateway, "")!=0)
	{
		fprintf(stdout, "\nGateway : %s\n", gateway);
	}
	return 0;
}

int find_wifi(char* iw_interface){
	wireless_scan_head head;
	wireless_scan *result;
	iwrange range;
	int sock, i = 0;

	// open iw_socket
	sock = iw_sockets_open();

	// get metadata to use for scan */
	if (iw_get_range_info(sock, iw_interface, &range) < 0) {
		fprintf(stderr, "ERROR : %s\n", strerror(errno));
		return -1;
	}

	// scan
	if (iw_scan(sock, iw_interface, range.we_version_compiled, &head) < 0) {
		fprintf(stderr, "ERROR : %s\n", strerror(errno));
		return -1;
	}

	result = head.result;
	while (NULL != result) {
		printf("%s\n", result->b.essid);
		result = result->next; i++;
	}
	printf("found %d wifi networks\n", i);
	return 0;
}

int check_wireless(const char* ifname, char* protocol) {
	int sock = -1;
	struct iwreq pwrq;

	memset(&pwrq, 0, sizeof(pwrq));
	strncpy(pwrq.ifr_name, ifname, IFNAMSIZ);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return 0;
	}

	if (ioctl(sock, SIOCGIWNAME, &pwrq) != -1) {
		if (protocol) strncpy(protocol, pwrq.u.name, IFNAMSIZ);
			close(sock);
		return 1;
	}

	close(sock);
	return 0;
}


char *get_wireless_iface(void) {
	struct ifaddrs *ifaddr, *ifa;
	char *wireless_iface;
	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		return NULL;
	}

	/* walk through linked list, maintaining head pointer so we can free list later */
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		char protocol[IFNAMSIZ]  = {0};

		if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_PACKET)
			continue;

		if (check_wireless(ifa->ifa_name, protocol)) {
			wireless_iface = ifa->ifa_name;
			freeifaddrs(ifaddr);
			return (char *)wireless_iface;
		}
	}
	freeifaddrs(ifaddr);
	return NULL;
}

#endif /* linux */
