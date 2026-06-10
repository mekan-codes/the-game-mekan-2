#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAXP 10000

const double VC = 299792458.0;
const double MU = 4.0e-7*M_PI;
const double QE = -1.60217663e-19;
const double ME = 9.10938363e-31;

typedef struct {
    char name[16];
    int Np;
    double M, Q;
    double X1, X2;
    double X1n, X2n;
    double X1o, X2o;
    double X1oo, X2oo;
    double F1, F2;
    double *m, *q;
    double *x1, *x2;
    double *v1, *v2;
    double *f1, *f2;
} Charge;

int main (int argc, char** argv) {
    srand(time(NULL));

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    TTF_Font* font = TTF_OpenFont("DejaVuSansMono.ttf", 14);
    if (font == NULL) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return 1;
    }
    SDL_Color color = {0, 0, 0};
/*
    if (argc != 2) {
        printf("Usage: %s input\n", argv[0]);
        return 1;
    }
*/
    FILE *fp = fopen("input.txt", "r");
    if (fp == NULL) {
        printf("Cannot open input.txt\n");
        return 1;
    }

    Charge *C;

    char buf[256];
    int n,i,j,k,l;
    int n1, n2;
    int N1, N2, Nc, Ne;
    int W, H;
    double x1s, x1e, dx1, x1;
    double x2s, x2e, dx2, x2;
    double dt, t;
    double *E1, *E2, *B3;
    double E1p, E2p, B3p;
    double *Phi;

    //Reading input
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        char* sharp = strchr(buf, '#');
        if (sharp != NULL) *sharp = '\0';

        char* eq = strchr(buf, '=');
        if (eq == NULL) continue;

        *eq = '\0';
        char* key = strtok(buf, " \t\n");
        char* val = strtok(eq + 1, " \t\n");

        if (key == NULL || val == NULL) continue;

        if (strcmp(key, "dx1") == 0)            dx1 = atof(val);
        else if (strcmp(key, "x1s") == 0)       x1s = atof(val);
        else if (strcmp(key, "x1e") == 0)       x1e = atof(val);
        else if (strcmp(key, "dx2") == 0)       dx2 = atof(val);
        else if (strcmp(key, "x2s") == 0)       x2s = atof(val);
        else if (strcmp(key, "x2e") == 0)       x2e = atof(val);
        else if (strcmp(key, "dt") == 0)        dt = atof(val);
        else if (strcmp(key, "Nc") == 0)        Nc = atoi(val);
        else if (strcmp(key, "Ne") == 0)        Ne = atoi(val);
        else if (strcmp(key, "W") == 0)         W = atoi(val);
        else if (strcmp(key, "H") == 0)         H = atoi(val);
        else printf("Unknown key '%s'\n", key);
    }
    fclose(fp);

    //if (dx1 < dx2)  dt = 0.5*dx1/VC;
    //else            dt = 0.5*dx2/VC;

    N1 = (int)(x1e/dx1 + 0.5);
    N2 = (int)(x2e/dx2 + 0.5);
    x1e = dx1*(double)N1;
    x2e = dx2*(double)N2;

    N1 = (int)((x1e-x1s)/dx1 + 0.5);
    N2 = (int)((x2e-x2s)/dx2 + 0.5);
    x1s = x1e - dx1*(double)N1;
    x2s = x2e - dx2*(double)N2;

    //malloc
    C = (Charge *)malloc(Nc*sizeof(Charge));
    for (k=0;k<Nc;k++) {
        double ran1, ran2;
        ran1 = (double)rand() / (double)RAND_MAX;
        ran2 = (double)rand() / (double)RAND_MAX;

        C[k].Np = 0;
        if (k==0) {
            C[k].M = ME * (double)Ne;
            C[k].Q = -QE * (double)Ne;
        }
        else if (k==1) {
            C[k].M = 1.0;
            C[k].Q = 0.0;
        }
        else {
            C[k].M = 9774.0 * ME;
            C[k].Q = 9774.0 * QE;
        }
        if (k==0) {
            C[k].X1 = C[k].X1n = C[k].X1o = C[k].X1oo = 0.0;
            C[k].X2 = C[k].X2n = C[k].X2o = C[k].X2oo = x2s + 0.1*(x2e-x2s);
        }
        else {
            C[k].X1 = C[k].X1n = C[k].X1o = C[k].X1oo = (0.5 - ran1) * (x1e-x1s);
            C[k].X2 = C[k].X2n = C[k].X2o = C[k].X2oo = (0.5 - ran2) * (x2e-x2s);
        }
        C[k].F1 = C[k].F2 = 0.0;
        C[k].m = (double *)malloc(MAXP*sizeof(double));
        C[k].q = (double *)malloc(MAXP*sizeof(double));
        C[k].x1 = (double *)malloc(MAXP*sizeof(double));
        C[k].x2 = (double *)malloc(MAXP*sizeof(double));
        C[k].v1 = (double *)malloc(MAXP*sizeof(double));
        C[k].v2 = (double *)malloc(MAXP*sizeof(double));
        C[k].f1 = (double *)malloc(MAXP*sizeof(double));
        C[k].f2 = (double *)malloc(MAXP*sizeof(double));
        for (l=0;l<MAXP;l++) {
            C[k].m[l] = C[k].q[l] = 0.0;
            C[k].x1[l] = C[k].x2[l] = 0.0;
            C[k].v1[l] = C[k].v2[l] = 0.0;
            C[k].f1[l] = C[k].f2[l] = 0.0;
        }
    }
    E1 = (double *)malloc(N1*N2*sizeof(double));
    E2 = (double *)malloc(N1*N2*sizeof(double));
    B3 = (double *)malloc(N1*N2*sizeof(double));
    Phi = (double *)malloc(N1*N2*sizeof(double));
    for (i=0;i<N1;i++) for (j=0;j<N2;j++) {
        E1[i+N1*j] = 0.0;
        E2[i+N1*j] = 0.0;
        B3[i+N1*j] = 0.0;
        Phi[i+N1*j] = 0.0;
    }

    //const int W = 800, H = 800;
    const int Size = 0.2*dx1/(x1e-x1s)*(double)W, size = 0.1*dx1/(x1e-x1s)*(double)W;

    SDL_Window* win = SDL_CreateWindow(
        "PHallguYS",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        W, H, SDL_WINDOW_SHOWN);

    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    double err = 0.0, dev = 0.0;
    int running = 1, over = 0, winner = 0, succ = 0, succMax = 0;
    n = 0;
    while (running) {
        t = dt*(double)n;

        k=1;
        C[k].X1 = 2.0e-9 * cos(2.0*M_PI*dt*(double)n/3.572e-16);
        C[k].X2 = 2.0e-9 * sin(2.0*M_PI*dt*(double)n/3.572e-16);

        k=2;
        C[k].X1 = 0.0;
        C[k].X2 = 0.0;

        k=0;
        if (C[1].X1-0.25*dx1 <= C[k].X1 && C[k].X1 <= C[1].X1+0.25*dx1 && C[1].X2-0.25*dx2 <= C[k].X2 && C[k].X2 <= C[1].X2+0.25*dx2 && C[1].X1-0.25*dx1 <= C[k].X1o && C[k].X1o <= C[1].X1+0.25*dx1 && C[1].X2-0.25*dx2 <= C[k].X2o && C[k].X2o <= C[1].X2+0.25*dx2 && C[1].X1-0.25*dx1 <= C[k].X1oo && C[k].X1oo <= C[1].X1+0.25*dx1 && C[1].X2-0.25*dx2 <= C[k].X2oo && C[k].X2oo <= C[1].X2+0.25*dx2) {
            succ++;
            if (succ > succMax) succMax = succ;
        }
        else    succ = 0;
        //if (C[1].X1-0.125*dx1 <= C[k].X1 && C[k].X1 <= C[1].X1+0.125*dx1 && C[1].X2-0.125*dx2 <= C[k].X2 && C[k].X2 <= C[1].X2+0.125*dx2 && C[1].X1-0.125*dx1 <= C[k].X1o && C[k].X1o <= C[1].X1+0.125*dx1 && C[1].X2-0.125*dx2 <= C[k].X2o && C[k].X2o <= C[1].X2+0.125*dx2 && C[1].X1-0.125*dx1 <= C[k].X1oo && C[k].X1oo <= C[1].X1+0.125*dx1 && C[1].X2-0.125*dx2 <= C[k].X2oo && C[k].X2oo <= C[1].X2+0.125*dx2)
        if (succ > 100)
            winner = 1;
        if ((int)(C[k].M/ME) == 1 && winner == 0)
            over = 1;

        for (i=0;i<N1;i++) for (j=0;j<N2;j++) {
            E1[i+N1*j] = 0.0;
            E2[i+N1*j] = 0.0;
            B3[i+N1*j] = 0.0;//-1.0e5;
            Phi[i+N1*j] = 0.0;
        }

        for (k=1;k<Nc;k++) for (i=0;i<N1;i++) for (j=0;j<N2;j++) {
            double rr, Ir;
            x1 = x1s + dx1 * (double)i + 0.5*dx1;
            x2 = x2s + dx2 * (double)j + 0.5*dx2;
            rr = (x1 - C[k].X1)*(x1 - C[k].X1) + (x2 - C[k].X2)*(x2 - C[k].X2);
            if(rr < 0.25*dx1*dx1)
                    Ir = 2.0/dx1;
            else    Ir = 1.0/sqrt(rr);
            Phi[i+N1*j] += C[k].Q * MU*VC*VC/(4.0*M_PI) * Ir;
        }

        for (i=1;i<N1-1;i++) for (j=1;j<N2-1;j++) {
            E1[i+N1*j] = -0.5 * (Phi[i+1+N1*j] - Phi[i-1+N1*j]) / dx1;
            E2[i+N1*j] = -0.5 * (Phi[i+N1*(j+1)] - Phi[i+N1*(j-1)]) / dx2;
        }
 
        for (k=Nc-1;k>=0;k--) {
            double D;
            i = (int)((C[k].X1-x1s-0.5*dx1)/dx1);
            j = (int)((C[k].X2-x2s-0.5*dx2)/dx2);

            if (0 <= i && i < N1-1 && 0 <= j && j < N2-1) {
                E1p = E1[i+N1*j]*(x1s+dx1*(double)(i+1)+0.5*dx1-C[k].X1)*(x2s+dx2*(double)(j+1)+0.5*dx2-C[k].X2);
                E1p += E1[i+1+N1*j]*(C[k].X1-x1s-dx1*(double)i-0.5*dx1)*(x2s+dx2*(double)(j+1)+0.5*dx2-C[k].X2);
                E1p += E1[i+N1*(j+1)]*(x1s+dx1*(double)(i+1)+0.5*dx1-C[k].X1)*(C[k].X2-x2s-dx2*(double)j-0.5*dx2);
                E1p += E1[i+1+N1*(j+1)]*(C[k].X1-x1s-dx1*(double)i-0.5*dx1)*(C[k].X2-x2s-dx2*(double)j-0.5*dx2);
                E1p /= (dx1*dx2); 

                E2p = E2[i+N1*j]*(x1s+dx1*(double)(i+1)+0.5*dx1-C[k].X1)*(x2s+dx2*(double)(j+1)+0.5*dx2-C[k].X2);
                E2p += E2[i+1+N1*j]*(C[k].X1-x1s-dx1*(double)i-0.5*dx1)*(x2s+dx2*(double)(j+1)+0.5*dx2-C[k].X2);
                E2p += E2[i+N1*(j+1)]*(x1s+dx1*(double)(i+1)+0.5*dx1-C[k].X1)*(C[k].X2-x2s-dx2*(double)j-0.5*dx2);
                E2p += E2[i+1+N1*(j+1)]*(C[k].X1-x1s-dx1*(double)i-0.5*dx1)*(C[k].X2-x2s-dx2*(double)j-0.5*dx2);
                E2p /= (dx1*dx2); 

                B3p = B3[i+N1*j]*(x1s+dx1*(double)(i+1)+0.5*dx1-C[k].X1)*(x2s+dx2*(double)(j+1)+0.5*dx2-C[k].X2);
                B3p += B3[i+1+N1*j]*(C[k].X1-x1s-dx1*(double)i-0.5*dx1)*(x2s+dx2*(double)(j+1)+0.5*dx2-C[k].X2);
                B3p += B3[i+N1*(j+1)]*(x1s+dx1*(double)(i+1)+0.5*dx1-C[k].X1)*(C[k].X2-x2s-dx2*(double)j-0.5*dx2);
                B3p += B3[i+1+N1*(j+1)]*(C[k].X1-x1s-dx1*(double)i-0.5*dx1)*(C[k].X2-x2s-dx2*(double)j-0.5*dx2);
                B3p /= (dx1*dx2); 

                D = 0.5 * C[k].Q*B3p*dt / C[k].M;
                C[k].F1 = E1p + B3p * (C[k].X2 - C[k].X2o)/dt;
                C[k].F1 += D*(E2p - B3p * (C[k].X1 - C[k].X1o)/dt);
                C[k].F2 = E2p - B3p * (C[k].X1 - C[k].X1o)/dt;
                C[k].F2 -= D*(E1p + B3p * (C[k].X2 - C[k].X2o)/dt);
 
                //C[k].F1 = E1[i+N1*j];
                //C[k].F2 = E2[i+N1*j];

                D = 1.0 + D*D;
                C[k].F1 *= C[k].Q/D;
                C[k].F2 *= C[k].Q/D;
            }

            for (l=0;l<C[k].Np;l++) {
                i = (int)((C[k].x1[l]-x1s-0.5*dx1)/dx1);
                j = (int)((C[k].x2[l]-x2s-0.5*dx2)/dx2);

                if (0 <= i && i < N1-1 && 0 <= j && j < N2-1) {
                    double e1, e2, b3;

                    e1 = E1[i+N1*j]*(x1s+dx1*(double)(i+1)+0.5*dx1-C[k].x1[l])*(x2s+dx2*(double)(j+1)+0.5*dx2-C[k].x2[l]);
                    e1 += E1[i+1+N1*j]*(C[k].x1[l]-x1s-dx1*(double)i-0.5*dx1)*(x2s+dx2*(double)(j+1)+0.5*dx2-C[k].x2[l]);
                    e1 += E1[i+N1*(j+1)]*(x1s+dx1*(double)(i+1)+0.5*dx1-C[k].x1[l])*(C[k].x2[l]-x2s-dx2*(double)j-0.5*dx2);
                    e1 += E1[i+1+N1*(j+1)]*(C[k].x1[l]-x1s-dx1*(double)i-0.5*dx1)*(C[k].x2[l]-x2s-dx2*(double)j-0.5*dx2);
                    e1 /= (dx1*dx2); 

                    e2 = E2[i+N1*j]*(x1s+dx1*(double)(i+1)+0.5*dx1-C[k].x1[l])*(x2s+dx2*(double)(j+1)+0.5*dx2-C[k].x2[l]);
                    e2 += E2[i+1+N1*j]*(C[k].x1[l]-x1s-dx1*(double)i-0.5*dx1)*(x2s+dx2*(double)(j+1)+0.5*dx2-C[k].x2[l]);
                    e2 += E2[i+N1*(j+1)]*(x1s+dx1*(double)(i+1)+0.5*dx1-C[k].x1[l])*(C[k].x2[l]-x2s-dx2*(double)j-0.5*dx2);
                    e2 += E2[i+1+N1*(j+1)]*(C[k].x1[l]-x1s-dx1*(double)i-0.5*dx1)*(C[k].x2[l]-x2s-dx2*(double)j-0.5*dx2);
                    e2 /= (dx1*dx2); 

                    b3 = B3[i+N1*j]*(x1s+dx1*(double)(i+1)+0.5*dx1-C[k].x1[l])*(x2s+dx2*(double)(j+1)+0.5*dx2-C[k].x2[l]);
                    b3 += B3[i+1+N1*j]*(C[k].x1[l]-x1s-dx1*(double)i-0.5*dx1)*(x2s+dx2*(double)(j+1)+0.5*dx2-C[k].x2[l]);
                    b3 += B3[i+N1*(j+1)]*(x1s+dx1*(double)(i+1)+0.5*dx1-C[k].x1[l])*(C[k].x2[l]-x2s-dx2*(double)j-0.5*dx2);
                    b3 += B3[i+1+N1*(j+1)]*(C[k].x1[l]-x1s-dx1*(double)i-0.5*dx1)*(C[k].x2[l]-x2s-dx2*(double)j-0.5*dx2);
                    b3 /= (dx1*dx2); 


                    D = 0.5 * C[k].q[l]*b3*dt / C[k].m[l];
                    C[k].f1[l] = e1 + b3 * C[k].v2[l];
                    C[k].f1[l] += D*(e2 - b3 * C[k].v1[l]); //(C[k].x1[l] - C[k].x1o[l])/dt);
                    C[k].f2[l] = e2 - b3 * C[k].v1[l];
                    C[k].f2[l] -= D*(e1 + b3 * C[k].v2[l]);

                    //C[k].f1[l] = E1[i+N1*j];
                    //C[k].f2[l] = E2[i+N1*j];

                    D = 1.0 + D*D;
                    C[k].f1[l] *= C[k].q[l]/D;
                    C[k].f2[l] *= C[k].q[l]/D;
                }
            }

        }

        for (k=0;k<Nc;k++) for (l=0;l<C[k].Np;l++) {
            C[k].v1[l] += C[k].f1[l] * dt / C[k].m[l];
            C[k].v2[l] += C[k].f2[l] * dt / C[k].m[l];

            C[k].f1[l] = 0.0;
            C[k].f2[l] = 0.0;
        }

        SDL_Event e;
        while (SDL_PollEvent(&e)) {

            if (over) {
                if (e.type == SDL_QUIT) {
                    running = 0;
                    break;
                }
                continue;
            }

            if (e.type == SDL_QUIT) {
                running = 0;
            }

            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                for (k=0;k<Nc;k++) {
                    double x1p, x2p, xX;
                    double X1, X2;
                    double X1o, X2o;
                    double ran1, ran2;
                    ran1 = (double)rand() / (double)RAND_MAX;
                    ran2 = (double)rand() / (double)RAND_MAX;

                    X1 = C[k].X1;
                    X1o = C[k].X1o;
                    X2 = C[k].X2;
                    X2o = C[k].X2o;

                    if (C[k].Np < MAXP && (int)(C[k].M/ME) > 1) {
                        C[k].x1[C[k].Np] = X1;
                        C[k].x2[C[k].Np] = X2;
                        if (k==0) {
                            x1p = x1s + (x1e-x1s) * (double)(e.button.x)/(double)W;
                            x2p = x2s + (x2e-x2s) * (double)(H - e.button.y)/(double)H;
                            xX = (x1p - X1)*(x1p - X1) + (x2p - X2)*(x2p - X2);

                            C[k].v1[C[k].Np] = (X1 - X1o)/dt + 0.25*VC * (x1p - X1)/sqrt(xX);
                            C[k].v2[C[k].Np] = (X2 - X2o)/dt + 0.25*VC * (x2p - X2)/sqrt(xX);
                            C[k].m[C[k].Np] = ME;
                            C[k].q[C[k].Np] = -QE;
                        }
                        else {
                            C[k].v1[C[k].Np] = (X1 - X1o)/dt + 0.5*VC * ran1 * cos(2.0*M_PI*ran2);
                            C[k].v2[C[k].Np] = (X2 - X2o)/dt + 0.5*VC * ran1 * sin(2.0*M_PI*ran2);
                            C[k].m[C[k].Np] = ME;
                            C[k].q[C[k].Np] = QE;
                        }

                        C[k].F1 -= C[k].m[C[k].Np] * (C[k].v1[C[k].Np]*dt - C[k].X1 + C[k].X1o) / (dt*dt);
                        C[k].F2 -= C[k].m[C[k].Np] * (C[k].v2[C[k].Np]*dt - C[k].X2 + C[k].X2o) / (dt*dt);
                        C[k].M -= C[k].m[C[k].Np];
                        C[k].Q -= C[k].q[C[k].Np];

                        C[k].Np++;
                    }

                }
            }
 
        }

            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            SDL_RenderClear(ren);

            SDL_SetRenderDrawColor(ren, 191, 191, 191, 255);
            for (i=0;i<N1;i++) {
                n1 = (int)(dx1*(double)i/(x1e-x1s)*(double)W);
                SDL_RenderDrawLine(ren, n1, 0, n1, H);
            }
            for (j=0;j<N2;j++) {
                n2 = H - (int)(dx2*(double)j/(x2e-x2s)*(double)H);
                SDL_RenderDrawLine(ren, 0, n2, W, n2);
            }

            for (i=0;i<N1;i++) for (j=0;j<N2;j++) {
                double dn1, dn2;
                x1 = x1s + dx1*(double)i + 0.5*dx1;
                x2 = x2s + dx2*(double)j + 0.5*dx2;

                n1 = (int)((x1-x1s)/(x1e-x1s)*(double)W);
                n2 = H - (int)((x2-x2s)/(x2e-x2s)*(double)H);

                dn1 = (int)(-1.0e2*E1[i+N1*j]*QE/ME*dt*dt/(x1e-x1s)*(double)W);
                dn2 = (int)(-1.0e2*E2[i+N1*j]*QE/ME*dt*dt/(x2e-x2s)*(double)H);

                SDL_SetRenderDrawColor(ren, 255, 127, 127, 255);
                SDL_RenderDrawLine(ren, n1, n2, n1+dn1, n2-dn2);
            }

            for (k=Nc-1;k>=0;k--) {
                double dn1, dn2;
                if (k==0) {
                    for (l=0;l<C[k].Np;l++) {
                        n1 = (int)((C[k].x1[l]-x1s)/(x1e-x1s)*(double)W);
                        n2 = H - (int)((C[k].x2[l]-x2s)/(x2e-x2s)*(double)H);
                        SDL_SetRenderDrawColor(ren, 0, 0, 255, 255);
                        SDL_Rect propellant = { n1-size/2, n2-size/2, size, size };
                        SDL_RenderFillRect(ren, &propellant);
                    }
                    n1 = (int)((C[k].X1oo-x1s)/(x1e-x1s)*(double)W);
                    n2 = H - (int)((C[k].X2oo-x2s)/(x2e-x2s)*(double)H);
                    SDL_SetRenderDrawColor(ren, 255, 191, 191, 255);
                    SDL_Rect charge = { n1-Size/2, n2-Size/2, Size, Size };
                    SDL_RenderFillRect(ren, &charge);
                    n1 = (int)((C[k].X1o-x1s)/(x1e-x1s)*(double)W);
                    n2 = H - (int)((C[k].X2o-x2s)/(x2e-x2s)*(double)H);
                    SDL_SetRenderDrawColor(ren, 255, 127, 127, 255);
                    charge = (SDL_Rect){ n1-Size/2, n2-Size/2, Size, Size };
                    SDL_RenderFillRect(ren, &charge);
                    n1 = (int)((C[k].X1-x1s)/(x1e-x1s)*(double)W);
                    n2 = H - (int)((C[k].X2-x2s)/(x2e-x2s)*(double)H);
                    SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
                    charge = (SDL_Rect){ n1-Size/2, n2-Size/2, Size, Size };
                    SDL_RenderFillRect(ren, &charge);

                }
                else {
                    //for (l=0;l<C[k].Np;l++) {
                    //    n1 = (int)((C[k].x1[l]-x1s)/(x1e-x1s)*(double)W);
                    //    n2 = H - (int)((C[k].x2[l]-x2s)/(x2e-x2s)*(double)H);
                    //    SDL_SetRenderDrawColor(ren, 127, 127, 127, 255);
                    //    SDL_Rect propellant = { n1-size/2, n2-size/2, size, size };
                    //    SDL_RenderFillRect(ren, &propellant);
                    //}
                    n1 = (int)((C[k].X1-x1s)/(x1e-x1s)*(double)W);
                    n2 = H - (int)((C[k].X2-x2s)/(x2e-x2s)*(double)H);
                    dn1 = (int)(0.5*dx1/(x1e-x1s)*(double)W);
                    dn2 = (int)(0.5*dx2/(x2e-x2s)*(double)H);
                    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
                    SDL_Rect charge = { n1-(int)(dn1/2), n2-(int)(dn2/2), dn1, dn2 };
                    SDL_RenderFillRect(ren, &charge);
                }
 

            }

            char line1[128], line2[128], line3[128], line4[128];
            k=0;
            if (over) {
                sprintf(line1, "Game over (t = %.1f as)", t*1.0e18);
                sprintf(line2, "Your mass = %d Me | Ejected mass = %d Me", (int)(C[k].M/ME), Ne - (int)(C[k].M/ME));
                sprintf(line3, "Velocity: Vx = %.5f, Vy = %.5f (nm/as) | superpos = %d, superposMax = %d", (C[k].X1-C[k].X1o)/dt*1.0e-9, (C[k].X2-C[k].X2o)/dt*1.0e-9, succ, succMax);

                SDL_Surface* s1 = TTF_RenderText_Solid(font, line1, color);
                SDL_Texture* t1 = SDL_CreateTextureFromSurface(ren, s1);
                SDL_Rect r1 = { 10, 10, s1->w, s1->h };
                SDL_RenderCopy(ren, t1, NULL, &r1);
                SDL_FreeSurface(s1);
                SDL_DestroyTexture(t1);

                SDL_Surface* s2 = TTF_RenderText_Solid(font, line2, color);
                SDL_Texture* t2 = SDL_CreateTextureFromSurface(ren, s2);
                SDL_Rect r2 = { 10, 10 + r1.h + 6, s2->w, s2->h };
                SDL_RenderCopy(ren, t2, NULL, &r2);
                SDL_FreeSurface(s2);
                SDL_DestroyTexture(t2);

                SDL_Surface* s3 = TTF_RenderText_Solid(font, line3, color);
                SDL_Texture* t3 = SDL_CreateTextureFromSurface(ren, s3);
                SDL_Rect r3 = { 10, 10 + r1.h + r2.h + 12, s3->w, s3->h };
                SDL_RenderCopy(ren, t3, NULL, &r3);
                SDL_FreeSurface(s3);
                SDL_DestroyTexture(t3);
            }
            else if (winner) {
                sprintf(line1, "Good work! (t = %.1f as)", t*1.0e18);
                sprintf(line2, "Your mass = %d Me | Ejected mass = %d Me", (int)(C[k].M/ME), Ne - (int)(C[k].M/ME));
                sprintf(line3, "Velocity: Vx = %.5f, Vy = %.5f (nm/as) | superpos = %d, superposMax = %d", (C[k].X1-C[k].X1o)/dt*1.0e-9, (C[k].X2-C[k].X2o)/dt*1.0e-9, succ, succMax);

                SDL_Surface* s1 = TTF_RenderText_Solid(font, line1, color);
                SDL_Texture* t1 = SDL_CreateTextureFromSurface(ren, s1);
                SDL_Rect r1 = { 10, 10, s1->w, s1->h };
                SDL_RenderCopy(ren, t1, NULL, &r1);
                SDL_FreeSurface(s1);
                SDL_DestroyTexture(t1);

                SDL_Surface* s2 = TTF_RenderText_Solid(font, line2, color);
                SDL_Texture* t2 = SDL_CreateTextureFromSurface(ren, s2);
                SDL_Rect r2 = { 10, 10 + r1.h + 6, s2->w, s2->h };
                SDL_RenderCopy(ren, t2, NULL, &r2);
                SDL_FreeSurface(s2);
                SDL_DestroyTexture(t2);

                SDL_Surface* s3 = TTF_RenderText_Solid(font, line3, color);
                SDL_Texture* t3 = SDL_CreateTextureFromSurface(ren, s3);
                SDL_Rect r3 = { 10, 10 + r1.h + r2.h + 12, s3->w, s3->h };
                SDL_RenderCopy(ren, t3, NULL, &r3);
                SDL_FreeSurface(s3);
                SDL_DestroyTexture(t3);
            }
            else {
                sprintf(line1, "t = %.1f (as) | X = %.3f, Y = %.3f (nm)", t*1.0e18, C[k].X1*1.0e9, C[k].X2*1.0e9);
                sprintf(line2, "M = %d Me, Q = %d Qe", (int)(C[k].M/ME), (int)(C[k].Q/QE));
                sprintf(line3, "Vx = %.3f, Vy = %.3f (nm/as)", (C[k].X1-C[k].X1o)/dt*1.0e-9, (C[k].X2-C[k].X2o)/dt*1.0e-9);
                sprintf(line4, "Ex = %.5e, Ey = %.5e (V/m), superpos = %d, superposMax = %d", C[k].F1/C[k].Q, C[k].F2/C[k].Q, succ, succMax);

                SDL_Surface* s1 = TTF_RenderText_Solid(font, line1, color);
                SDL_Texture* t1 = SDL_CreateTextureFromSurface(ren, s1);
                SDL_Rect r1 = { 10, 10, s1->w, s1->h };
                SDL_RenderCopy(ren, t1, NULL, &r1);
                SDL_FreeSurface(s1);
                SDL_DestroyTexture(t1);

                SDL_Surface* s2 = TTF_RenderText_Solid(font, line2, color);
                SDL_Texture* t2 = SDL_CreateTextureFromSurface(ren, s2);
                SDL_Rect r2 = { 10, 10 + r1.h + 6, s2->w, s2->h };
                SDL_RenderCopy(ren, t2, NULL, &r2);
                SDL_FreeSurface(s2);
                SDL_DestroyTexture(t2);

                SDL_Surface* s3 = TTF_RenderText_Solid(font, line3, color);
                SDL_Texture* t3 = SDL_CreateTextureFromSurface(ren, s3);
                SDL_Rect r3 = { 10, 10 + r1.h + r2.h + 12, s3->w, s3->h };
                SDL_RenderCopy(ren, t3, NULL, &r3);
                SDL_FreeSurface(s3);
                SDL_DestroyTexture(t3);

                SDL_Surface* s4 = TTF_RenderText_Solid(font, line4, color);
                SDL_Texture* t4 = SDL_CreateTextureFromSurface(ren, s4);
                SDL_Rect r4 = { 10, 10 + r1.h + r2.h + r3.h + 18, s4->w, s4->h };
                SDL_RenderCopy(ren, t4, NULL, &r4);
                SDL_FreeSurface(s4);
                SDL_DestroyTexture(t4);
            }

            SDL_RenderPresent(ren);


        for (k=0;k<Nc;k++) {
            if (k==0) for (l=0;l<C[k].Np;l++) {
                if (C[k].x1[l] + C[k].v1[l]*dt < x1s) {
                    C[k].x1[l] = x1s + (x1s - C[k].x1[l] - C[k].v1[l]*dt);
                    C[k].v1[l] = -C[k].v1[l];
                }
                else if (x1e < C[k].x1[l] + C[k].v1[l]*dt) {
                    C[k].x1[l] = x1e - (C[k].x1[l] + C[k].v1[l]*dt - x1e);
                    C[k].v1[l] = -C[k].v1[l];
                }
                else
                    C[k].x1[l] += C[k].v1[l]*dt;

                if (C[k].x2[l] + C[k].v2[l]*dt < x2s) {
                    C[k].x2[l] = x2s + (x2s - C[k].x2[l] - C[k].v2[l]*dt);
                    C[k].v2[l] = -C[k].v2[l];
                }
                else if (x2e < C[k].x2[l] + C[k].v2[l]*dt) {
                    C[k].x2[l] = x2e - (C[k].x2[l] + C[k].v2[l]*dt - x2e);
                    C[k].v2[l] = -C[k].v2[l];
                }
                else
                    C[k].x2[l] += C[k].v2[l]*dt;
            }
            else for (l=0;l<C[k].Np;l++) {
                C[k].x1[l] += C[k].v1[l]*dt;
                C[k].x2[l] += C[k].v2[l]*dt;
            }


            C[k].X1n = 2.0*C[k].X1 - C[k].X1o + C[k].F1 * dt*dt / C[k].M;
            C[k].X2n = 2.0*C[k].X2 - C[k].X2o + C[k].F2 * dt*dt / C[k].M;

            if (C[k].X1n < x1s) {
                C[k].X1n = x1s + (x1s - C[k].X1n);
                C[k].X1 = x1s + (x1s - C[k].X1);
            }
            else if (x1e < C[k].X1n) {
                C[k].X1n = x1e - (C[k].X1n - x1e);
                C[k].X1 = x1e - (C[k].X1 - x1e);
            }
            if (C[k].X2n < x2s) {
                C[k].X2n = x2s + (x2s - C[k].X2n);
                C[k].X2 = x2s + (x2s - C[k].X2);
            }
            else if (x2e < C[k].X2n) {
                C[k].X2n = x2e - (C[k].X2n - x2e);
                C[k].X2 = x2e - (C[k].X2 - x2e);
            }

            C[k].X1oo = C[k].X1o;
            C[k].X1o = C[k].X1;
            C[k].X1 = C[k].X1n;
            C[k].X2oo = C[k].X2o;
            C[k].X2o = C[k].X2;
            C[k].X2 = C[k].X2n;
            C[k].F1 = 0.0;
            C[k].F2 = 0.0;
        }

        //SDL_RenderPresent(ren);
        SDL_Delay(20);
        n++;
    }

    free(E1);
    free(E2);
    free(B3);
    for (k=0;k<Nc;k++) {
        free(C[k].m);
        free(C[k].x1);
        free(C[k].x2);
        free(C[k].v1);
        free(C[k].v2);
        free(C[k].f1);
        free(C[k].f2);
    }
    free(C);

    TTF_CloseFont(font);
    TTF_Quit();

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

	return 0;	
}
