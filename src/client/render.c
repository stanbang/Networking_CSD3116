/* Main-thread neon line renderer with delayed interpolation. Author: <team>/Codex, 2026-11-04. */
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stb_easy_font.h>
#include "client/client.h"
#define AST_PARTICLES 256
typedef struct { char str_text[160]; int int_quads; _Alignas(16) char arr_vertices[16384]; } Text;
typedef struct { float flt_x,flt_y,flt_vx,flt_vy,flt_life; } Particle;
typedef struct {
    Text str_title,str_status,str_wave,str_controls,str_over,str_restart,arr_players[AST_PLAYERS];
    Particle arr_particles[AST_PARTICLES];
    Frame str_previous;
    uint64_t int_last_time;
} Renderer;
static const float arr_colors[AST_PLAYERS][3] = {{0.25f,1,0.75f},{0.4f,0.65f,1},{1,0.5f,0.7f},{1,0.85f,0.3f},{0.75f,0.5f,1}};

/* Rebuild cached glyph vertices only when the text changes. */
static void text_draw(Text *str_mesh, const char *str_text, float flt_x, float flt_y, float flt_scale)
{
    if (strcmp(str_mesh->str_text,str_text) != 0) {
        snprintf(str_mesh->str_text,sizeof(str_mesh->str_text),"%s",str_text);
        str_mesh->int_quads = stb_easy_font_print(0,0,str_mesh->str_text,NULL,str_mesh->arr_vertices,sizeof(str_mesh->arr_vertices));
    }
    glPushMatrix(); glTranslatef(flt_x,flt_y,0); glScalef(flt_scale,flt_scale,1);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2,GL_FLOAT,16,str_mesh->arr_vertices);
    glDrawArrays(GL_QUADS,0,str_mesh->int_quads*4);
    glDisableClientState(GL_VERTEX_ARRAY); glPopMatrix();
}

/* Deterministic stars require no texture allocations or per-frame random calls. */
static void background(void)
{
    glClearColor(0.018f,0.025f,0.055f,1); glClear(GL_COLOR_BUFFER_BIT);
    glPointSize(1.5f); glColor3f(0.22f,0.29f,0.45f); glBegin(GL_POINTS);
    for (unsigned int int_i=0;int_i<110;++int_i) glVertex2f((float)((int_i*173+47)%AST_WIDTH),(float)((int_i*277+83)%AST_HEIGHT));
    glEnd();
    glColor3f(0.08f,0.17f,0.25f); glBegin(GL_LINE_LOOP);
    glVertex2f(8,8);glVertex2f(AST_WIDTH-8,8);glVertex2f(AST_WIDTH-8,AST_HEIGHT-8);glVertex2f(8,AST_HEIGHT-8);
    glEnd();
}

/* Follow the shortest toroidal displacement; ordinary lerp jumps across wrap seams. */
static float interpolate(int32_t int_from, int32_t int_to, float flt_alpha, float flt_extent)
{
    float flt_start=int_from/100.0f,flt_delta=(int_to-int_from)/100.0f;
    if (flt_delta>flt_extent/2) flt_delta-=flt_extent;
    else if (flt_delta< -flt_extent/2) flt_delta+=flt_extent;
    return sim_wrap(flt_start+flt_delta*flt_alpha,flt_extent);
}

/* Angles also cross a periodic seam. */
static float angle_lerp(uint16_t int_from,uint16_t int_to,float flt_alpha)
{
    float flt_start=int_from*(2*AST_PI/65535.0f);
    float flt_end=int_to*(2*AST_PI/65535.0f);
    return flt_start+remainderf(flt_end-flt_start,2*AST_PI)*flt_alpha;
}

/* Ships use slot identity; new/disappearing slots are never read outside bounds. */
static const Ast_Ship *old_ship(const Frame *str_frame,unsigned int int_id)
{
    for(unsigned int int_i=0;int_i<str_frame->int_ships;++int_i)
        if(str_frame->arr_ships[int_i].int_player_id==int_id) return &str_frame->arr_ships[int_i];
    return NULL;
}

/* A small flame is cosmetic; only the server's position/angle drives the hull. */
static void ship_draw(const Ast_Ship *str_ship,const Ast_Ship *str_old,float flt_alpha,uint64_t int_tick)
{
    if(!str_ship->int_lives||str_ship->int_respawn||(str_ship->int_invulnerable&&int_tick%12<6)) return;
    if(!str_old||str_old->int_respawn||str_old->int_lives!=str_ship->int_lives) str_old=str_ship;
    float flt_x=interpolate(str_old->int_x,str_ship->int_x,flt_alpha,AST_WIDTH);
    float flt_y=interpolate(str_old->int_y,str_ship->int_y,flt_alpha,AST_HEIGHT);
    float flt_angle=angle_lerp(str_old->int_angle,str_ship->int_angle,flt_alpha);
    glPushMatrix();glTranslatef(flt_x,flt_y,0);glRotatef(flt_angle*(180/AST_PI),0,0,1);
    glColor3fv(arr_colors[str_ship->int_player_id-1]);glLineWidth(2);glBegin(GL_LINE_LOOP);
    glVertex2f(15,0);glVertex2f(-10,-9);glVertex2f(-6,0);glVertex2f(-10,9);glEnd();
    if(str_ship->int_vx*str_ship->int_vx+str_ship->int_vy*str_ship->int_vy>100000) {
        glColor3f(1,0.55f,0.2f);glBegin(GL_LINE_STRIP);glVertex2f(-9,-4);glVertex2f(-19-(float)(int_tick%5),0);glVertex2f(-9,4);glEnd();
    }
    glLineWidth(1);glPopMatrix();
}

/* Irregular outlines derive from public ids, with bounded12-vertex geometry. */
static void asteroids_draw(const Frame *str_from,const Frame *str_to,float flt_alpha)
{
    glColor3f(0.57f,0.65f,0.79f);glLineWidth(1.5f);
    for(unsigned int int_i=0;int_i<str_to->int_asteroids;++int_i) {
        const Ast_Asteroid *str_now=&str_to->arr_asteroids[int_i],*str_old=str_now;
        for(unsigned int int_j=0;int_j<str_from->int_asteroids;++int_j)
            if(str_from->arr_asteroids[int_j].int_id==str_now->int_id) {str_old=&str_from->arr_asteroids[int_j];break;}
        float flt_x=interpolate(str_old->int_x,str_now->int_x,flt_alpha,AST_WIDTH);
        float flt_y=interpolate(str_old->int_y,str_now->int_y,flt_alpha,AST_HEIGHT);
        glBegin(GL_LINE_LOOP);
        for(unsigned int int_k=0;int_k<12;++int_k) {
            float flt_angle=int_k*(2*AST_PI/12)+(float)str_to->int_tick*0.002f;
            float flt_radius=sim_radius(str_now->int_size)*(0.80f+(float)((str_now->int_id*17+int_k*7)%21)/100);
            glVertex2f(flt_x+cosf(flt_angle)*flt_radius,flt_y+sinf(flt_angle)*flt_radius);
        }
        glEnd();
    }
    glLineWidth(1);
}

/* Bullet ids distinguish pool reuse from a continuous projectile trajectory. */
static void bullets_draw(const Frame *str_from,const Frame *str_to,float flt_alpha)
{
    glPointSize(4);glColor3f(0.78f,1,0.85f);glBegin(GL_POINTS);
    for(unsigned int int_i=0;int_i<str_to->int_bullets;++int_i) {
        const Ast_Bullet *str_now=&str_to->arr_bullets[int_i],*str_old=str_now;
        for(unsigned int int_j=0;int_j<str_from->int_bullets;++int_j)
            if(str_from->arr_bullets[int_j].int_id==str_now->int_id) {str_old=&str_from->arr_bullets[int_j];break;}
        glVertex2f(interpolate(str_old->int_x,str_now->int_x,flt_alpha,AST_WIDTH),interpolate(str_old->int_y,str_now->int_y,flt_alpha,AST_HEIGHT));
    }
    glEnd();
}

/* Cosmetic sparks use a fixed ring-like free-slot search, never heap allocation. */
static void burst(Renderer *str_renderer,float flt_x,float flt_y)
{
    unsigned int int_added=0;
    for(unsigned int int_i=0;int_i<AST_PARTICLES&&int_added<10;++int_i) if(str_renderer->arr_particles[int_i].flt_life<=0) {
        float flt_angle=int_added*(2*AST_PI/10);
        str_renderer->arr_particles[int_i]=(Particle){.flt_x=flt_x,.flt_y=flt_y,
            .flt_vx=cosf(flt_angle)*65,.flt_vy=sinf(flt_angle)*65,.flt_life=0.6f};
        ++int_added;
    }
}

/* Infer cosmetic destruction events from validated entity-id disappearance. */
static void particles_update(Renderer *str_renderer,const Frame *str_frame,float flt_dt)
{
    const Frame *str_old=&str_renderer->str_previous;
    if(str_frame->int_seq!=str_old->int_seq) {
        for(unsigned int int_i=0;int_i<str_old->int_asteroids;++int_i) {
            bool b_found=false;
            for(unsigned int int_j=0;int_j<str_frame->int_asteroids;++int_j)
                b_found|=str_old->arr_asteroids[int_i].int_id==str_frame->arr_asteroids[int_j].int_id;
            if(!b_found) burst(str_renderer,str_old->arr_asteroids[int_i].int_x/100.0f,str_old->arr_asteroids[int_i].int_y/100.0f);
        }
        str_renderer->str_previous=*str_frame;
    }
    glPointSize(2);glBegin(GL_POINTS);
    for(unsigned int int_i=0;int_i<AST_PARTICLES;++int_i) {
        Particle *str_particle=&str_renderer->arr_particles[int_i];
        if(str_particle->flt_life<=0) continue;
        str_particle->flt_life-=flt_dt;
        str_particle->flt_x=sim_wrap(str_particle->flt_x+str_particle->flt_vx*flt_dt,AST_WIDTH);
        str_particle->flt_y=sim_wrap(str_particle->flt_y+str_particle->flt_vy*flt_dt,AST_HEIGHT);
        glColor3f(1,fmaxf(0,str_particle->flt_life),0.2f);glVertex2f(str_particle->flt_x,str_particle->flt_y);
    }
    glEnd();
}

/* Compact HUD reports authoritative health/score and an input-echo RTT estimate. */
static void hud(Renderer *str_renderer,Client *str_client,const Frame *str_frame)
{
    char str_text[160]={0};
    unsigned int int_player=atomic_load(&str_client->int_player_id);
    glColor3f(0.45f,0.9f,1);text_draw(&str_renderer->str_title,"DDS ASTEROIDS",24,20,3);
    bool b_stale=str_frame->int_received&&monotonic_ns()-str_frame->int_received>AST_LEASE_NS;
    bool b_own_state=old_ship(str_frame,int_player)!=NULL;
    const char *str_status=atomic_load(&str_client->b_rejected)?"REJECTED":b_stale?"CONNECTION LOST":
        int_player&&b_own_state?"CONNECTED / DDS SECURE":int_player?"WAITING FOR GAME STATE":
        atomic_load(&str_client->b_ready)?"AUTHENTICATING":"WAITING FOR SERVER";
    snprintf(str_text,sizeof(str_text),"%s   P%u   RTT %ums",str_status,int_player,atomic_load(&str_client->int_ping_ms));
    glColor3f(0.4f,0.8f,0.7f);text_draw(&str_renderer->str_status,str_text,24,53,1.5f);
    snprintf(str_text,sizeof(str_text),"WAVE %u",str_frame->int_wave);text_draw(&str_renderer->str_wave,str_text,655,25,2);
    for(unsigned int int_i=0;int_i<str_frame->int_ships;++int_i) {
        const Ast_Ship *str_ship=&str_frame->arr_ships[int_i];
        snprintf(str_text,sizeof(str_text),"P%u  LIVES %u  SCORE %u%s",str_ship->int_player_id,str_ship->int_lives,str_ship->int_score,str_ship->int_respawn?"  RESPAWNING":"");
        glColor3fv(arr_colors[str_ship->int_player_id-1]);text_draw(&str_renderer->arr_players[int_i],str_text,24,80+int_i*16,1.2f);
    }
    glColor3f(0.4f,0.48f,0.63f);text_draw(&str_renderer->str_controls,"ARROWS: TURN / THRUST    SPACE: FIRE    R: RESTART",110,575,1.5f);
    if(str_frame->b_gameover) {
        glColor3f(1,0.4f,0.4f);text_draw(&str_renderer->str_over,"GAME OVER",230,265,5);
        glColor3f(0.8f,0.85f,1);text_draw(&str_renderer->str_restart,"PRESS R TO RESTART",250,325,2);
    }
}

/* Main thread samples keys as state; all authority remains in server physics. */
static void input_keys(Client *str_client,GLFWwindow *str_window)
{
    unsigned int int_keys=0;
    if(glfwGetKey(str_window,GLFW_KEY_UP)==GLFW_PRESS) int_keys|=AST_KEY_THRUST;
    if(glfwGetKey(str_window,GLFW_KEY_LEFT)==GLFW_PRESS) int_keys|=AST_KEY_LEFT;
    if(glfwGetKey(str_window,GLFW_KEY_RIGHT)==GLFW_PRESS) int_keys|=AST_KEY_RIGHT;
    if(glfwGetKey(str_window,GLFW_KEY_SPACE)==GLFW_PRESS) int_keys|=AST_KEY_FIRE;
    if(glfwGetKey(str_window,GLFW_KEY_R)==GLFW_PRESS) int_keys|=AST_KEY_RESTART;
    atomic_store(&str_client->int_keys,int_keys);
    if(glfwGetKey(str_window,GLFW_KEY_ESCAPE)==GLFW_PRESS) glfwSetWindowShouldClose(str_window,GLFW_TRUE);
}

/* Frame copies and particle buffers live on main's stack for the entire window. */
bool render_run(Client *str_client,uint64_t int_deadline)
{
    if(!glfwInit()) {fprintf(stderr,"glfwInit failed\n");return false;}
    GLFWwindow *str_window=glfwCreateWindow(AST_WIDTH,AST_HEIGHT,"DDS Asteroids",NULL,NULL);
    if(!str_window) {glfwTerminate();return false;}
    glfwMakeContextCurrent(str_window);glfwSwapInterval(1);stb_easy_font_spacing(0);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,AST_WIDTH,AST_HEIGHT,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    Renderer str_renderer={0};str_renderer.int_last_time=monotonic_ns();
    while(!client_interrupted()&&!glfwWindowShouldClose(str_window)&&
        (!atomic_load(&str_client->b_stop)||atomic_load(&str_client->b_rejected))&&monotonic_ns()<int_deadline) {
        glfwPollEvents();input_keys(str_client,str_window);background();
        Frame str_from={0},str_to={0};float flt_alpha=1;
        uint64_t int_now=monotonic_ns();float flt_dt=fminf((float)(int_now-str_renderer.int_last_time)/1e9f,0.1f);
        str_renderer.int_last_time=int_now;
        if(client_frames(str_client,&str_from,&str_to,&flt_alpha)) {
            asteroids_draw(&str_from,&str_to,flt_alpha);bullets_draw(&str_from,&str_to,flt_alpha);
            for(unsigned int int_i=0;int_i<str_to.int_ships;++int_i)
                ship_draw(&str_to.arr_ships[int_i],old_ship(&str_from,str_to.arr_ships[int_i].int_player_id),flt_alpha,str_to.int_tick);
            particles_update(&str_renderer,&str_to,flt_dt);
        }
        hud(&str_renderer,str_client,&str_to);glfwSwapBuffers(str_window);
        sleep_ns(UINT64_C(1000000000)/AST_HZ);
    }
    glfwDestroyWindow(str_window);glfwTerminate();return true;
}
