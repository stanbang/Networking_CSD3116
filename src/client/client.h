/**
 * @file
 * @brief Shared client state and interfaces for networking, rendering and frame access.
 *
 * Team contribution split: Stanley Lur 50%, Jordain Ng 50%.
 * Project AI-use disclosure: docs/AI_USAGE.md.
 */
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
    atomic_uint_fast64_t int_end_deadline;
    pthread_mutex_t str_mutex;
    Frame arr_frames[AST_FRAME_HISTORY];
    unsigned int int_head, int_frames;
    EventQueue str_events;
    LoginCommand str_login_command;
    LoginView str_login_view;
    bool b_login_command;
} Client;
/** @brief Run the client DDS worker until shutdown. */
void *client_worker(void *ptr_client);
/** @brief Run the graphical client on the calling thread until its deadline or exit. */
bool render_run(Client *str_client, uint64_t int_deadline);
/** @brief Report whether the process has received a shutdown signal. */
bool client_interrupted(void);
/** @brief Copy the frames and interpolation factor for the next render. */
bool client_frames(Client *str_client, Frame *str_from, Frame *str_to, float *flt_alpha);
/** @brief Choose the current bot input from the configured strategy. */
uint16_t client_bot_keys(Client *str_client);
/** @brief Report whether the client has reached its round-end deadline. */
bool client_round_ended(const Client *str_client);
#endif
