
#include "common.h"
#include "clientMain.h"
#include "proxyConfig.h"
#include "procCommServ.h"
#include "procMgr.h"
#include "netpool.h"

#include <TlHelp32.h> 
#include <io.h>
#include <mswsock.h>

#include "common_def.h"
#include "utilcommon.h"
#include "CSocksMem.h"
#include "CLocalServer.h"
#include "CRemoteServer.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "inject.h"

char g_localModDir[MAX_PATH + 1] = { 0 };
HWND g_parentWnd;

static void _remote_chk_timer_callback(void* param1, void* param2,
                void* param3, void* param4)
{
	if(!g_is_start) return;
	
	MUTEX_LOCK(m_remote_srv_lock);
    if (NULL == g_RemoteServ)
    {
    	proxy_cfg_t* cfginfo = proxy_cfg_get();

        g_RemoteServ = new CRemoteServer(cfginfo->vpn_ip, cfginfo->vpn_port);
//        g_RemoteServ->init_async_write_resource(socks_malloc, socks_free);
        if(0 != g_RemoteServ->init())
        {
        	delete g_RemoteServ;
        	g_RemoteServ = NULL;
        }
    }
    else if (g_RemoteServ->is_connected())
    {
    	if (!g_RemoteServ->is_authed())
	    {
	    	g_RemoteServ->send_auth_quest_msg();
	    }
	}
    MUTEX_UNLOCK(m_remote_srv_lock);
}

static void _session_update_timer_callback(void* param1, void* param2,
                void* param3, void* param4)
{
	/*通知主窗口更新*/
    ::PostMessage(g_parentWnd, WM_UPDATE_PROXY_IP_LIST, NULL, NULL);
}

int proxy_init()
{
	g_parentWnd = AfxGetMainWnd()->m_hWnd;

	HMODULE module = GetModuleHandle(0);    
    GetModuleFileName(module, g_localModDir, MAX_PATH);
    char *ch = _tcsrchr(g_localModDir, _T('\\')); //查找最后一个\出现的位置，并返回\后面的字符（包括\）
    ch[1] = 0;//NULL  通过操作来操作szFilePath = 将szFilePath截断，截断最后一个\后面的字符（不包括\）

	loggger_init(g_localModDir, (char*)("GoProxy"), 1 * 1024, 6, false);
	logger_set_level(L_INFO);

	if (FALSE == np_init())
	{
		MessageBox(NULL, _T("np init failed"), _T("Error"), MB_OK);

		_LOG_ERROR("netsock init failed.");
		loggger_exit();
		return -1;
	}
	int cpu_num = util_get_cpu_core_num();
	np_init_worker_thrds(cpu_num*2);
	
	/*initialize config*/
	if(0 != proxy_cfg_init())
	{
		loggger_exit();
		return -1;
	}

	if(0 != proc_comm_init())
	{
		MessageBox(NULL, _T("init proc comm server failed!"), _T("Error"), MB_OK);

		loggger_exit();
		return -1;
	}

	proxy_proc_mgr_init();

	if (g_is_start)
	{
		proxy_proc_mgr_start();
	}

	MUTEX_SETUP(m_remote_srv_lock);
    /*start check timer*/
    np_add_time_job(_remote_chk_timer_callback, NULL, NULL, NULL, NULL, 5, FALSE);
	/*start update timer*/
    np_add_time_job(_session_update_timer_callback, NULL, NULL, NULL, NULL, SESSION_UPDATE_TIME, FALSE);

    if (0 != socks_mem_init(CLIENT_MEM_NODE_CNT))
    {
    	MessageBox(NULL, _T("socks mem init failed!"), _T("Error"), MB_OK);
        loggger_exit();
        return -1;
    }

    g_ConnMgr = CConnMgr::instance();
    
    proxy_cfg_t* cfginfo = proxy_cfg_get();
    g_LocalServ = new CLocalServer(cfginfo->local_port);
    if(0 != g_LocalServ->init())
    {
    	MessageBox(NULL, _T("local server init failed!"), _T("Error"), MB_OK);
        loggger_exit();
        return -1;
    }
	
	inject_init();

	/*start check timer*/
	np_start();

	_LOG_INFO("start proxy success.");
	return 0;
}

void proxy_free()
{
	if (g_is_start)
	{
		proxy_proc_mgr_stop();
	}

	proxy_proc_mgr_free();
	np_let_stop();
	np_wait_stop();
	loggger_exit();
	return;
}
