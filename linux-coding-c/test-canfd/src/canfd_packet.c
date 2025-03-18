#include <string.h>
#include <stdlib.h>
#include "canfd_packet.h"
#include "canfd_parse.h"

bool canfd_packet_set_sigval(canfd_packet_struct *pkt, const char *name, int value)
{
    signal_struct *sig = canfd_search_sig_from_pkt(pkt, name);
    if (sig) {
        canfd_signal_set_parent(sig, pkt);
        return canfd_signal_set_value(sig, value);
    }
    return false;
}

canfd_packet_struct *canfd_packet_dup2(canfd_packet_struct *old, canfd_packet_struct *new)
{
    canfd_packet_struct *rp = NULL;
    if (new) {
        rp = new;
    }
    else {
        rp  = malloc(sizeof(canfd_packet_struct));
    }

    if (!rp) {
        return rp;
    }
    rp->canfd_id = old->canfd_id;
    rp->dlc = old->dlc;
    rp->signal_num = old->signal_num;
    memcpy(rp->data, old->data, rp->dlc);
    for(int i=0; i < old->signal_num; ++i){
        signal_struct *osig= &old->signal[i];
        signal_struct *nsig = &rp->signal[i];
        canfd_signal_set_parent(nsig, rp);
        canfd_signal_dup2(osig, nsig);
    }
    return rp;
}

bool canfd_packet_set_cbfun(canfd_packet_struct *pkt, canfd_pkt_changed_cb fun)
{
    if (!pkt)
        return false;
    if(fun)
        pkt->value_changed_cb = fun;
    return true;
}

canfd_packet_struct *canfd_packet_new(uint32_t canfd_id, uint8_t dlc, uint8_t signal_num)
{
    canfd_packet_struct *pkt = malloc(sizeof(canfd_packet_struct));
    if(pkt == NULL)
        return NULL;
    if (canfd_packet_init(pkt, canfd_id, dlc, signal_num)) {
        free(pkt);
        pkt = NULL;
    }
    return pkt;
}

int canfd_packet_init(canfd_packet_struct *pkt, uint32_t canfd_id, uint8_t dlc, uint8_t signal_num)
{
    memset(pkt, 0, sizeof(*pkt));

    pkt->value_changed_cb = NULL;
    pkt->canfd_id = canfd_id;
    pkt->dlc = dlc;
    pkt->signal_num = signal_num;

    return 0;
}

bool canfd_packet_free(canfd_packet_struct *pkt)
{
    if (pkt == NULL)
        return false;

    for (int j = 0; j < pkt->signal_num; j++) {
        canfd_signal_release(&pkt->signal[j]);
    }

    free(pkt);
    return true;
}
