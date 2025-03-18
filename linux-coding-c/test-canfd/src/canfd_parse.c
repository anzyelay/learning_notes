#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include "canfd_parse.h"
#include "cjson_comptitable.h"
#include "avl_tree.h"
#include "can_tool.h"
#include "include/canfd_log.h"

typedef struct __real_canfd_struct {
	int num; // Canfd 报文的数量
	avl_tree_t *canfd_tree;
	canfd_pkt_changed_cb all_package_changed_handle;
	canfd_sig_changed_cb all_signal_changed_handle;
} real_canfd_struct;

static inline real_canfd_struct *static_cast_real(canfd_struct *canfd)
{
    return (real_canfd_struct *)(canfd);
}

static int canfd_cmp_fun(struct avl_node *n1, struct avl_node *n2, void* aux)
{
    int id1 = node_entry(n1, canfd_packet_struct, node)->canfd_id;
    int id2 = node_entry(n2, canfd_packet_struct, node)->canfd_id;
    return id2 - id1;
}

canfd_struct *canfd_create()
{
    real_canfd_struct *canfd = (real_canfd_struct *)malloc(sizeof(real_canfd_struct));
    canfd->all_package_changed_handle = NULL;
    canfd->all_signal_changed_handle = NULL;
    canfd->canfd_tree = avl_tree_new(canfd_cmp_fun);
    return (canfd_struct *)canfd;
}

sigvariant_un search_sigval(canfd_packet_struct *canfd_pkt, const char *name)
{
    return canfd_search_sig_from_pkt(canfd_pkt, name)->value;
}

// 订阅信号量变化
bool canfd_signal_subscribe(canfd_struct *canfd, const char *signame,
                            canfd_sig_changed_cb fun, void *user_data)
{
    signal_struct *sig = canfd_search_sig_from_name(canfd, signame);
    return canfd_signal_set_cbfun(sig, fun, user_data);
}

// 订阅报文变化
bool canfd_package_subscribe(canfd_struct *canfd, uint32_t id, canfd_pkt_changed_cb fun)
{
    canfd_packet_struct *pkt = canfd_search_pkt_from_id(canfd, id);
    if (!pkt)
        return false;
    pkt->value_changed_cb = fun;
    return true;
}

// 针对所有CAN FD数据包变化的回调函数
void canfd_set_handle_for_package(canfd_struct *canfd, canfd_pkt_changed_cb cb)
{
    real_canfd_struct *rl_canfd = static_cast_real(canfd);
    rl_canfd->all_package_changed_handle = cb;
}

// 针对所有CAN FD信号量变化的回调函数
void canfd_set_handle_for_signal(canfd_struct *canfd, canfd_sig_changed_cb cb)
{
    real_canfd_struct *rl_canfd = static_cast_real(canfd);
    rl_canfd->all_signal_changed_handle = cb;
}

// 查找canid对应的can报文数据
canfd_packet_struct *canfd_search_pkt_from_id(canfd_struct *canfd, uint32_t canfd_id)
{
    canfd_packet_struct search;
    search.canfd_id = canfd_id;
    avl_node_t *n = avl_search(canfd->canfd_tree, &search.node);
    if (n != NULL) {
        return node_entry(n, canfd_packet_struct, node);
    }
    log_err("can't get canfd packet by canfd id %x\n", canfd_id);
    return NULL;
}

canfd_packet_struct *canfd_search_dup2_from_id(canfd_struct *canfd, uint32_t canfd_id, canfd_packet_struct *pout)
{
    canfd_packet_struct *ppkt = canfd_search_pkt_from_id(canfd, canfd_id);
    if (!ppkt) {
        return NULL;
    }
    return canfd_packet_dup2(ppkt, pout);
}

// 查找报文name对应的can报文数据
canfd_packet_struct *canfd_search_pkt_from_name(canfd_struct *canfd, const char *name)
{
    canfd_packet_struct *pkt = NULL;
    list_for_each_entry(pkt, static_cast_real(canfd)->canfd_tree, node) {
        if (0 == strcmp(name, pkt->name)) {
            return pkt;
        }
    }
    log_err("can't get canfd packet by canfd name %s\n", name);
    return NULL;
}

// 查找信号量name对应的信号量数据
signal_struct *canfd_search_sig_from_name(canfd_struct *canfd, const char *name)
{
    canfd_packet_struct *pkt = NULL;
    list_for_each_entry(pkt, static_cast_real(canfd)->canfd_tree, node) {
        for (int j = 0; j < pkt->signal_num; j++) {
            if (0 == strcmp(name, pkt->signal[j].name))
                return &pkt->signal[j];
        }
    }
    log_err("can't get signal by signal name %s\n", name);
    return NULL;
}

signal_struct *canfd_search_sig_from_pkt(canfd_packet_struct *canfd_pkt , const char *name)
{
    for (int j = 0; j < canfd_pkt->signal_num; j++){
        if (0 == strcmp(name, canfd_pkt->signal[j].name))
            return &canfd_pkt->signal[j];
    }
    log_err("can't get signal by signal name %s in canfd pkt\n", name);
    return NULL;
}

static int update_pkt(real_canfd_struct *canfd, canfd_packet_struct *pkt, canfd_msgbuff *can)
{
    int changed = 0;

    if(pkt->dlc < can->canfd_len ) {
        log_err("packet data len %dByte is too short for %dByte can msg \n",
        pkt->dlc, can->canfd_len);
        return false;
    }

    if (0 != memcmp(pkt->data, can->data, can->canfd_len)) {
        changed = 1;
        memcpy(pkt->data, can->data, can->canfd_len);

        for (int i = 0; i < pkt->signal_num; i++) {
            if (canfd_signal_update(&pkt->signal[i])) {
                if (canfd->all_signal_changed_handle) {
                    canfd->all_signal_changed_handle(&pkt->signal[i]);
                }
            }
        }
        if (pkt->value_changed_cb) {
            pkt->value_changed_cb(pkt);
        }

        if (canfd->all_package_changed_handle) {
            canfd->all_package_changed_handle(pkt);
        }
    }
    return changed;
}

// 解析 CAN FD 消息
int canfd_parse(canfd_struct *canfd, canfd_msgbuff *buffer)
{
    canfd_msgbuff *canMsg = NULL;
    canfd_packet_struct *can_pkt = NULL;

    canMsg = buffer;
    can_pkt = canfd_search_pkt_from_id(canfd, htonl(canMsg->canfd_id));
    if (NULL == can_pkt)
        return -1;
    log_never("canfd_id = 0x%X, canfd_len = %d\n",
              can_pkt->canfd_id, can_pkt->dlc);

    update_pkt(static_cast_real(canfd), can_pkt, canMsg);
    return 0;
}

// get candata.json 相关内容
static int canfd_load_json_config(canfd_struct *canfd, const char *config_path)
{
    FILE *can_file = NULL;
    cJSON *jsonroot, *item, *signals, *signal_item = NULL;
    char buf[45056] = {0};
    uint32_t id;
    int i, j = 0;

    log_info("ready to get candata.json content\n");
    // 打开一个只读文件，该文件必须存在
    can_file = fopen(config_path, "r");
    if (NULL == can_file)
    {
        log_err("fopen CanData.json file fail: %s\n", strerror(errno));
        return -1;
    }

    // 读取文件内容
    if (fread(buf, sizeof(buf), 1, can_file) < 0)
    {
        perror("fread error\n");
        fclose(can_file);
        return -1;
    }
    fclose(can_file);

    // 解析 JSON 数据
    jsonroot = cjson_parse(buf);
    if (NULL == jsonroot)
    {
        log_err("parse candata.json data fail: %s\n", cjson_get_error_ptr());
        return -1;
    }

    // get canfd packet number
    canfd->num = cjson_get_array_size(jsonroot);
    log_info("canfd packet num: %d\n", canfd->num);

    for (i = 0; i < canfd->num; i++)
    {
        item = cjson_get_array_item(jsonroot, i);
        if (NULL == item)
            continue;

        // *********************** CANID *************************
        char *canfd_id = cjson_get_object_item(item, "CANID")->valuestring;
        sscanf(canfd_id, "%x", &id);

        // *********************** DLC *************************
        uint8_t dlc = cjson_get_object_item(item, "DLC")->valueint;

        // *********************** signals *************************
        signals = cjson_get_object_item_case_sensitive(item, "Signals");
        if (signals)
        {
            // 获取signal数量
            uint8_t signal_num = cjson_get_array_size(signals);
            log_never("canfd packet signal num: %d\n", signal_num);

            canfd_packet_struct *pkt = canfd_packet_new(id, dlc, signal_num);
            if (NULL == pkt) {
                log_err("malloc canfd packet fail\n");
                return -1;
            }

            for (j = 0; j < pkt->signal_num; j++)
            {
                signal_item = cjson_get_array_item(signals, j);

                if (NULL == signal_item)
                    continue;

                signal_struct *sig = &pkt->signal[j];
                cJSON *data = cjson_get_object_item(signal_item, "Data");
                if (data)
                {
                    char *name = cjson_get_object_item(signal_item, "Name")->valuestring;
                    int start_byte = cjson_get_object_item(data, "startByte")->valueint;
                    int start_bit = cjson_get_object_item(data, "startBit")->valueint;
                    int bit_length = cjson_get_object_item(data, "bitLength")->valueint;
                    if (canfd_signal_init(sig, name, start_byte, start_bit, bit_length)) {
                        log_err("the signal[%s]\'s bit length is error, canfd is less than 512\n", sig->name);
                        exit(-1);
                    }
                    canfd_signal_set_parent(sig, pkt);
                }
            }
            avl_insert(static_cast_real(canfd)->canfd_tree, &pkt->node);
        }
    }

    // 释放内存
    cjson_delete(jsonroot);
    jsonroot = NULL;

    canfd_print(canfd);
    return 0;
}

int canfd_load_config(canfd_struct *canfd, const char *config_path)
{
    return canfd_load_json_config(canfd, config_path);
}

// 释放CAN FD数据包资源
int canfd_destroy(canfd_struct *canfd)
{
    if (canfd == NULL)
        return 0;
    real_canfd_struct *real = static_cast_real(canfd);
    canfd_packet_struct *pkt, *tmp;
    list_for_each_entry_safe(pkt, tmp, real->canfd_tree, node) {
        printf("pkt->canfd_id: 0x%x\n", pkt->canfd_id);
        canfd_packet_free(pkt);
        pkt = NULL;
    }
    printf("+++2\n");
    avl_tree_free(real->canfd_tree);
    real->canfd_tree = NULL;
    free(real);
    return 0;
}

void canfd_print(canfd_struct *canfd)
{
    if (canfd == NULL)
        return;

    canfd_packet_struct *pkt = NULL;
    signal_struct *sig = NULL;
    list_for_each_entry(pkt, static_cast_real(canfd)->canfd_tree, node) {
        printf("CANID(hex): 0x%02x\t%d\n",
               pkt->canfd_id,
               pkt->dlc);
        for (int j = 0; j < pkt->signal_num; j++)
        {
            sig = &pkt->signal[j];
            printf("sig_name: %s\n", sig->name);
            printf("start_byte: %d\tstart_bit: %d\tbit_length: %d\n",
                   sig->start_byte,
                   sig->start_bit,
                   sig->bit_length);
        }
        printf("\n");
    }
}
