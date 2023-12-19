#include <Arduino.h>
#line 1 "\\\\tb-pc\\产品资料\\001-线上产品\\002-机械臂手掌\\002 仿生手掌手套\\仿生移动机械手（Gihand2）\\03-源码例程\\002-手套板出厂程序\\002-源码程序\\ZGloveR_Gihand2_V3\\ZGloveR_Gihand2_V3.ino"
/*
    硬件IO分布：
    1、x轴 y轴 I2C芯片
    2、电位器 A6 A7 A0 A2 A3
    3、舵机引脚 11 10 9 8 7 6
    4、led引脚13 蜂鸣器引脚 5 校准按键 3 模式按键 4

    闭合手 控制小车
    张开手 控制云台
    其他控制手指

    20210130
    1、修复校准 包括陀螺仪的校准和手指的校准，校准时候，手套需要保持水平、手指完全闭合和手指完全张开。
*/

#include <Servo.h>            // 调用Servo.h库
#include <Wire.h>
#include <I2Cdev.h>
#include <ADXL345.h>
#include <EEPROM.h>           //调用<EEPROM.h库


#define SERVO_NUM   6               //宏定义电位器数量
#define DWQ_NUM     5               //宏定义电位器数量
#define PIN_beep    5               //宏定义蜂鸣器引脚
#define PIN_nled    13              //宏定义工作指示灯引脚
#define PIN_KEY_J   3               //宏定义校准引脚
#define PIN_KEY_M   4               //宏定义模式引脚

#define ADC_MIN_ADDR   0            //握紧时采集电位器值起始地址
#define ADC_MAX_ADDR   20           //张手时采集电位器值起始地址
#define POS_X_ADDR     30           //X的存储地址
#define POS_Y_ADDR     34           //Y的存储地址
#define JXB_PWM_RANGE  1000.0       //量程
#define JXB_PWM_STRAT  (500+(2000-JXB_PWM_RANGE)/2)     //机械臂PWM起始值
#define X_MIDDLE 0
#define Y_MIDDLE 0

#define beep_on() digitalWrite(PIN_beep, HIGH);
#define beep_off() digitalWrite(PIN_beep, LOW);

#define nled_on() digitalWrite(PIN_nled, LOW);
#define nled_off() digitalWrite(PIN_nled, HIGH);

char cmd_return[168];
Servo myservo[SERVO_NUM];                           //创建舵机类数组
byte servo_pin[SERVO_NUM] = {11, 10, 9, 8, 7, 6};   //定义舵机控制引脚数组
byte dwq_pin[DWQ_NUM] = {A6, A7, A0, A2, A3};       //定义电位器引脚数组
int  ADC_MAX[DWQ_NUM] = {0, 0, 0, 0, 0};            //默认握紧时采集电位器值
int  ADC_MIN[DWQ_NUM] = {0, 0, 0, 0, 0};            //默认张手时采集电位器值
int  ADC_MID[DWQ_NUM] = {0, 0, 0, 0, 0};            //默认张手时采集电位器值
int  pos[DWQ_NUM] = {0}, pos_x=0, pos_y=0;          //变量pos用来存储转化后的电位器数据
int  pos_bak[5] = {0}, pos_x_bak=0, pos_y_bak=0;    //变量pos备份值
int  pwm_value[DWQ_NUM] = {0};                      //变量pwm值
float pos_p[DWQ_NUM] = {1,1,1,1,1};                 //放大倍数
int pos_x_verify = 0, pos_y_verify = 0;             //x y校验值

ADXL345 myAdxl345;
int16_t ax, ay, az;

#line 61 "\\\\tb-pc\\产品资料\\001-线上产品\\002-机械臂手掌\\002 仿生手掌手套\\仿生移动机械手（Gihand2）\\03-源码例程\\002-手套板出厂程序\\002-源码程序\\ZGloveR_Gihand2_V3\\ZGloveR_Gihand2_V3.ino"
void setup();
#line 108 "\\\\tb-pc\\产品资料\\001-线上产品\\002-机械臂手掌\\002 仿生手掌手套\\仿生移动机械手（Gihand2）\\03-源码例程\\002-手套板出厂程序\\002-源码程序\\ZGloveR_Gihand2_V3\\ZGloveR_Gihand2_V3.ino"
void loop();
#line 120 "\\\\tb-pc\\产品资料\\001-线上产品\\002-机械臂手掌\\002 仿生手掌手套\\仿生移动机械手（Gihand2）\\03-源码例程\\002-手套板出厂程序\\002-源码程序\\ZGloveR_Gihand2_V3\\ZGloveR_Gihand2_V3.ino"
void eepromRead(u32 addr, u8 len, u8 *mydat);
#line 132 "\\\\tb-pc\\产品资料\\001-线上产品\\002-机械臂手掌\\002 仿生手掌手套\\仿生移动机械手（Gihand2）\\03-源码例程\\002-手套板出厂程序\\002-源码程序\\ZGloveR_Gihand2_V3\\ZGloveR_Gihand2_V3.ino"
void eepromWrite(u32 addr, u8 len, u8 *mydat);
#line 138 "\\\\tb-pc\\产品资料\\001-线上产品\\002-机械臂手掌\\002 仿生手掌手套\\仿生移动机械手（Gihand2）\\03-源码例程\\002-手套板出厂程序\\002-源码程序\\ZGloveR_Gihand2_V3\\ZGloveR_Gihand2_V3.ino"
void loop_key_j();
#line 200 "\\\\tb-pc\\产品资料\\001-线上产品\\002-机械臂手掌\\002 仿生手掌手套\\仿生移动机械手（Gihand2）\\03-源码例程\\002-手套板出厂程序\\002-源码程序\\ZGloveR_Gihand2_V3\\ZGloveR_Gihand2_V3.ino"
int Filter(int FILTER_N, int pin);
#line 221 "\\\\tb-pc\\产品资料\\001-线上产品\\002-机械臂手掌\\002 仿生手掌手套\\仿生移动机械手（Gihand2）\\03-源码例程\\002-手套板出厂程序\\002-源码程序\\ZGloveR_Gihand2_V3\\ZGloveR_Gihand2_V3.ino"
void adc_read2buf();
#line 272 "\\\\tb-pc\\产品资料\\001-线上产品\\002-机械臂手掌\\002 仿生手掌手套\\仿生移动机械手（Gihand2）\\03-源码例程\\002-手套板出厂程序\\002-源码程序\\ZGloveR_Gihand2_V3\\ZGloveR_Gihand2_V3.ino"
void loop_nled();
#line 290 "\\\\tb-pc\\产品资料\\001-线上产品\\002-机械臂手掌\\002 仿生手掌手套\\仿生移动机械手（Gihand2）\\03-源码例程\\002-手套板出厂程序\\002-源码程序\\ZGloveR_Gihand2_V3\\ZGloveR_Gihand2_V3.ino"
void loop_Gihand_R();
#line 61 "\\\\tb-pc\\产品资料\\001-线上产品\\002-机械臂手掌\\002 仿生手掌手套\\仿生移动机械手（Gihand2）\\03-源码例程\\002-手套板出厂程序\\002-源码程序\\ZGloveR_Gihand2_V3\\ZGloveR_Gihand2_V3.ino"
void setup(){
    Serial.begin(115200);                           //初始化波特率为115200
    pinMode(PIN_nled, OUTPUT);                      //设置LED引脚为输出模式
    pinMode(PIN_beep, OUTPUT);                      //设置蜂鸣器引脚为输出模式
    pinMode(PIN_KEY_M, INPUT_PULLUP);               //将模式按键对应引脚设置为内部上拉输入模式，防止误判
    pinMode(PIN_KEY_J, INPUT_PULLUP);               //将校准按键对应引脚设置为内部上拉输入模式，防止误判

    Wire.begin();
    myAdxl345.initialize();
    myAdxl345.testConnection();

    eepromRead(ADC_MIN_ADDR, sizeof(ADC_MIN), (u8 *)ADC_MIN);//读取存储电位器最小值
    eepromRead(ADC_MAX_ADDR, sizeof(ADC_MAX), (u8 *)ADC_MAX);//读取存储电位器最大值
    for(int i=0;i<DWQ_NUM;i++) {
        ADC_MID[i] = (ADC_MIN[i] + ADC_MAX[i])/2;
        pos_p[i] = JXB_PWM_RANGE/abs(ADC_MIN[i] - ADC_MAX[i]);//放大倍数
    }

    //eepromRead(POS_X_ADDR, sizeof(pos_x_verify), (u8 *)pos_x_verify);//读陀螺仪X的校准值
    //eepromRead(POS_Y_ADDR, sizeof(pos_y_verify), (u8 *)pos_y_verify);//读陀螺仪Y的校准值

    pos_x_verify = EEPROM.read(POS_X_ADDR)*256 + EEPROM.read(POS_X_ADDR+1)-1024;//读陀螺仪X的校准值
    pos_y_verify = EEPROM.read(POS_Y_ADDR)*256 + EEPROM.read(POS_Y_ADDR+1)-1024;//读陀螺仪Y的校准值

    for(byte i = 0; i < SERVO_NUM; i++){
         myservo[i].attach(servo_pin[i]);   // 将5引脚与声明的舵机对象连接起来
         myservo[i].writeMicroseconds(1500);//初始化舵机为1500状态       
    } 

    //启动示意
    nled_on();beep_on();delay(100);nled_off();beep_off();delay(100);
    nled_on();beep_on();delay(100);nled_off();beep_off();delay(100);
    nled_on();beep_on();delay(100);nled_off();beep_off();delay(100);

#if 1
      sprintf(cmd_return,"x:%04d y:%04d ADC_MIN:%04d %04d %04d %04d %04d",
                          pos_x_verify, pos_y_verify, ADC_MIN[0], ADC_MIN[1], ADC_MIN[2], ADC_MIN[3], ADC_MIN[4]);
      Serial.println(cmd_return);  

      sprintf(cmd_return,"x:%04d y:%04d  ADC_MAX:%04d %04d %04d %04d %04d",
                          pos_x_verify, pos_y_verify, ADC_MAX[0], ADC_MAX[1], ADC_MAX[2], ADC_MAX[3], ADC_MAX[4]);
      Serial.println(cmd_return);  
      delay(3000); 
#endif

}

void loop(){ 
    loop_nled();        //led灯闪烁函数
    loop_key_j();       //校验函数
    loop_Gihand_R();    //旋钮控制
}

/*
    读写eeprom数据
    addr 地址
    len 数据长度
    mydat 数据buf
*/
void eepromRead(u32 addr, u8 len, u8 *mydat){
    for(u8 i = 0; i < len; i++) {//求取旋钮平均值，存储到EEPROM
        mydat[i] = EEPROM.read(addr+i);
    }        
}

/*
    读写eeprom数据
    addr 地址
    len 数据长度
    mydat 数据buf
*/
void eepromWrite(u32 addr, u8 len, u8 *mydat){
    for(u8 i = 0; i < len; i++) {//求取旋钮平均值，存储到EEPROM
        EEPROM.write(addr+i, mydat[i]);
    }        
}

void loop_key_j() {
    static u8 flag=0;
    if((digitalRead(PIN_KEY_J)==LOW) && (flag==0)) {//第一次校准，握手值：在舒适的方式下最大程度的握紧手指，此时按下校准按钮
        delay(10);    
        if((digitalRead(PIN_KEY_J)==LOW) && (flag==0)){
            while(digitalRead(PIN_KEY_J)==LOW);
            adc_read2buf();
            for(int i=0;i<DWQ_NUM;i++) {
                ADC_MIN[i] = pos[i];
            }
            beep_on();delay(100);beep_off();
            eepromWrite(ADC_MIN_ADDR, sizeof(ADC_MIN), (u8 *)ADC_MIN);//写手指采集的最小值
            flag = 1; 

            sprintf(cmd_return,"x:%d y:%d ad0:%d ad1:%d ad1:%d ad3:%d ad4:%d ",
            pos_x, pos_y, pos[0], pos[1], pos[2], pos[3], pos[4]);
            Serial.println(cmd_return);  
        }
    } else if((digitalRead(PIN_KEY_J)==LOW) && (flag==1)) {//第二次校准，伸手值：在舒适的方式下最大程度的伸开手指，此时按下校准按钮
        delay(10);    
        if((digitalRead(PIN_KEY_J)==LOW ) && (flag==1)){
            while(digitalRead(PIN_KEY_J)==LOW);
            adc_read2buf();
            for(int i=0;i<DWQ_NUM;i++) {
                ADC_MAX[i] = pos[i];
            }
            for(int i=0;i<DWQ_NUM;i++) {
                ADC_MID[i] = (ADC_MIN[i] + ADC_MAX[i])/2;
                pos_p[i] = 1.0*JXB_PWM_RANGE/(abs(ADC_MIN[i] - ADC_MAX[i]));//放大倍数
            }
            
            beep_on();delay(500);beep_off();
            eepromWrite(ADC_MAX_ADDR,  sizeof(ADC_MAX), (u8 *)ADC_MAX);//写手指采集的最大值
            //eepromWrite(POS_X_ADDR, sizeof(pos_x_verify), (u8 *)pos_x_verify);//写陀螺仪X的校准值
            //eepromWrite(POS_Y_ADDR, sizeof(pos_y_verify), (u8 *)pos_y_verify);//写陀螺仪Y的校准值

            pos_x_verify = pos_x+1024;
            pos_y_verify = pos_y+1024;
            
            EEPROM.write(POS_X_ADDR, pos_x_verify/256);
            EEPROM.write(POS_X_ADDR+1, pos_x_verify%256);//写陀螺仪X的校准值

            EEPROM.write(POS_Y_ADDR, pos_y_verify/256);
            EEPROM.write(POS_Y_ADDR+1, pos_y_verify%256);//写陀螺仪Y的校准值
            
            flag = 0;  

            sprintf(cmd_return,"x:%d y:%d ad0:%d ad1:%d ad1:%d ad3:%d ad4:%d ",
            pos_x, pos_y, pos[0], pos[1], pos[2], pos[3], pos[4]);
            Serial.println(cmd_return);    
            delay(3000);      
        }    
    }
    
    if(digitalRead(PIN_KEY_M)==LOW) {//模式切换按钮
        delay(10);    
        if(digitalRead(PIN_KEY_M)==LOW){
        }
    }    
}


int Filter(int FILTER_N, int pin) {
  int filter_buf[FILTER_N];
  int i, j;
  int filter_temp;
  for(i = 0; i < FILTER_N; i++) {
    filter_buf[i] = analogRead(pin);
    delayMicroseconds(300);
  }
  // 采样值从小到大排列（冒泡法）
  for(j = 0; j < FILTER_N - 1; j++) {
    for(i = 0; i < FILTER_N - 1 - j; i++) {
      if(filter_buf[i] > filter_buf[i + 1]) {
        filter_temp = filter_buf[i];
        filter_buf[i] = filter_buf[i + 1];
        filter_buf[i + 1] = filter_temp;
      }
    }
  }
  return filter_buf[(FILTER_N - 1) / 2];
}

void adc_read2buf() {
    static u32 knob_value;
    static int16_t ax_t[5],ay_t[5],az_t[5];
    for(int i=0;i<5;i++) {
        knob_value = 0;
        for(int j=0;j<10;j++) {
            knob_value+=analogRead(dwq_pin[i]);
            delayMicroseconds(100);
         }
         knob_value = knob_value/10.0;
         pos[i] = knob_value;
    }

    for(int i=0;i<5;i++) {
        myAdxl345.getAcceleration(&ax_t[i], &ay_t[i], &az_t[i]);
    }

    int FILTER_N = 5, filter_temp;
    for(int j = 0; j < FILTER_N - 1; j++) {
        for(int i = 0; i < FILTER_N - 1 - j; i++) {
          if(ax_t[i] > ax_t[i + 1]) {
            filter_temp = ax_t[i];
            ax_t[i] = ax_t[i + 1];
            ax_t[i + 1] = filter_temp;
          }

          if(ay_t[i] > ay_t[i + 1]) {
            filter_temp = ay_t[i];
            ay_t[i] = ay_t[i + 1];
            ay_t[i + 1] = filter_temp;
          }
        }
    }
    
    //display tab-separated accel x/y/z values
    //Serial.print("myAdxl345:\t");
    //Serial.print(ax); Serial.print("\t");
    //Serial.print(ay); Serial.print("\t");
    //Serial.println(az);
    
    pos_x = ay_t[2];
    pos_y = ax_t[2];

#if 0
      sprintf(cmd_return,"ax:%04d ay:%04d ad2:%04d ad3:%04d ad4:%04d ad5:%04d ad6:%04d ",
                          pos_x, pos_y, pos[0], pos[1], pos[2], pos[3], pos[4]);
      Serial.println(cmd_return);  
      delay(100);
#endif
}

void loop_nled() {
    static u32 systick_ms_bak = 0;
    static u8 flag=0;
    if(millis() - systick_ms_bak<500)return;
    systick_ms_bak = millis();
    flag = !flag;
    if(flag) {
        nled_on();
    } else {
        nled_off();
    }
}


/*以固定格式发送数据,定时100毫秒发一次*/
//陀螺仪可能出现的误差
#define posx_err 0
#define posy_err 0
void loop_Gihand_R(){    
    static u32 systick_ms_bak = 0, knob_value, curMode = 0;
    if(millis() - systick_ms_bak > 100) {
        systick_ms_bak = millis();
      
        //获取5个手指的数值
        adc_read2buf();

        //pwm控制
        for(int i=0;i<5;i++) {
            pwm_value[i] = (int)(1500+(pos[i]-ADC_MID[i])*pos_p[i]);
            
            if(abs(pos_bak[i] - pos[i]) >= 5) {
                pos_bak[i] = pos[i];
                
                if(pwm_value[i]>2500)pwm_value[i]=2500;
                else if(pwm_value[i]<500)pwm_value[i]=500;
                
                myservo[i+1].writeMicroseconds(pwm_value[i]); 
            }
        } 


        //Serial.print("pos_x:");
        //Serial.print(pos_x);
        pos_x_bak = pos_x;
        pos_x = pos_x-pos_x_verify;
        //Serial.print(" pos_x2:");
        //Serial.println(pos_x);
        pos_x = 1500 - pos_x*4;
        pos_x = 3000-pos_x;
        
        if(pos_x>2500)pos_x=2500;
        else if(pos_x<500)pos_x=500; 
        
        //Serial.print("pos_y:");
        //Serial.print(pos_y);
        pos_y_bak = pos_y;
        pos_y = pos_y-pos_y_verify;
        //Serial.print(" pos_y2:");
        //Serial.println(pos_y);
        pos_y = 1500 - pos_y*6;
        
        if(pos_y>2500)pos_y=2500;
        else if(pos_y<500)pos_y=500;  

        //手指数据进行更改
        if((pwm_value[0]>1800) &&
        (pwm_value[1]<1200) &&
        (pwm_value[2]<1200) &&
        (pwm_value[3]>1800) &&
        (pwm_value[4]>1800) 
         ) {//闭合手控制
            if(abs(pos_x - pos_x_bak) >= 5 || abs(pos_y - pos_y_bak) >= 5) {
                                //sprintf(cmd_return,"{#000P%04dT0500!#001P1900T0500!#002P1100T0500!#003P1100T0500!#004P1900T0500!#005P1900T0500!}", pos_x);
                //Serial.println(cmd_return);                 
                
                //云台及小车控制
                if(pos_y>2000 && abs(pos_x-1500)<500) {
                    if(curMode != 1) {
                        curMode = 1;
                        sprintf(cmd_return,"{#006P2500T0000!#007P0500T0000!}"); 
                        Serial.println(cmd_return);
                        
                    } 
                } else if(pos_y<1000 && abs(pos_x-1500)<500) {
                    if(curMode != 2) {
                        curMode = 2;
                        sprintf(cmd_return,"{#006P0500T0000!#007P2500T0000!}"); 
                        Serial.println(cmd_return);
                        
                }
                } else if(pos_x>2000) {
                    if(curMode != 3) {
                        curMode = 3;  
                        sprintf(cmd_return,"{#006P2400T0000!#007P2400T0000!}");
                        Serial.println(cmd_return);
                        
                    }   
                } else if(pos_x<1000) {
                    if(curMode != 4) {
                        curMode = 4;
                        sprintf(cmd_return,"{#006P0600T0000!#007P0600T0000!}"); 
                        Serial.println(cmd_return);
                        
                    }
                } else {
                    if(curMode != 5) {
                        curMode = 5;
                        sprintf(cmd_return,"{#006P1500T0000!#007P1500T0000!#000P1500T1500!#001P1000T1500!#002P2000T1500!#003P2000T1500!#004P2000T1500!#005P2000T1500!}"); 
                        Serial.println(cmd_return);
                    }
                }
                
            }        
        } else  if((pwm_value[0]<1200) &&
        (pwm_value[1]>1800) &&
        (pwm_value[2]>1800) &&
        (pwm_value[3]<1200) &&
        (pwm_value[4]<1200) 
        ) {//张开手控制
//            if(abs(pos_x - pos_x_bak) >= 5 || abs(pos_y - pos_y_bak) >= 5) {
//                pos_x_bak = pos_x;
//                pos_x = pos_x-pos_x_verify;
//                pos_x = 1500 - pos_x*4;
//                
//                if(pos_x>2500)pos_x=2500;
//                else if(pos_x<500)pos_x=500; 
//                
//                pos_y_bak = pos_y;
//                pos_y = pos_y-pos_y_verify;
//                pos_y = 1500 + pos_y*2;
//                
//                
//                if(pos_y>2500)pos_y=2500;
//                else if(pos_y<500)pos_y=500;  
//                
//                sprintf(cmd_return,"{#000P%04dT0500!#001P1900T1500!#002P1100T1500!#003P1100T1500!#004P1100T1500!#005P1100T1500!}", pos_y);
//                Serial.println(cmd_return); 
//            }
        } else {//手指控制
            //停止车
            if(curMode != 0) {
                curMode = 0;
                sprintf(cmd_return,"{#006P1500T0000!#007P1500T0000!}"); 
                Serial.println(cmd_return);
            }
            sprintf(cmd_return,"{#000P%04dT0100!#001P%04dT0100!#002P%04dT0100!#003P%04dT0100!#004P%04dT0100!#005P%04dT0100!}", pos_x, 3000-pwm_value[0],3000-pwm_value[1],3000-pwm_value[2],pwm_value[3],pwm_value[4]);
            Serial.println(cmd_return);  
        }

        //陀螺仪数据控制板载PWM
        myservo[0].writeMicroseconds(pos_x); 
        
    
    }
    return;
}

