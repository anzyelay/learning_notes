#include <string.h>
#include "canfd_collect.h"
#include "canfd_log.h"

static canfd_struct *g_canfd = NULL;

struct {
    uint16_t id;
    canfd_pkt_changed_cb f;
} pkt_subcribe_map []  = {
    { 0x490, handle_can_ivi1_id },
    { 0x495, handle_can_ivi2_id },
};

static void signal_common_handle(const signal_struct *const sig)
{
    if (!sig || !sig->user_data) {
        return;
    }

    // pthread_mutex_lock(&g_pmutex_locks[E_MUTEX_CAR_INFO]);
    switch (sig->data_type)
    {
    case SIG_DATA_TYPE_BOOL:
        *((bool *)sig->user_data) = sig->value.b;
        break;
    case SIG_DATA_TYPE_UINT8:
        *((uint8_t*)sig->user_data) = sig->value.u8;
        break;
    case SIG_DATA_TYPE_UINT16:
        *((uint16_t*)sig->user_data) = sig->value.u16;
        break;
    case SIG_DATA_TYPE_UINT32:
        *((uint32_t*)sig->user_data) = sig->value.u32;
        break;
    case SIG_DATA_TYPE_UINT64:
        *((uint64_t*)sig->user_data) = sig->value.u64;
        break;
    default:
        if (sig->value.pu8 && sig->data_len > 0) {
            memcpy((uint8_t *)sig->user_data, sig->value.pu8, sig->data_len);
        }
        break;
    }
    // pthread_mutex_unlock(&g_pmutex_locks[E_MUTEX_CAR_INFO]);
    log_info("%s = %llu\n", sig->name, sig->value.u64);
}

uint8_t wifi_sta;
struct {
    char *name;
    void *pdata;
    canfd_sig_changed_cb f;
} sig_subcribe_map []  = {
    { "Wifi_Sta_IVI_TBOX", &wifi_sta, signal_common_handle},
};

// CANFD 初始化
int canfd_init(){

    g_canfd = canfd_create();

    // 加载CANFD配置文件
    if(canfd_load_config(g_canfd, "/home/CanData.json")){
        log_err("CANFD配置文件CanData.json加载失败\n");
        return -1;
    }
    printf("111\n");
#if SUBSCRIBE_CANFD_PKT
    // 订阅需要监听的CAN包
    for (int i = 0; i < ARRAY_SIZE(pkt_subcribe_map); ++i) {
        canfd_package_subscribe(g_canfd, pkt_subcribe_map[i].id, pkt_subcribe_map[i].f);
    }
    printf("222\n");
#else
    // 处理所有CAN包
       canfd_set_handle_for_package(g_canfd, handle_can_all_id);
#endif
    printf("333\n");
    // 订阅需要监听的CAN信号量
    for (int i = 0; i < ARRAY_SIZE(sig_subcribe_map); ++i) {
        canfd_signal_subscribe(g_canfd,
                                sig_subcribe_map[i].name,
                                sig_subcribe_map[i].f,
                                sig_subcribe_map[i].pdata);
    }
    printf("444\n");
    return 0;
}

int canfd_uninit()
{
	log_info("=====%s,%d\n",__FUNCTION__, __LINE__);
    canfd_destroy(g_canfd);
    g_canfd = NULL;
    return 0;
}

canfd_struct *get_canfd(){
    return g_canfd;
}

static inline void assign_car_info(void *dest, void *src, int len)
{
    // if (len > 0 && NULL != src)
    // {
    //     pthread_mutex_lock(&g_pmutex_locks[E_MUTEX_CAR_INFO]);
    //     memcpy(dest, src, len);
    //     pthread_mutex_unlock(&g_pmutex_locks[E_MUTEX_CAR_INFO]);
    // }
}

int handle_can_ivi1_id(canfd_packet_struct *can_pkt)
{
    return 0;
}

int handle_can_ivi2_id(canfd_packet_struct *can_pkt)
{
    return 0;
}

int handle_can_all_id(canfd_packet_struct *can_pkt)
{
    for(int i = 0; i < ARRAY_SIZE(pkt_subcribe_map); i++){
        if (can_pkt->canfd_id == pkt_subcribe_map[i].id){
            pkt_subcribe_map[i].f(can_pkt);
            return 0;
        }
    }
    log_err("recv unknown canfd id %x from uart\n", can_pkt->canfd_id);
    return -1;
}

int canfd_msg_parse(canfd_msgbuff *buffer)
{
    return canfd_parse(g_canfd, buffer);
}
