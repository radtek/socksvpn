#pragma once

#include "commtype.h"
#include <list>


#define PROXY_MSG_MAGIC 0xfeebbeef
#define PROXY_SOCK_MAX_FD  32768

typedef enum 
{
    SOCKS_INIT = 1,
    SOCKS_REGISTERING,
    SOCKS_PROXING,
    SOCKS_ABNORMAL
}SOCKS_STATUS_E;

typedef struct 
{
	/* data */
	uint32_t magic;
	uint32_t reserved1;
	uint64_t proc_id;
	uint32_t remote_ipaddr;
	uint16_t remote_port;
	uint16_t reserved2;
}register_req_t;

typedef struct
{
	uint32_t result;
}register_resp_t;

typedef struct 
{
	int sockfd;
	int status;
	int remote_addr;
	unsigned short remote_port;

#if 0
	uint64_t send_pkts;
	uint64_t send_bytes;
	uint64_t recv_pkts;
	uint64_t recv_bytes;
#endif
}proxy_sock_t;

int proxysocks_connect_handle(int socket, int remote_addr, int remote_port);
int proxysocks_send_handle(int socket, uint32_t bytes);
int proxysocks_close_handle(int socket);

int proxysocks_init();
void proxysocks_free();
