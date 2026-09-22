#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspgum.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

PSP_MODULE_INFO("FrutigerAeroWorld", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define SCREEN_W 480
#define SCREEN_H 272
#define WORLD_W 3200.0f
#define WORLD_H 2400.0f
#define MAX_ORBS 192
#define MAX_FRIENDS 4
#define PI 3.14159265f

typedef struct { unsigned int color; float x, y, z; } Vertex;

typedef struct { float x, y; int got; } Orb;
typedef struct { float x, y, dx; unsigned int c1, c2; } Friend;

typedef struct { float x,y,w,h; int kind; } Landmark;

static unsigned int __attribute__((aligned(16))) list[262144];
static int running = 1;
static int show_map = 1;
static float px = 1560.0f, py = 1180.0f;
static float energy = 100.0f;
static int orb_count = 0;
static Orb orbs[MAX_ORBS];
static Friend friends[MAX_FRIENDS];
static Landmark landmarks[] = {
    {1580,1120,220,120,0}, {600,1250,260,90,1}, {2580,1330,300,100,2},
    {620,460,340,140,3}, {1540,480,420,150,4}, {2500,480,300,130,5}
};

static inline unsigned int rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return GU_RGBA(r,g,b,a);
}

static void gu_start(void) {
    sceGuStart(GU_DIRECT, list);
    sceGuDisable(GU_TEXTURE_2D);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_BLEND);
    sceGuColor(0xffffffff);
}

static void gu_end(void) {
    sceGuFinish();
    sceGuSync(0,0);
}

static void rect(float x, float y, float w, float h, unsigned int color) {
    Vertex v[2] = {
        {color, x, y, 0.0f},
        {color, x+w, y+h, 0.0f}
    };
    sceGuColor(color);
    sceGuDrawArray(GU_SPRITES,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
        2, 0, v);
}

static void circle(float cx, float cy, float r, unsigned int color) {
    Vertex v[10];
    v[0].color=color; v[0].x=cx; v[0].y=cy; v[0].z=0;
    for (int i=0;i<9;i++) {
        float a=(float)i*2.0f*PI/8.0f;
        v[i+1].color=color;
        v[i+1].x=cx+cosf(a)*r;
        v[i+1].y=cy+sinf(a)*r;
        v[i+1].z=0;
    }
    sceGuColor(color);
    sceGuDrawArray(GU_TRIANGLE_FAN,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
        10, 0, v);
}

static void blob(float cx, float cy, float r, unsigned int c1, unsigned int c2) {
    circle(cx, cy+8, r*1.05f, c1);
    circle(cx, cy-r*0.50f, r*0.68f, c1);
    circle(cx-r*0.25f, cy-r*0.72f, r*0.18f, c2);
    circle(cx+r*0.25f, cy-r*0.72f, r*0.12f, c2);
    rect(cx-r*0.42f, cy+r*0.55f, r*0.30f, r*0.40f, c1);
    rect(cx+r*0.12f, cy+r*0.55f, r*0.30f, r*0.40f, c1);
}

static void tree(float x,float y,float s) {
    rect(x-4*s,y,8*s,28*s,rgba(132,88,50,255));
    circle(x,y-8*s,20*s,rgba(69,220,108,255));
    circle(x-10*s,y-22*s,13*s,rgba(125,245,104,255));
    circle(x+10*s,y-24*s,13*s,rgba(86,232,125,255));
}

static void wind_turbine(float x,float y) {
    rect(x-2,y,4,52,rgba(240,250,255,255));
    circle(x,y-3,5,rgba(80,225,230,255));
    rect(x+4,y-22,3,20,rgba(240,250,255,255));
    rect(x+6,y-8,25,3,rgba(240,250,255,255));
    rect(x+4,y-8,3,20,rgba(240,250,255,255));
}

static void building(float x,float y,float w,float h,unsigned int roof,unsigned int body) {
    rect(x,y,w,h,body);
    rect(x,y,w,7,roof);
    for (int yy=16; yy<h-8; yy+=18)
        for (int xx=10; xx<w-10; xx+=24)
            rect(x+xx,y+yy,11,8,rgba(222,250,255,255));
}

static void init_world(void) {
    int n=0;
    for (int y=300; y<=2200 && n<MAX_ORBS; y+=180) {
        for (int x=260; x<=2960 && n<MAX_ORBS; x+=220) {
            orbs[n].x=x+((y/180)%2)*70;
            orbs[n].y=y; orbs[n].got=0; n++;
        }
    }
    friends[0]=(Friend){1490,1040,1.0f,GU_RGBA(165,115,240,255),GU_RGBA(255,120,200,255)};
    friends[1]=(Friend){1870,1170,-0.8f,GU_RGBA(70,145,255,255),GU_RGBA(30,220,225,255)};
    friends[2]=(Friend){510,1170,0.6f,GU_RGBA(255,225,70,255),GU_RGBA(255,160,70,255)};
    friends[3]=(Friend){2700,1280,-0.5f,GU_RGBA(66,235,110,255),GU_RGBA(170,255,105,255)};
}

static int zone_at(float x,float y) {
    if (y<900) return 0;       /* skyline */
    if (x<1100) return 1;      /* reef */
    if (x>2100) return 2;      /* meadow */
    return 3;                   /* aero */
}

static void draw_cloud(float x,float y,float w,float h) {
    unsigned int c=rgba(255,255,255,240);
    rect(x,y+h*.35f,w,h*.65f,c);
    circle(x+w*.28f,y+h*.35f,h*.38f,c);
    circle(x+w*.53f,y+h*.18f,h*.46f,c);
    circle(x+w*.72f,y+h*.38f,h*.32f,c);
}

static void draw_background(float camx,float camy) {
    rect(0,0,SCREEN_W,SCREEN_H,rgba(92,205,255,255));
    rect(0,118,SCREEN_W,55,rgba(190,245,255,255));

    draw_cloud(30-camx*.18f,35-camy*.08f,105,24);
    draw_cloud(720-camx*.18f,100-camy*.08f,125,26);
    draw_cloud(1400-camx*.18f,58-camy*.08f,105,24);
    draw_cloud(2150-camx*.18f,92-camy*.08f,135,28);
    draw_cloud(2860-camx*.18f,45-camy*.08f,100,24);

    int minx=(int)floorf(camx/80)-1, maxx=(int)floorf((camx+SCREEN_W)/80)+1;
    int miny=(int)floorf(camy/80)-1, maxy=(int)floorf((camy+SCREEN_H)/80)+1;
    for (int ty=miny;ty<=maxy;ty++) for (int tx=minx;tx<=maxx;tx++) {
        float wx=tx*80, wy=ty*80;
        int z=zone_at(wx+40,wy+40);
        unsigned int c;
        if (z==1) c=(ty&1)?rgba(71,207,238,255):rgba(26,166,224,255);
        else if (z==0) c=(tx&1)?rgba(36,170,74,255):rgba(74,205,86,255);
        else if (z==2) c=(tx&1)?rgba(92,220,100,255):rgba(75,205,85,255);
        else c=(tx&1)?rgba(61,196,78,255):rgba(74,205,86,255);
        rect(wx-camx,wy-camy,81,81,c);
        if (z!=1 && (tx%5==0 || ty%5==0)) {
            rect(wx-camx,wy-camy,81,81,rgba(145,150,160,255));
            if (ty%5==0) rect(wx-camx+8,wy-camy+38,36,4,rgba(186,191,198,255));
        }
        if (z==1 && ((tx+ty)%3==0)) {
            rect(wx-camx+12,wy-camy+58,48,8,rgba(13,110,181,255));
            rect(wx-camx+25,wy-camy+50,5,8,GU_RGBA(255,120,200,255));
            rect(wx-camx+35,wy-camy+45,5,13,GU_RGBA(255,225,70,255));
        }
    }
}

static void draw_landmarks(float camx,float camy) {
    for (unsigned int i=0;i<sizeof(landmarks)/sizeof(landmarks[0]);i++) {
        Landmark o=landmarks[i];
        float x=o.x-camx,y=o.y-camy;
        if (x<-o.w-50 || x>SCREEN_W+50 || y<-o.h-80 || y>SCREEN_H+80) continue;
        if(o.kind==0) {
            rect(x,y,o.w,o.h,rgba(110,240,220,255));
            rect(x+12,y+12,o.w-24,20,rgba(222,250,255,255));
            rect(x+35,y+38,o.w-70,55,rgba(80,170,225,255));
            circle(x+110,y+66,12,rgba(255,255,255,255));
        } else if(o.kind==1) {
            rect(x,y,o.w,o.h,rgba(47,185,215,255));
            for(int k=0;k<8;k++) {
                rect(x+16+k*30,y+52-(k&1)*16,10,22,rgba(255,120,200,255));
                rect(x+27+k*30,y+58-(k%3)*10,8,16,rgba(255,225,70,255));
            }
        } else if(o.kind==2) {
            rect(x,y,o.w,o.h,rgba(77,224,100,255));
            for(int k=0;k<6;k++) tree(x+35+k*52,y+88-(k&1)*12,0.8f);
        } else if(o.kind==3) {
            building(x,y+20,o.w,o.h-20,GU_RGBA(30,220,225,255),GU_RGBA(222,250,255,255));
            wind_turbine(x+o.w/2,y);
        } else if(o.kind==4) {
            building(x+20,y+40,72,110,GU_RGBA(30,220,225,255),GU_RGBA(222,250,255,255));
            building(x+110,y+20,80,130,GU_RGBA(165,115,240,255),GU_RGBA(222,250,255,255));
            building(x+215,y+55,70,95,GU_RGBA(70,145,255,255),GU_RGBA(222,250,255,255));
        } else {
            building(x,y+20,o.w,o.h-20,GU_RGBA(170,255,105,255),GU_RGBA(222,250,255,255));
            rect(x+22,y+42,65,30,GU_RGBA(83,220,150,255));
            rect(x+102,y+42,65,30,GU_RGBA(110,210,255,255));
            rect(x+182,y+42,65,30,GU_RGBA(255,250,200,255));
        }
    }
    for(int i=0;i<18;i++) {
        float wx=140+(i*173)%2920, wy=980+(i*127)%1120;
        float x=wx-camx,y=wy-camy;
        if(x>-60&&x<SCREEN_W+60&&y>-80&&y<SCREEN_H+80) {
            if(i%3==0) wind_turbine(x,y); else tree(x,y,0.9f+(i&1)*0.25f);
        }
    }
    for(int i=0;i<9;i++) {
        float wx=250+i*300,h=90+(i%4)*35;
        float x=wx-camx,y=760-h-camy;
        if(x>-160&&x<SCREEN_W+40) building(x,y,72,h,(i&1)?GU_RGBA(66,235,110,255):GU_RGBA(30,220,225,255),GU_RGBA(222,250,255,255));
    }
}

static void draw_orbs(float camx,float camy) {
    for(int i=0;i<MAX_ORBS;i++) if(!orbs[i].got) {
        float x=orbs[i].x-camx, y=orbs[i].y-camy;
        if(x>-12&&x<SCREEN_W+12&&y>-12&&y<SCREEN_H+12) {
            circle(x,y,6,GU_RGBA(255,225,70,255));
            circle(x-2,y-2,2,GU_RGBA(255,255,255,255));
        }
    }
}

static void draw_bubbles(float camx,float camy) {
    for(int i=0;i<28;i++) {
        float wx=fmodf((float)(i*173),WORLD_W);
        float wy=fmodf((float)(i*97),WORLD_H);
        wy -= fmodf((float)(i*17),WORLD_H) * 0.01f;
        float x=wx-camx, y=wy-camy;
        if(zone_at(wx,wy)==1 && x>-8&&x<SCREEN_W+8&&y>-8&&y<SCREEN_H+8)
            circle(x,y,2+(i%4),GU_RGBA(220,250,255,170));
    }
}

static void draw_friends(float camx,float camy) {
    for(int i=0;i<MAX_FRIENDS;i++) {
        Friend *f=&friends[i];
        f->x += f->dx*0.35f;
        if(f->x<250||f->x>2950) f->dx=-f->dx;
        float x=f->x-camx,y=f->y-camy;
        if(x>-50&&x<SCREEN_W+50&&y>-70&&y<SCREEN_H+50) blob(x,y,22,f->c1,f->c2);
    }
}

static void draw_player(void) {
    blob(240,136,24,GU_RGBA(30,220,225,255),GU_RGBA(255,255,255,255));
    rect(226,154,9,14,GU_RGBA(70,145,255,255));
    rect(245,154,9,14,GU_RGBA(70,145,255,255));
    circle(232,130,3,GU_RGBA(12,28,48,255));
    circle(249,130,3,GU_RGBA(12,28,48,255));
}

static void draw_hud(float camx,float camy) {
    rect(0,0,SCREEN_W,24,GU_RGBA(8,50,85,255));
    rect(10,6,120,6,GU_RGBA(30,220,225,255));
    rect(10,6,(energy/100.0f)*120,6,GU_RGBA(170,255,105,255));
    rect(150,6,orb_count*2,6,GU_RGBA(255,225,70,255));
    if(show_map) {
        float mx=350,my=32,mw=120,mh=80;
        rect(mx,my,mw,mh,GU_RGBA(240,255,255,255));
        rect(mx+2,my+2,mw-4,mh-4,GU_RGBA(65,185,215,255));
        rect(mx+4,my+4,112,24,GU_RGBA(100,205,245,255));
        rect(mx+4,my+28,42,48,GU_RGBA(50,140,200,255));
        rect(mx+46,my+28,40,48,GU_RGBA(80,205,95,255));
        rect(mx+86,my+28,30,48,GU_RGBA(95,220,105,255));
        float mxp=mx+4+(px/WORLD_W)*(mw-8), myp=my+4+(py/WORLD_H)*(mh-8);
        rect(mxp-2,myp-2,5,5,GU_RGBA(255,120,200,255));
    }
    /* minimal button legends */
    rect(8,246,92,18,GU_RGBA(8,50,85,230));
    rect(104,246,92,18,GU_RGBA(8,50,85,230));
    rect(208,246,92,18,GU_RGBA(8,50,85,230));
    rect(312,246,92,18,GU_RGBA(8,50,85,230));
}

static void handle_input(void) {
    SceCtrlData pad;
    sceCtrlReadBufferPositive(&pad,1);
    int dx=0,dy=0;
    if(pad.Buttons & PSP_CTRL_LEFT) dx--;
    if(pad.Buttons & PSP_CTRL_RIGHT) dx++;
    if(pad.Buttons & PSP_CTRL_UP) dy--;
    if(pad.Buttons & PSP_CTRL_DOWN) dy++;
    int ax=(int)pad.Lx-128, ay=(int)pad.Ly-128;
    if(abs(ax)>30 || abs(ay)>30) {
        dx=(ax>30)-(ax<-30); dy=(ay>30)-(ay<-30);
    }
    float sp=(pad.Buttons & PSP_CTRL_SQUARE) ? 4.4f : 2.6f;
    if(dx||dy) {
        float len=sqrtf((float)(dx*dx+dy*dy));
        px=px+(dx/len)*sp; py=py+(dy/len)*sp;
        if(pad.Buttons & PSP_CTRL_SQUARE) energy-=0.35f; else energy+=0.10f;
    } else energy+=0.10f;
    if(energy<0)energy=0;if(energy>100)energy=100;
    if(px<20)px=20;if(px>WORLD_W-20)px=WORLD_W-20;
    if(py<20)py=20;if(py>WORLD_H-20)py=WORLD_H-20;

    static unsigned int old=0;
    unsigned int now=pad.Buttons;
    if((now & PSP_CTRL_CROSS) && !(old & PSP_CTRL_CROSS)) {
        for(int i=0;i<MAX_ORBS;i++) if(!orbs[i].got) {
            float dx=px-orbs[i].x,dy=py-orbs[i].y;
            if(dx*dx+dy*dy<38*38) { orbs[i].got=1; orb_count++; break; }
        }
    }
    if((now & PSP_CTRL_TRIANGLE) && !(old & PSP_CTRL_TRIANGLE)) show_map=!show_map;
    if(now & PSP_CTRL_START) running=0;
    old=now;
}

static void init_graphics(void) {
    sceGuInit();
    gu_start();
    sceGuDrawBuffer(GU_PSM_8888,(void*)0,512);
    sceGuDispBuffer(SCREEN_W,SCREEN_H,(void*)0x88000,512);
    sceGuDepthBuffer((void*)0x110000,512);
    sceGuOffset(2048-(SCREEN_W/2),2048-(SCREEN_H/2));
    sceGuViewport(2048,2048,SCREEN_W,SCREEN_H);
    sceGuDepthRange(65535,0);
    sceGuScissor(0,0,SCREEN_W,SCREEN_H);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuFinish(); sceGuSync(0,0); sceGuDisplay(GU_TRUE);
}

int main(void) {
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    init_world();
    init_graphics();

    while(running) {
        handle_input();
        float camx=px-SCREEN_W/2.0f, camy=py-SCREEN_H/2.0f;
        if(camx<0)camx=0; if(camx>WORLD_W-SCREEN_W)camx=WORLD_W-SCREEN_W;
        if(camy<0)camy=0; if(camy>WORLD_H-SCREEN_H)camy=WORLD_H-SCREEN_H;

        gu_start();
        draw_background(camx,camy);
        draw_landmarks(camx,camy);
        draw_orbs(camx,camy);
        draw_friends(camx,camy);
        draw_bubbles(camx,camy);
        draw_player();
        draw_hud(camx,camy);
        gu_end();
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }
    sceGuDisplay(GU_FALSE);
    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
