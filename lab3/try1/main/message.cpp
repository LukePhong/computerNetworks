//
// Created by 'Confidence'F on 11/17/2022.
//
#include "message.h"

transmission transmission_message::pure_ack = {0, 0, ack, 0, 0};
transmission transmission_message::pure_syn = {0, 0, syn, 0, 0};
transmission transmission_message::syn_ack = {0, 0, ack + syn, 0, 0};
transmission transmission_message::fin_ack = {0, 0, ack + fin, 0, 0};