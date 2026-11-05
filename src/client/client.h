/* Client shared state, all scalar handoff atomic. Author: <team>, Codex assistance. Date: 2026-11-03. */
#ifndef AST_CLIENT_H
#define AST_CLIENT_H
#include "common/protocol.h"
#include "common/frame.h"
#include "common/event_queue.h"
#include "client/login.h"
#define AST_FRAME_HISTORY 8
typedef struct Client {
    Options str_options;
    atomic_bool b_stop, b_failed, b_ready, b_rejected;
    atomic_bool b_joined_once, b_account_complete;
    atomic_uint int_player_id, int_keys;
    atomic_uint int_ping_ms;
    atomic_uint_fast64_t int_latest_tick;
    pthread_mutex_t str_mutex;
    Frame arr_frames[AST_FRAME_HISTORY];
    unsigned int int_head, int_frames;
    EventQueue str_events;
    LoginCommand str_login_command;
    LoginView str_login_view;
    bool b_login_command;
} Client;
void *client_worker(void *ptr_client);
bool render_run(Client *str_client, uint64_t int_deadline);
bool client_interrupted(void);
bool client_frames(Client *str_client, Frame *str_from, Frame *str_to, float *flt_alpha);
uint16_t client_bot_keys(Client *str_client);
#endif
