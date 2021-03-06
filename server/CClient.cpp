#include "commtype.h"
#include "logproc.h"
#include "CConnection.h"
#include "CConMgr.h"
#include "common_def.h"
#include "relay_pkt_def.h"
#include "CClient.h"
#include "CRemote.h"
#include "CLocalServer.h"
#include "socks_server.h"


int CClient::send_data(char *buf, int buf_len)
{
    _LOG_ERROR("client(%s/%u/%s/%u/fd%d) shouldn't be here when send data", m_ipstr, m_port, 
        m_inner_ipstr, m_inner_port, m_fd);
    return -1;
}

int CClient::send_remote_close_msg()
{
    /*nothing to send, just close remote in free_handle*/
    PKT_HDR_T pkthdr;
    PKT_S2R_HDR_T s2rhdr;

    memset(&pkthdr, 0, sizeof(pkthdr));
    memset(&s2rhdr, 0, sizeof(s2rhdr));

    pkthdr.pkt_type = PKT_S2R;
    pkthdr.pkt_len = sizeof(PKT_S2R_HDR_T);
    PKT_HDR_HTON(&pkthdr);

    s2rhdr.client_pub_ip = m_ipaddr;
    s2rhdr.client_pub_port = m_port;
    s2rhdr.client_inner_ip = m_inner_ipaddr;
    s2rhdr.client_inner_port = m_inner_port;
    s2rhdr.sub_type = REMOTE_CLOSED;
    PKT_S2R_HDR_HTON(&s2rhdr);

    MUTEX_LOCK(m_local_srv_lock);
    if (NULL == g_LocalServ)
    {
        MUTEX_UNLOCK(m_local_srv_lock);
        _LOG_WARN("local server NULL when send client data");
        return -1;
    }
    if(0 != g_LocalServ->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
    {
        MUTEX_UNLOCK(m_local_srv_lock);
        return -1;
    }
    if(0 != g_LocalServ->send_data((char*)&s2rhdr, sizeof(PKT_S2R_HDR_T)))
    {
        MUTEX_UNLOCK(m_local_srv_lock);
        return -1;
    }
    MUTEX_UNLOCK(m_local_srv_lock);

    _LOG_INFO("client(%s/%u/%s/%u/fd%d) send remote close msg to client", m_ipstr, m_port, 
        m_inner_ipstr, m_inner_port, m_fd);
    return 0;
}

int CClient::send_connect_result(BOOL result)
{
	char resp_buf[10] = {0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int resp_buf_len = 10;
    
    if (!result)
    {
        /*set flag, which should be terminated in client, can't forward to app by client
        because this action is not standard*/
        resp_buf[1] = 0xFF;
    }

	PKT_HDR_T pkthdr;
    PKT_S2R_HDR_T s2rhdr;

    memset(&pkthdr, 0, sizeof(pkthdr));
    memset(&s2rhdr, 0, sizeof(s2rhdr));

    pkthdr.pkt_type = PKT_S2R;
    pkthdr.pkt_len = sizeof(PKT_S2R_HDR_T) + resp_buf_len;
    PKT_HDR_HTON(&pkthdr);

    s2rhdr.client_pub_ip = m_ipaddr;
    s2rhdr.client_pub_port = m_port;
    s2rhdr.client_inner_ip = m_inner_ipaddr;
    s2rhdr.client_inner_port = m_inner_port;
    s2rhdr.sub_type = S2R_CONNECT_RESULT;
    PKT_S2R_HDR_HTON(&s2rhdr);

    MUTEX_LOCK(m_local_srv_lock);
    if (NULL == g_LocalServ)
    {
        MUTEX_UNLOCK(m_local_srv_lock);
        _LOG_WARN("local server NULL when send client data");
        return -1;
    }
    if(0 != g_LocalServ->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
    {
        MUTEX_UNLOCK(m_local_srv_lock);
        return -1;
    }
    if(0 != g_LocalServ->send_data((char*)&s2rhdr, sizeof(PKT_S2R_HDR_T)))
    {
        MUTEX_UNLOCK(m_local_srv_lock);
        return -1;
    }

    if(0 != g_LocalServ->send_data(resp_buf, sizeof(resp_buf)))
    {
        MUTEX_UNLOCK(m_local_srv_lock);
        return -1;
    }
    MUTEX_UNLOCK(m_local_srv_lock);

    _LOG_INFO("client(%s/%u/%s/%u/fd%d) send connect result to client", m_ipstr, m_port, 
        m_inner_ipstr, m_inner_port, m_fd);
    return 0;
}

int CClient::send_data_msg(char *buf, int buf_len)
{
	PKT_HDR_T pkthdr;
    PKT_S2R_HDR_T s2rhdr;

    memset(&pkthdr, 0, sizeof(pkthdr));
    memset(&s2rhdr, 0, sizeof(s2rhdr));

    pkthdr.pkt_type = PKT_S2R;
    pkthdr.pkt_len = sizeof(PKT_S2R_HDR_T) + buf_len;
    PKT_HDR_HTON(&pkthdr);

    s2rhdr.client_pub_ip = m_ipaddr;
    s2rhdr.client_pub_port = m_port;
    s2rhdr.client_inner_ip = m_inner_ipaddr;
    s2rhdr.client_inner_port = m_inner_port;
    s2rhdr.sub_type = S2R_DATA;
    PKT_S2R_HDR_HTON(&s2rhdr);

    MUTEX_LOCK(m_local_srv_lock);
    if (NULL == g_LocalServ)
    {
        MUTEX_UNLOCK(m_local_srv_lock);
        _LOG_WARN("local server NULL when send client data");
        return -1;
    }
    if(0 != g_LocalServ->send_data((char*)&pkthdr, sizeof(PKT_HDR_T)))
    {
        MUTEX_UNLOCK(m_local_srv_lock);
        return -1;
    }
    if(0 != g_LocalServ->send_data((char*)&s2rhdr, sizeof(PKT_S2R_HDR_T)))
    {
        MUTEX_UNLOCK(m_local_srv_lock);
        return -1;
    }
    if(0 != g_LocalServ->send_data(buf, buf_len))
    {
        MUTEX_UNLOCK(m_local_srv_lock);
        return -1;
    }
    MUTEX_UNLOCK(m_local_srv_lock);
    return 0;
}
