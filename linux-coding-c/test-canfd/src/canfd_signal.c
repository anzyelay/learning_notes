#include <string.h>
#include "can_tool.h"
#include "canfd_signal.h"
#include "canfd_packet.h"
#include "include/canfd_log.h"

void canfd_signal_set_parent(signal_struct *sig,  struct canfd_packet *parent)
{
    if (sig) {
        sig->parent = parent;
    }
}

bool canfd_signal_set_cbfun(signal_struct *sig,  canfd_sig_changed_cb fun, void *usrdata)
{
    if (!sig)
        return false;
    if (fun)
        sig->value_changed_cb = fun;
    if (usrdata)
        sig->user_data = usrdata;
    return true;
}

int canfd_signal_init(signal_struct *sig, const char *name,int start_byte ,int start_bit,int bit_length)
{
    memset(sig, 0, sizeof(*sig));
    sig->value_changed_cb = NULL;
    sig->user_data = NULL;
    sig->parent = NULL;

    pthread_spin_init(&sig->lock, PTHREAD_PROCESS_PRIVATE);

    strncpy(sig->name, name, sizeof(sig->name));
    sig->start_byte = start_byte;
    sig->start_bit = start_bit;
    sig->bit_length = bit_length;
    if (sig->bit_length > 8*64) {
        log_err("the signal[%s]\'s bit length is error, canfd is less than 512\n", sig->name);
        return 1;
    }
    else if (sig->bit_length > 64) {
        sig->data_type = SIG_DATA_TYPE_POINTER;
        sig->data_len = (int)((sig->bit_length + 7)/8);
        sig->value.pu8 = (uint8_t *)malloc(sig->data_len);
    }
    else if (sig->bit_length > 32) {
        sig->data_type = SIG_DATA_TYPE_UINT64;
        sig->data_len = 8;
    }
    else if (sig->bit_length > 16) {
        sig->data_type = SIG_DATA_TYPE_UINT32;
        sig->data_len = 4;
    }
    else if (sig->bit_length > 8) {
        sig->data_type = SIG_DATA_TYPE_UINT16;
        sig->data_len = 2;
    }
    else if (sig->bit_length > 1) {
        sig->data_type = SIG_DATA_TYPE_UINT8;
        sig->data_len = 1;
    }
    else {
        sig->data_type = SIG_DATA_TYPE_BOOL;
        sig->data_len = 1;
    }
    return 0;
}

signal_struct *canfd_signal_new(const char *name, int start_byte, int start_bit, int bit_length)
{
    signal_struct *sig = malloc(sizeof(signal_struct));
    if (!sig) {
        return NULL;
    }
    if (canfd_signal_init(sig, name, start_byte, start_bit, bit_length)) {
        free(sig);
        sig = NULL;
    }
    return sig;
}

bool canfd_signal_release(signal_struct *sig)
{
    if (NULL == sig)
        return false;

    // 释放锁资源
    pthread_spin_destroy(&sig->lock);

    if (sig->data_type == SIG_DATA_TYPE_POINTER) {
        free(sig->value.pu8);
        sig->value.pu8 = NULL;
    }
    return true;
}

// 释放信号量资源
void canfd_signal_free(signal_struct *sig)
{
    if (canfd_signal_release(sig)) {
        free(sig);
    }
}

bool canfd_signal_update(signal_struct *sig)
{
    // 检查指针是否为空
    if(NULL == sig || NULL == sig->parent) {
        log_err("Null pointer for sig or his parent\n");
        return false;
    }
    uint8_t *pdata = sig->parent->data;

    // 判断信号量数据类型, 如果是指针类型，则直接赋值，否则需要解析数据
    if (sig->data_type == SIG_DATA_TYPE_POINTER) {
        if (!memcmp(sig->value.pu8, &pdata[sig->start_byte], sig->data_len)) {
            return false;
        }

        pthread_spin_lock(&sig->lock); // 加锁
        memcpy(sig->value.pu8, &pdata[sig->start_byte], sig->data_len);
        pthread_spin_unlock(&sig->lock); // 解锁
    }
    else {
        uint64_t val = get_motorola_backward_data_from(pdata, sig->parent->dlc ,sig->start_byte, sig->start_bit, sig->bit_length);
        if (sig->value.u64 == val) {
            return false;
        }
        pthread_spin_lock(&sig->lock); // 加锁
        sig->value.u64 = val;
        pthread_spin_unlock(&sig->lock); // 解锁
    }

    if (sig->value_changed_cb) {
        sig->value_changed_cb(sig);
    }
    return true;
}

bool canfd_signal_set_value(signal_struct *sig, uint64_t value)
{
    if (!sig ||sig->data_type == SIG_DATA_TYPE_POINTER) {
        return false;
    }
    pthread_spin_lock(&sig->lock); // 加锁
    sig->value.u64 = value;
    pthread_spin_unlock(&sig->lock);
    if (sig->parent) {
        canfd_packet_struct *parent = sig->parent;
        set_motorola_backward_data_from(parent->data, value, parent->dlc, sig->start_byte, sig->start_bit, sig->bit_length);
    }
    return true;
}

bool canfd_signal_set_values(signal_struct *sig, uint8_t *value, int len)
{
    if (!sig || sig->data_type != SIG_DATA_TYPE_POINTER) {
        return false;
    }
    if (len > sig->data_len) {
        return false;
    }

    pthread_spin_lock(&sig->lock); // 加锁
    memcpy(sig->value.pu8, value, len);
    pthread_spin_unlock(&sig->lock);

    if (sig->parent) {
        canfd_packet_struct *parent = sig->parent;
        memcpy(&parent->data[sig->start_byte], sig->value.pu8, sig->data_len);
    }
    return true;
}

signal_struct *canfd_signal_dup2(signal_struct *sig, signal_struct *new)
{
    signal_struct *rs = NULL;
    if (!sig) {
        return rs;
    }

    if (!new) {
        rs = canfd_signal_new(sig->name, sig->start_byte, sig->start_bit, sig->bit_length);
    }
    else {
        rs = new;
        canfd_signal_init(rs, sig->name, sig->start_byte, sig->start_bit, sig->bit_length);
    }

    if (rs) {
        if (sig->data_type == SIG_DATA_TYPE_POINTER) {
            canfd_signal_set_values(rs, sig->value.pu8, sig->data_len);
        }
        else {
            canfd_signal_set_value(rs, sig->value.u64);
        }
    }

    return rs;
}
