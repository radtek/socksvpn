#pragma once

#include <list>

#define PROC_NAME_LEN  64
#define PROC_UPDATE_TIME  1

enum
{
	PROC_ST_INIT = 0,
	PROC_ST_INJECTED, /*已注入*/
	PROC_ST_REGISTED, /*已经获取配置*/
	PROC_ST_UNINJECTED, /*取消注入*/

	PROC_ST_MAX
};

typedef struct 
{
	char proc_name[PROC_NAME_LEN + 1];
	uint64_t proc_id;
	bool is_proc_64;
	uint64_t start_time;

	int status;
	uint64_t update_time;
	bool is_injected;
}proc_proxy_t;

typedef std::list<proc_proxy_t*> PROC_LIST;
typedef PROC_LIST::iterator PROC_LIST_Itr;
typedef PROC_LIST::reverse_iterator PROC_LIST_RItr; /*反向遍历迭代器*/

int proxy_proc_add(char *proc_name, uint64_t proc_id);
void proxy_proc_del(uint64_t proc_id);
void proxy_proc_set_registered(uint64_t proc_id);
void proxy_proc_set_uninjected(uint64_t proc_id);
void proxy_proc_update_all();
void proxy_proc_del_all();

int proxy_proc_get_injected_cnt();

int proxy_proc_mgr_init();
void proxy_proc_mgr_free();

int  proxy_proc_mgr_start();
void  proxy_proc_mgr_stop();

extern MUTEX_TYPE g_prox_proc_lock;
extern PROC_LIST g_prox_proc_objs;
extern const char* g_proxy_proc_status_desc[];

