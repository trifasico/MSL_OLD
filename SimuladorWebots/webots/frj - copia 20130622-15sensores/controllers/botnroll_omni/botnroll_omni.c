/*
 * File:        Botnroll_omni.c
 * Date:        May 31, 2013
 * Description: Demo for the Bot'n Roll OMNI (by SAR)
 * Author:      Fernando Ribeiro, University of Minho, Guimar�es
 */

#include <webots/robot.h>
#include <webots/servo.h>
#include <webots/distance_sensor.h>
#include <webots/compass.h>
#include <webots/gps.h>
#include <webots/led.h>
#include <webots/light_sensor.h>
#include <webots/touch_sensor.h>
#include <stdio.h>
#include <math.h>

#define TIME_STEP 16

#define SINAL(x) ((x)>0?1:((x)<0?-1:0))
#define RED    0
#define YELLOW 1
#define GREEN  2

#define NR_SENSORES 15

WbDeviceTag wheels[3];
WbDeviceTag SonarFrente, SonarTras, SonarEsquerda, SonarDireita;
WbDeviceTag Bussola;
WbDeviceTag gps;
WbDeviceTag led[3]={ 0,0,0};
WbDeviceTag LuzC, LuzF, LuzT, LuzE, LuzD;
WbDeviceTag Kicker;
WbDeviceTag SToque;
WbDeviceTag SBola[NR_SENSORES];

//Variaveis Globais - guardam valores dos sensores
int SF, ST, SE, SD;
int LuzCvalor, LuzFvalor, LuzTvalor, LuzEvalor, LuzDvalor;
double gpsValor[3];
int SBolaValor[NR_SENSORES];
double BOLAAng;       // numero do sensor que v� a bola [1-15]
int BOLAValor;  // Valor lido pelo sensor de bola (para calcular a distancia)
int ANG_BUSSOLA=90;
int KickerValue=0;

int estado=1;   // esta variavel controla o GrafCet (comportamento do robo)

#define ACERTO_VEL_ANGULAR 5

#define ALVOMEIO     1
#define ALVOESQUERDA 0
#define ALVODIREITA  2
double ALVOZ[3]={-0.23, 0.0, 0.23};
double ALVOX=0.95;
double PosIniGRX=-0.78, PosIniGRZ=0.0;

int direccao=180, vel_linear=0, vel_angular=0;

int BolaAngulos[NR_SENSORES]   ={ 0, 25, 33, 79, 99, 120, 141, 159, 201, 219, 240, 261, 281, 327, 335};
int BolaAngulosRel[NR_SENSORES]={25, 25,  8, 46, 20,  21,  21,  18,  42,  18,  21,  21,  20,  46,   8};


#include "outros.c"

void orienta(double xf, double zf)
{
double xd, zd;
double angulo;
double temp;

  xd=gpsValor[0] - xf;
  zd=gpsValor[2] - zf;
  angulo = M_PI/2 + atan2(xd, zd);     norm_rad(&angulo);
  temp=((double)(ANG_BUSSOLA-90)-(angulo*180.0/M_PI));
  corrige_vel_angular(-temp);
//  printf("BUSS=%d angulo=%lf temp=%lf\n", ANG_BUSSOLA, angulo*180.0/M_PI, temp);
}

void orientaBussola(int norte)
{
int temp;

  temp=(ANG_BUSSOLA-norte); norm_graus(&temp);
  corrige_vel_angular((double)(-temp));
//  printf("BUSS=%d temp=%d\n", ANG_BUSSOLA, temp);
}

void orientaBola(void)  // ainda n�o est� a ser usado
{
double angulo;

  angulo = BOLAAng;
  norm_graus_double(&angulo);
  corrige_vel_angular(angulo);
//  printf("\nangulo=%.4lf ", angulo);
}

void vai_para(double xf, double zf)
{
double xd, zd;
double angulo;

  xd=gpsValor[0] - xf;
  zd=gpsValor[2] - zf;
  angulo = atan2(xd, zd);
  direccao=(int)(0.5+(angulo-M_PI/2)*180.0/M_PI);
  if (direccao<0) direccao+=360;
//  printf("angulo=%lf direcao=%d\n", angulo*180.0/M_PI, direccao);
}

void orbita(double xf, double zf) // N�o est� a ser usado
{
double xd, zd;
double angulo;

  xd=gpsValor[0] - xf;
  zd=gpsValor[2] - zf;
  angulo = atan2(xd, zd);
//  printf("\nangulo=%.4lf ", angulo*180.0/M_PI);

  if (angulo>M_PI/2 || angulo<-M_PI/2)
    angulo += M_PI;
  norm_rad(&angulo);
  
  direccao=(int)(0.5 + angulo*180.0/M_PI);
  if (direccao<0) direccao+=360;
//  printf("Nangulo=%.4f direccao=%d\n", angulo*180.0/M_PI, direccao);
}

void orbitaBola(int lado)
{
double angulo;

  angulo = (BOLAAng-90.0)*M_PI/180.0;
//  printf("\nangulo=%.4lf ", angulo*180.0/M_PI);

  switch (lado)
  {
    case 0: // na zona central do campo (lado mais perto)
            if (angulo>M_PI/2 || angulo<-M_PI/2)
              angulo += M_PI;
            break;
    case 1: // lado direito
            angulo += M_PI;
            break;
    case -1: // lado esquerdo
            break;
  }
  norm_rad(&angulo);
  direccao=(int)(0.5 + angulo*180.0/M_PI);
  if (direccao<0) direccao+=360;
//  printf("lado=%d Nangulo=%.4f direccao=%d\n", lado, angulo*180.0/M_PI, direccao);
}

int valida_dentro_campo(void)
{
int flag=0;
    if (gpsValor[2]> 0.65) direccao=270, flag= 1;
    if (gpsValor[2]<-0.65) direccao= 90, flag=-1;
    if (gpsValor[0]> 0.95) direccao=  0;
    if (gpsValor[0]<-0.95) direccao=180;
return flag;
}

char estados[12][25]={"(nulo)",
                      "Vai posicao inicial",
                      "Vai � bola em recta",
                      "Orbita",
                      "Ataca",
                      "",
                      "",
                      "",
                      "",
                      "",
                      "Guarda-Redes"};

#define RaioRobo 0.105
#define DIST_LIMITE_IR_BOLA      1.5
#define DIST_LIMITE_ATACAR_BOLA  0.25
#define DIST_LIMITE_GUARDA_REDES 0.4

/*
int decide_orbita(double x1, double z1, double x2, double z2, double x3, double z3)
{
double mr, mt;
double x, z, r;
int result=0;

  if(x2!=x1 && x2!=x3)  // evita divis�es por 0
  {
    mr=(z2-z1)/(x2-x1);
    mt=(z3-z2)/(x3-x2);
    if(mr!=mt && mr!=0)  // evita divis�es por 0
    {
      x=(mr*mt*(z3-z1)+mr*(x2+x3)-mt*(x1+x2))/(2*(mr-mt));
      z=-(1/mr)*(x-(x1+x2)/2)+((z1+z2)/2);
      r=sqrt((x-x1)*(x-x1)+(z-z1)*(z-z1));
      printf("O(%lf,%lf) R=%lf\n", x, z, r);
    }
    if(z+r> 0.65)    result= 1; // atingir� linha lateral direita
    if(z-r<-0.65)    result=-1; // atingir� linha lateral esquerda
  }
return result;
}
*/

int main(void)
{
int estado_old=0;
int flag_dentro_campo=0;
double distancia;
double BolaX, BolaZ;
double temp;
double kickertemp=0, kickertemp_old=0;

  wb_robot_init();  // initialize Webots
  declara_sensores();

  vel_linear=10;
  vel_angular=0;
  
  while (1) {

    le_tecla_decide_comportamento();

    le_sensores();

    distancia=RaioRobo + (0.000895*(BOLAValor*BOLAValor) -0.71255726*BOLAValor+162.98)/100;
    BolaX = gpsValor[0]+distancia*sin(M_PI/2 + BOLAAng*M_PI/180.0);
    BolaZ = gpsValor[2]+distancia*cos(M_PI/2 + BOLAAng*M_PI/180.0);
//printf("{%.3lf %.3lf %.3lf} ", distancia, BolaX, BolaZ);
    

    switch(estado)
    {
      case 1: // vai para posicao inicial do jogo (proteje a baliza)
              if (distancia<DIST_LIMITE_IR_BOLA) estado=2;
              if (estado==1)
              {
                vai_para(-0.3, 0.0);
                orienta(ALVOX, gpsValor[2]);
              }
              break;
      case 2: // Vai para a bola em linha recta
              flag_dentro_campo = 0;
              if (distancia>DIST_LIMITE_IR_BOLA)  estado=1;
              if (distancia-RaioRobo<DIST_LIMITE_ATACAR_BOLA) estado=3; // Problema no calculo da distancia
              if (estado==2)
              {
                vai_para(BolaX-0.2, BolaZ);
                orienta(ALVOX, gpsValor[2]);
              }
              break;
      case 3: // orbita
              if (distancia>DIST_LIMITE_IR_BOLA)             estado=1;
              if (distancia-RaioRobo>DIST_LIMITE_ATACAR_BOLA)   estado=2;
              if (BOLAAng<5 || BOLAAng>355)  estado=4;
              if(estado==3) // se n�o � para sair deste estado
              {
                if ((BOLAAng>90.0 && BOLAAng<180.0) && BolaZ>0.3) // Coordenada 0.3 a rever (dever� ser menor)
                  flag_dentro_campo=1;
                if ((BOLAAng>180.0 && BOLAAng<270.0) && BolaZ<-0.3) // Coordenada 0.3 a rever (dever� ser menor)
                  flag_dentro_campo=-1;
//                if(valida_dentro_campo()!=0)
//                  flag_dentro_campo = valida_dentro_campo();
                orbitaBola(flag_dentro_campo);
                orienta(ALVOX, gpsValor[2]); // � para tirar
              }
              break;
      case 4: // ataca a bola
              flag_dentro_campo = 0;
              if (distancia-RaioRobo>DIST_LIMITE_ATACAR_BOLA)   estado=2;  // NAO CORRIGE QUANDO BOLA MEXE
              if (estado==4)
              {
                vai_para(BolaX, BolaZ+gpsValor[2]/2.0); // VALOR A CORRIGIR
                if (gpsValor[2]>0)
                  orienta(ALVOX, ALVOZ[ALVODIREITA]);
                else
                  orienta(ALVOX, ALVOZ[ALVOESQUERDA]);
//                orienta(ALVOX, ALVOZ[ALVOMEIO]);  // estou a ignorar o if anterior
              }
              break;
      case 10: // GUARDA-REDES
              if(BOLAValor>100)   // VALOR A CONFIGURAR... VE_BOLA ?
              {
                temp=BOLAAng; norm_graus_double(&temp);
                if (temp> 20.0) temp= 20.0;
                if (temp<-20.0) temp=-20.0;
                temp=-temp/80.0;
              }
              else
                temp=0.0;
              vai_para(PosIniGRX, PosIniGRZ + temp);
              orientaBussola(90);
              break;
    }
    
    // codigo para o chuto
    {
      kickertemp_old=kickertemp;
      kickertemp=wb_touch_sensor_get_value(SToque);
      if (kickertemp!=kickertemp_old)
      {
        if (kickertemp > 0)
          wb_servo_set_force(Kicker,50.0);
        else
          wb_servo_set_position(Kicker, 0.0);
      }
    }
    
    (void)valida_dentro_campo();
    if (estado!=estado_old)
    {
      printf("\nEstado=%d - %s", estado, estados[estado]);
      estado_old = estado;
    }
    
//    printf("dir(%3d) vlin(%3d) vang(%3d)\n", direccao, vel_linear, vel_angular);
//  estado=0; direccao=0; vel_linear=0; vel_angular=-10;
    andar(direccao, vel_linear, vel_angular);
    wb_robot_step(TIME_STEP);
  }
  andar(0, 0, 0);

  wb_robot_cleanup();
  return 0;
}
